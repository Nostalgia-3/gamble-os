#!/usr/bin/env -S deno run -A

import { existsSync } from 'node:fs';
import * as m from './m.ts';
import path from 'node:path';

const LD = `ld`;
const GCC = `gcc`;

const MBR = 'build/mbr.bin';
const KERNEL = 'build/kernel.bin';
const BOOT = 'build/gambleos-{BUILD-INDEX}.bin';
const LINKER_SCRIPT = 'linker.ld';

// const BIN_SIZE = 1048576;
const BIN_SIZE = 524288; // 512KiB

const INCLUDE = 'include';

if(Deno.args[0] == 'listen') {
    m.startExServer();
} else {
    Deno.removeSync('build', { recursive: true });

    if(!existsSync('build')) {
        Deno.mkdirSync('build');
    }

    if(!existsSync('make.json')) {
        Deno.writeTextFileSync('make.json', JSON.stringify({
            build_index: 0
        }));
    }

    let { build_index }: {
        build_index: number
    } = JSON.parse(Deno.readTextFileSync('make.json'));
    
    const assemblyFiles: string[] = m.scanDir('src/', /\.asm$/, /^s\_/).sort((a,b)=>a.charCodeAt(0)-b.charCodeAt(0));
    const cFiles: string[] = m.scanDir('src/', /\.c$/);

    console.log(`[BUILD INDEX] ${build_index}`);
    
    // hardcode the MBR because it's like one file
    m.call(`nasm src/boot/s_main.asm -fbin -o ${MBR}`);
    
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
    
    console.log(`kernel size: ${fKERNEL.length} bytes (bytes remaining: ${BIN_SIZE - fMBR.length - fKERNEL.length})`);

    const fBOOT = new Uint8Array(BIN_SIZE); // 4MiB
    fBOOT.set(fMBR);
    fBOOT.set(fKERNEL, fMBR.length);

    build_index++;
    
    Deno.writeFileSync(BOOT.replaceAll('{BUILD-INDEX}', build_index.toString()), fBOOT);

    // update the .make.json
    Deno.writeTextFileSync('make.json', JSON.stringify({
        build_index
    }));

    if(!existsSync('disk.img')) {
        // should be raw, idk though
        m.call(`qemu-img create disk.img 512M`);
    }

    // '172.25.112.1' (leaving this here) Deno.args[1]
    m.exCall(`qemu-system-i386 -drive file=${BOOT.replaceAll('{BUILD-INDEX}', build_index.toString())},format=raw,media=disk -drive file=disk.img,format=raw,media=disk -m 1G -monitor stdio -audiodev sdl,id=speaker -machine pcspk-audiodev=speaker`, Deno.args[0]);
}
