#!/usr/bin/env -S deno run -A

import { existsSync } from 'node:fs';
import { Builder } from 'jsr:@nostalgia3/build-tool@1.1.1';

// @ts-types="npm:@types/yargs@17.0.33"
import {  } from 'npm:yargs@17.7.2';

export class DynamicData {
    buffer: Uint8Array;
    length: number;

    constructor() {
        this.buffer = new Uint8Array(0);
        this.length = 0;
    }

    increaseSize(c: number) {
        this.length += c;
        const r = new Uint8Array(this.length);
        r.set(this.buffer);
        this.buffer = r;
    }

    writeU32LE(d: number) {
        this.increaseSize(4);
        new DataView(this.buffer.buffer).setUint32(this.length-4, d, true);
    }

    writeU32BE(d: number) {
        this.increaseSize(4);
        new DataView(this.buffer.buffer).setUint32(this.length-4, d, false);
    }

    writeU16LE(d: number) {
        this.increaseSize(2);
        new DataView(this.buffer.buffer).setUint16(this.length-2, d, true);
    }

    writeAsciiString(s: string) {
        const b = new TextEncoder().encode(s);
        this.increaseSize(b.length);
        this.buffer.set(b, this.length-b.length);
    }

    writeUint8Array(u: Uint8Array) {
        this.increaseSize(u.length);
        this.buffer.set(u, this.length-u.length);
    }
}

const DATE              = Date.now();

const KERNEL            = 'build/kernel.elf';
const LINKER_SCRIPT     = 'linker.ld';

const CFLAGS            = `-O2 -s -m32 -fno-pie -nostdlib -ffreestanding -Wall -Werror`; // -s = strip, -g = include debugger symbols
const INCLUDE           = 'include';

const b = new Builder();

b.addArgument({
    name: 'output',
    type: 'string',
    alias: ['o'],
    description: 'Specify the output iso file'
});

b.addArgument({
    name: 'gcc',
    description: 'Specify the GCC compiler binary',
    type: 'string',
    default: 'i686-elf-gcc'
});

b.addArgument({
    name: 'nasm',
    description: 'Specify the NASM assembler binary',
    type: 'string',
    default: 'nasm'
});

b.addArgument({
    name: 'qemu',
    description: 'Specify the QEMU emulator binary',
    type: 'string',
    default: 'qemu-system-i386'
});

b.addArgument({
    name: 'clean',
    alias: ['c'],
    description: 'Specify whether to remove build/** before compiling',
    type: 'boolean'
});

b.addArgument({
    name: 'emu-ip',
    alias: ['x'],
    description: 'Specify an external server to run qemu on',
    type: 'string'
});

b.addArgument({
    name: 'add-efi',
    alias: ['e'],
    description: 'Specify whether to add grub-efi to the ISO',
    type: 'boolean',
    default: false
});

b.addArgument({
    name: 'initrd',
    alias: 'i',
    description: 'The output archive file',
    type: 'string',
    default: 'build/gos.initrd'
});

b.addTask('initrd', 'Generate a GaOS initrd archive with the file specified by -i/--initrd using the directory initrd/', (args) => {
    if(!args.i) {
        b.error(`-i/--initrd not specified`);
        return -1;
    }
    
    if((args.i as string).startsWith('build/')) {
        b.createDirectory('build/');
    }

    b.runCommand(`nasm -fbin initrd/main.asm -o initrd/init`);

    const d = new DynamicData();
    
    const filecount = Deno.readDirSync('initrd/').filter((v)=>!v.isDirectory).toArray().length;

    d.writeU32BE(0x47614F53);
    d.writeU32LE(filecount)
    d.writeU16LE(0x01);
    d.writeU16LE(0x00);

    for(const cf of Deno.readDirSync('initrd/')) {
        if(cf.isDirectory) {
            b.verbose(`initrd doesn't support directories at the moment`);
            continue;
        }

        const n = cf.name;
        const c = Deno.readFileSync(`initrd/${cf.name}`);
        d.writeU32LE(n.length);
        d.writeU32LE(c.length);
        d.writeAsciiString(n);
        d.writeUint8Array(c);
    };
    
    Deno.writeFileSync(args.i as string, d.buffer);


    b.verbose(`Created archive \x1b[34m${args.i}\x1b[0m`);

    return 0;
});

