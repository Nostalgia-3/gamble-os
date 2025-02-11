#!/usr/bin/env -S deno run -A

// @ts-types="npm:@types/yargs"
import yargs from 'npm:yargs';

import { existsSync } from 'node:fs';

import * as m from './m.ts';

// The date path
const date = Date.now().toString();

const MBR               = 'build/mbr.bin';      // The first 512 bytes of the bootloader
const SECOND_STAGE      = 'build/second.bin';   // The next 2KiB of the bootloader
const KERNEL            = 'build/kernel.elf';   // A file that's supposed to be loaded by the bootloader (not yet, though)
const LINKER_SCRIPT     = 'linker.ld';

// -Wall
const CFLAGS            = `-O2 -s -m32 -fno-pie -nostdlib -ffreestanding`;
const INCLUDE           = 'include';

yargs(Deno.args)
    .strictCommands()
    .demandCommand()
    .command('run', 'Build then emulate the operating system binary', () => {}, (args) => { build(args); emulate(args); })
    .command('emulate', 'Emulate the first image binary found in build/', () => {}, (args) => { emulate(args) })
    .command('listen', 'Start an external server', () => {}, () => { m.startExServer(); })
    .command('compile', `Compile the operating system to a binary`, ()=>{}, (args)=>{ console.log(`Kernel built to ${build(args)}`); })
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
    .option('efi', {
        alias: ['uefi', 'u'],
        type: 'boolean',
        description: 'Specify whether to include Grub builds for (U)EFI',
        default: false
    })
    .option('verbose', {
        type: 'boolean',
        alias: 'v',
        description: 'Toggle verbose messages while building',
        default: false
    })
    .option('clean', {
        type: 'boolean',
        alias: 'c',
        description: 'Run a clean build, deleting the build/** directory',
        default: false
    })
.parseSync();

function emulate(pargs: Record<string, unknown>) {
    const output = m.scanDir('build', /.*\.iso/);

    if(!output[0]) {
        console.error(`\x1b[31m/!\\\x1b[0m Couldn't find an image file`);
        Deno.exit();
    }

    if(!existsSync('disk.img')) {
        m.call(`qemu-img create disk.img 512M`);
    }

    if(!existsSync('nvm.img')) {
        m.call(`qemu-img create nvm.img 8G`);
        m.call(`dd conv=notrunc if=${output} of=nvm.img`);
    }

    const qemu = [
        // Primary drive
        `-drive file=${output},format=raw,media=disk,index=0`,
        
        // Secondary drive
        `-drive if=none,file=./disk.img,format=raw,id=stick`,
        
        // NVMe drive
        `-drive file=nvm.img,format=raw,if=none,id=nvm`,
        `-device nvme,serial=test,drive=nvm`, // serial=nvmedrive,

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
        `-m 1G`
    ];

    if(pargs.exip == undefined) {
        m.call(`qemu-system-i386 ${qemu.join(' ')} -monitor stdio`);
    } else {
        m.exCall(`qemu-system-i386 ${qemu.join(' ')} -monitor stdio`, pargs.exip as string);
    }
}

function build(pargs: Record<string, unknown>) {
    m.setVerbose(pargs.verbose as boolean);

    if(pargs.clean && existsSync('build/')) {
        m.removeDir('build');
    }

    if(!existsSync('build')) {
        Deno.mkdirSync('build');
    }

    for(const bin of m.scanDir('build/', /\.iso$/)) {
        Deno.removeSync(bin);
    }

    const assemblyFiles: string[] = m.scanDir('src/', /\.asm$/, /^s\_/).sort((a,b)=>a.charCodeAt(0)-b.charCodeAt(0));
    const cFiles: string[] = m.scanDir('src/', /\.c$/);
    const objs: string[] = [];
    
    m.call(`${pargs.nasm} src/boot/s_main.asm -fbin -o ${MBR}`);
    m.call(`${pargs.nasm} src/boot/s_second.asm -fbin -o ${SECOND_STAGE}`);
    
    for(const asm of assemblyFiles) {
        objs.push(`build/${m.ext(m.base(asm), '.asm.o')}`);
        if(
            !existsSync(`build/${m.ext(m.base(asm), '.asm.o')}`) || 
            (Deno.statSync(`build/${m.ext(m.base(asm), '.asm.o')}`).mtime?.getTime() ?? 0) < (Deno.statSync(asm).mtime?.getTime() ?? 0)
        ) m.call(`${pargs.nasm} ${asm} -felf32 -o build/${m.ext(m.base(asm), '.asm.o')}`);
    }
    
    for(const c of cFiles) {
        objs.push(`build/${m.ext(m.base(c), '.c.o')}`);
        if(
            !existsSync(`build/${m.ext(m.base(c), '.c.o')}`) ||
            (Deno.statSync(`build/${m.ext(m.base(c), '.c.o')}`).mtime?.getTime() ?? 0) < (Deno.statSync(c).mtime?.getTime() ?? 0)
        ) m.call(`${pargs.gcc} -I ${INCLUDE} ${CFLAGS} -c -o build/${m.ext(m.base(c), '.c.o')} ${c}`);
    }

    m.call(`${pargs.gcc} ${CFLAGS} -z noexecstack -T ${LINKER_SCRIPT} -o ${KERNEL} ${objs.filter((v)=>v!='build/a_kernel-entry.asm.o').join(' ')}`);
    
    const outFile   = `${pargs.output ?? 'build/kernel'}${pargs['no-date'] ? '' : `-${date}`}.iso`;

    m.dirExist('build/iso/sys');
    m.dirExist('build/iso/boot/grub');
    m.dirExist('build/iso/EFI/BOOT', pargs.efi as boolean ?? false);
    if(pargs.efi) {
        m.assert(m.copyFile('resources/BOOTX64.EFI', 'build/iso/EFI/BOOT/BOOTX64.EFI'), `Failed to find "resources/BOOTX64.EFI"`);
    } else {
        m.removeDir('build/iso/EFI');
    }

    m.copyFile('grub.cfg', 'build/iso/boot/grub/grub.cfg');
    m.copyFile(KERNEL, 'build/iso/sys/kernel.elf');

    m.call(`grub-mkrescue -o ${outFile} build/iso`);

    return outFile;
}