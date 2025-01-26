#!/usr/bin/env -S deno run -A

// @ts-types="npm:@types/yargs"
import yargs from 'npm:yargs';

import { existsSync } from 'node:fs';
import path from 'node:path';

import * as m from './m.ts';

// The date path
const date = Date.now().toString();

const MBR               = 'build/mbr.bin';      // The first 512 bytes of the bootloader
const SECOND_STAGE      = 'build/second.bin';   // The next 2KiB of the bootloader
const KERNEL            = 'build/kernel.bin';   // A file that's supposed to be loaded by the bootloader (not yet, though)
const LINKER_SCRIPT     = 'linker.ld';

const MAX_KERNEL_SIZE   = 1024*512;             // This is accurate enough
const FILE_SIZE         = 1024;                 // Size of the output .img file in 512-byte sectors (512KiB)

const INCLUDE           = 'include';

yargs(Deno.args)
    .strictCommands()
    .demandCommand()
    .command('run', 'Build then run the operating system binary', (yargs) => {
        return yargs.option('outsize', {
            alias: 's',
            type: 'number',
            description: 'The size of the output image file',
            default: 512*1024
        });
    }, (args) => { build(args) })
    .command('emulate', 'Emulate the first image binary found in build/', () => {}, (args) => { emulate(args) })
    .command('listen', 'Start an external server', () => {}, () => {
        m.startExServer();
    })
    .option('output', {
        alias: 'o',
        type: 'string',
        description: 'The output image file; a date is padded unless -d/--no-date is specified'
    })
    .option('no-date', {
        alias: 'd',
        type: 'boolean',
        description: 'Disable the build date at the end of the kernel image file'
    })
    .option('exip', {
        alias: 'x',
        type: 'string',
        description: 'The external server\'s ip for emulating'
    })
    .option('gcc', {
        type: 'string',
        description: 'Specifify a specific binary for gcc',
        default: 'gcc'
    })
    .option('ld', {
        type: 'string',
        description: 'Specifify a specific binary for ld',
        default: 'ld'
    })
    .option('nasm', {
        type: 'string',
        description: 'Specifify a specific binary for nasm',
        default: 'nasm'
    })
.parseSync();

function emulate(pargs: Record<string, unknown>) {
    const output = m.scanDir('build', /.*\.os/);

    if(!output[0]) {
        console.error(`\x1b[31m/!\\\x1b[0m Couldn't find an image file`);
        Deno.exit();
    }

    if(!existsSync('disk.img')) {
        m.call(`qemu-img create disk.img 512M`);
    }

    const qemu = [
        // Primary drive
        `-drive file=${output},format=raw,media=disk,index=0`,
        // Secondary drive
        `-drive file=./disk.img,format=raw,media=disk,index=1`,
        // AC97 audio card
        `-audio driver=sdl,model=ac97,id=speaker`,
        // PC Speaker
        `-machine pcspk-audiodev=speaker`,
        // E1000 network card
        `-net nic,model=e1000,macaddr=00:11:22:33:44:55`,
        `-net user`,
        // Add a generic usb device
        '-device qemu-xhci',
        // RAM
        `-m 512M`,
        // Give information about interrupts
        // `-d int`
    ];

    if(pargs.exip == undefined) {
        m.call(`qemu-system-i386 ${qemu.join(' ')} -monitor stdio`);
    } else {
        m.exCall(`qemu-system-i386 ${qemu.join(' ')} -monitor stdio`, pargs.exip as string);
    }
}

function build(pargs: Record<string, unknown>) {
    if(!existsSync('build')) {
        Deno.mkdirSync('build');
    }

    for(const bin of m.scanDir('build/', /\.os$/)) {
        Deno.removeSync(bin);
    }

    const assemblyFiles: string[] = m.scanDir('src/', /\.asm$/, /^s\_/).sort((a,b)=>a.charCodeAt(0)-b.charCodeAt(0));
    const cFiles: string[] = m.scanDir('src/', /\.c$/);
    
    m.call(`${pargs.nasm} src/boot/s_main.asm -fbin -o ${MBR}`);
    m.call(`${pargs.nasm} src/boot/s_second.asm -fbin -o ${SECOND_STAGE}`);
    
    for(const asm of assemblyFiles) {
        if(
            !existsSync(`build/${m.ext(m.base(asm), '.asm.o')}`) || 
            (Deno.statSync(`build/${m.ext(m.base(asm), '.asm.o')}`).mtime?.getTime() ?? 0) < (Deno.statSync(asm).mtime?.getTime() ?? 0))
        m.call(`${pargs.nasm} ${asm} -felf32 -o build/${m.ext(m.base(asm), '.asm.o')}`);
    }
    
    for(const c of cFiles) {
        if(
            !existsSync(`build/${m.ext(m.base(c), '.c.o')}`) ||
            (Deno.statSync(`build/${m.ext(m.base(c), '.c.o')}`).mtime?.getTime() ?? 0) < (Deno.statSync(c).mtime?.getTime() ?? 0))
        m.call(`${pargs.gcc} -I ${INCLUDE} -O2 -m32 -o build/${m.ext(m.base(c), '.c.o')} -fno-pie -ffreestanding -c ${c}`);
    }

    const objs: string[] = [];

    for(const file of assemblyFiles.map((v)=>path.join('build/', m.ext(m.base(v), '.asm.o')))) {
        objs.push(file);
    }

    for(const file of cFiles.map((v)=>path.join('build/', m.ext(m.base(v), '.c.o')))) {
        objs.push(file);
    }

    m.call(`${pargs.gcc} -T ${LINKER_SCRIPT} -o ${KERNEL} -ffreestanding -O2 -nostdlib -m32 ${objs.join(' ')}`);
    
    const fMBR = Deno.readFileSync(MBR);
    const fKERNEL = Deno.readFileSync(KERNEL);
    const fSECOND = Deno.readFileSync(SECOND_STAGE);
    
    console.log(`kernel size: ${fKERNEL.length} bytes (bytes remaining: ${MAX_KERNEL_SIZE - fKERNEL.length})`);

    const fBOOT = new Uint8Array(FILE_SIZE*512);
    fBOOT.set(fMBR);
    fBOOT.set(fSECOND, fMBR.length);
    fBOOT.set(fKERNEL, fMBR.length+fSECOND.length);
    
    const outFile   = `${pargs.output ?? 'build/kernel'}${pargs['no-date'] ? '' : `-${date}`}.os`;

    Deno.writeFileSync(outFile, fBOOT);

    // Make a GRUB iso
    // Deno.mkdirSync('build/iso/sys');
    // Deno.mkdirSync('build/iso/boot/grub', { recursive: true });
    // Deno.copyFileSync('grub.cfg', 'build/iso/boot/grub/grub.cfg');
    // Deno.copyFileSync(KERNEL, 'build/iso/sys/kernel.bin');

    // m.call(`grub-mkrescue -o ${outFile} build/iso`);

    emulate(pargs);
}