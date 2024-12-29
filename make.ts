#!/usr/bin/env -S deno run -A

import { existsSync } from 'node:fs';
import * as m from './m.ts';
import path from 'node:path';

const LD = `ld`;
const GCC = `gcc`;

const MBR = 'build/mbr.bin';
const KERNEL = 'build/kernel.bin';
const BOOT = 'build/os.bin';
const LINKER_SCRIPT = 'linker.ld';

const BIN_SIZE = 4194304;
// const BIN_SIZE = 512*20;

const INCLUDE = 'include';

if(Deno.args[0] == 'listen') {
    m.startExServer();
} else {
    if(!existsSync('build')) {
        Deno.mkdirSync('build');
    }
    
    const assemblyFiles: string[] = m.scanDir('src/', /\.asm$/, /^s\_/).sort((a,b)=>a.charCodeAt(0)-b.charCodeAt(0));
    const cFiles: string[] = m.scanDir('src/', /\.c$/);
    
    // hardcode the MBR because it's like one file
    m.call(`nasm src/boot/s_main.asm -fbin -o ${MBR}`);
    
    for(const asm of assemblyFiles)
        m.call(`nasm ${asm} -felf -o build/${m.ext(m.base(asm), '.asm.o')}`);
    
    for(const c of cFiles)
        m.call(`${GCC} -I ${INCLUDE} -O1 -m32 -o build/${m.ext(m.base(c), '.c.o')} -fno-pie -ffreestanding -c ${c}`);

    const files: string[] = [];

    for(const file of assemblyFiles.map((v)=>path.join('build/', m.ext(m.base(v), '.asm.o')))) {
        files.push(file);
    }

    for(const file of cFiles.map((v)=>path.join('build/', m.ext(m.base(v), '.c.o')))) {
        files.push(file);
    }

    m.call(`${LD} -m elf_i386 -T ${LINKER_SCRIPT} -o ${KERNEL} -Ttext 0x1000 --oformat binary ${files.join(' ')}`);
    
    const fMBR = Deno.readFileSync(MBR);
    const fKERNEL = Deno.readFileSync(KERNEL);
    
    console.log(`kernel size: ${fMBR.length+fKERNEL.length} bytes (bytes remaining: ${BIN_SIZE - fMBR.length - fKERNEL.length})`);

    const fBOOT = new Uint8Array(BIN_SIZE); // 4MiB
    fBOOT.set(fMBR);
    fBOOT.set(fKERNEL, fMBR.length);
    
    Deno.writeFileSync(BOOT, fBOOT);

    m.exCall(`qemu-system-i386 -drive file=${BOOT},format=raw,index=0,media=disk -drive file=disk.img,format=raw,index=1,media=disk -m 512M -monitor stdio -audio sdl,model=sb16`, '172.25.112.1');
}