async function compile(args: Record<string, unknown>) {
    if(args.c) b.remove('build/');
    b.createDirectory('build/');

    // if(!existsSync(args.i as string)) {
    // }
    b.runTask('initrd', args);

    for(const bin of b.scanDir('build/', /\.iso$/)) {
        b.remove(bin);
    }

    const assemblyFiles: string[] = b.scanDir('src/', /\.asm$/);
    const cFiles: string[] = b.scanDir('src/', /\.c$/);
    const objs: string[] = [];
    
    for(const asm of assemblyFiles) {
        const dest = `build/${b.extension(b.justFile(asm), '.asm.o')}`;
        objs.push(dest);
        b.compile(`${args.nasm} ${asm} -felf32 -o ${dest}`, asm, dest);
    }
    
    for(const c of cFiles) {
        const dest = `build/${b.extension(b.justFile(c), '.c.o')}`;
        objs.push(dest);
        b.compile(`${args.gcc} -I ${INCLUDE} ${CFLAGS} -c -o ${dest} ${c}`, c, dest);
    }

    b.runCommand(`${args.gcc} ${CFLAGS} -z noexecstack -T ${LINKER_SCRIPT} -o ${KERNEL} ${objs.filter((v)=>v!='build/entry.asm.o').join(' ')}`);

    const outFile   = args.output ?? `build/kernel-${DATE}.iso`;

    b.createDirectory('build/iso/sys');
    b.createDirectory('build/iso/boot/grub');

    b.copy('grub.cfg', 'build/iso/boot/grub/grub.cfg');
    b.copy(args.i as string, 'build/iso/sys/gos.initrd');
    b.copy(KERNEL, 'build/iso/sys/kernel.elf');

    if(args.x) {
        const sock = await Deno.connect({ hostname: '172.27.152.12' as string, port: 8096 });
        b.verbose(`grub-mkrescue -o ${outFile} build/iso`);
        await sock.write(new TextEncoder().encode(`r::grub-mkrescue -o ${outFile} build/iso`));
        await sock.read(new Uint8Array(1));
    } else {
        b.runCommand(`grub-mkrescue -o ${outFile} build/iso`);
    }

    return 0;
}

b.addTask('compile', 'Build the kernel', (args) => {
    compile(args);
    return 0;
});

b.addTask('emulate', 'Emulate the output file specified by -o/--output found in build/', (args) => {
    const output   = (args.output as string) ?? `build/kernel-${DATE}.iso`;

    if(!output[0]) {
        b.fatal(`Couldn't find an image file in build/`);
    }

    b.createDirectory('emulator/');

    if(!existsSync('emulator/disk.img')) {
        b.runCommand(`qemu-img create emulator/disk.img 512M`);
    }

    // ${output}

    const qemu = [
        // Primary drive
        `-drive file=${output},format=raw,media=disk,index=0`,
        
        // Secondary drive
        `-drive if=none,file=./emulator/disk.img,format=raw,id=stick`,

        // AC97 audio card
        `-audio driver=sdl,model=ac97,id=speaker`,
        // PC Speaker
        `-machine pcspk-audiodev=speaker`,

        // E1000 network card
        `-net nic,model=e1000,macaddr=00:11:22:33:44:55`,
        `-net user`,

        // USB stick
        '-device usb-ehci,id=ehci',
        '-device usb-storage,bus=ehci.0,drive=stick',

        // RAM
        `-m 1G`,

        // Debugging
        // `-s -S`
        // `-d int`,
        // `-no-reboot`
    ];

    b.runCommand(`qemu-system-i386 ${qemu.join(' ')} -monitor stdio`);

    return 0;
});

b.addTask('run', `Compile then emulate the kernel`, (args) => {
    if(args.x) b.allowExternal();

    compile(args).then(() => { b.runTask('emulate', args); });

    return 0;
});

b.addTask('server', `Run a server for qemu`, (_args) => {
    const listener = Deno.listen({ port: 8096 });
    b.verbose(`Started server on port :8096`);

    (async () => {
        for await(const conn of listener) {
            const buff = new Uint8Array(512);
            await conn.read(buff);
            const cmd = new TextDecoder().decode(buff);
            if(cmd.startsWith('r::')) {
                const comm = cmd.substring(3);
                const args = comm.split('').filter((v)=>v!='\0').join('').split(' ');
                new Deno.Command(args[0], { stdin: 'inherit', stdout: 'inherit', stderr: 'inherit', args: args.slice(1) }).outputSync();
            }
            conn.close();
        }
    })();
    
    return 0;
});

b.begin(Deno.args);