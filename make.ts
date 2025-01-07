#!/usr/bin/env -S deno run -A

import { existsSync } from 'node:fs';
import * as m from './m.ts';
import path from 'node:path';

const LD = `ld`;
const GCC = `gcc`;

const MBR               = 'build/mbr.bin';      // The first 512 bytes of the bootloader
const SECOND_STAGE      = 'build/second.bin';   // The next 2KiB of the bootloader
const KERNEL            = 'build/kernel.bin';   // The kernel loaded into the output ISO
const BOOT              = 'build/gambleos-{BUILD-INDEX}.img';
const LINKER_SCRIPT     = 'linker.ld';

const MAX_KERNEL_SIZE   = 1024*500;             // This is accurate enough
const FILE_SIZE         = 1024;                 // Size of the output .img file in 512-byte sectors (512KiB)

const INCLUDE           = 'include';

if(Deno.args[0] == 'listen') {
    m.startExServer();
} else {
    let { build_index }: {
        build_index: number
    } = JSON.parse(Deno.readTextFileSync('make.json'));

    build_index++;

    if(existsSync('build')) {
        Deno.removeSync('build', { recursive: true });
    }

    Deno.mkdirSync('build');

    if(!existsSync('make.json')) {
        Deno.writeTextFileSync('make.json', JSON.stringify({
            build_index: 0
        }));
    }

    const assemblyFiles: string[] = m.scanDir('src/', /\.asm$/, /^s\_/).sort((a,b)=>a.charCodeAt(0)-b.charCodeAt(0));
    const cFiles: string[] = m.scanDir('src/', /\.c$/);

    console.log(`[BUILD INDEX] ${build_index}`);
    
    m.call(`nasm src/boot/s_main.asm -fbin -o ${MBR}`);
    m.call(`nasm src/boot/s_second.asm -fbin -o ${SECOND_STAGE}`);
    
    for(const asm of assemblyFiles)
        m.call(`nasm ${asm} -felf -o build/${m.ext(m.base(asm), '.asm.o')}`);
    
    for(const c of cFiles) {
        Deno.writeTextFileSync(
            c,
            Deno.readTextFileSync(c).replaceAll('<MAKE::BUILD_INDEX>', build_index.toString())
        );
        m.call(`${GCC} -I ${INCLUDE} -O1 -m32 -o build/${m.ext(m.base(c), '.c.o')} -fno-pie -ffreestanding -c ${c}`);
    }

    const files: string[] = [];

    for(const file of assemblyFiles.map((v)=>path.join('build/', m.ext(m.base(v), '.asm.o')))) {
        files.push(file);
    }

    for(const file of cFiles.map((v)=>path.join('build/', m.ext(m.base(v), '.c.o')))) {
        files.push(file);
    }

    m.call(`${LD} -m elf_i386 -T ${LINKER_SCRIPT} -o ${KERNEL} --oformat binary ${files.join(' ')}`);
    
    const fMBR = Deno.readFileSync(MBR);
    const fKERNEL = Deno.readFileSync(KERNEL);
    const fSECOND = Deno.readFileSync(SECOND_STAGE);
    
    console.log(`kernel size: ${fKERNEL.length} bytes (bytes remaining: ${MAX_KERNEL_SIZE - fKERNEL.length})`);

    const fBOOT = new Uint8Array(FILE_SIZE*512);
    fBOOT.set(fMBR);
    fBOOT.set(fSECOND, fMBR.length);
    fBOOT.set(fKERNEL, fMBR.length+fSECOND.length);
    
    const outFile   = BOOT.replaceAll('{BUILD-INDEX}', build_index.toString());

    Deno.writeFileSync(outFile, fBOOT);


    if(!existsSync('disk.img')) {
        m.call(`qemu-img create disk.img 512M`);
    }

    // update the .make.json
    Deno.writeTextFileSync('make.json', JSON.stringify({
        build_index
    }));

    // -device usb-tablet,bus=usb-bus.0 -device usb-storage,bus=ehci.0,drive=usbstick
    const qemu = [
        // Primary drive
        `-drive file=${outFile},format=raw,media=disk,index=0`,
        // Secondary drive
        `-drive file=./disk.img,format=raw,media=disk,index=1`,
        // AC97 audio card
        `-audio driver=sdl,model=ac97,id=speaker`,
        // PC Speaker
        `-machine pcspk-audiodev=speaker`,
        // RAM
        `-m 1G`
    ];

    m.exCall(`qemu-system-i386 ${qemu.join(' ')} -monitor stdio`, Deno.args[0]);
}