#!/usr/bin/env -S deno run -A

import path from 'node:path';

function base(s: string) {
    return path.basename(s);
}

function ext(s: string, x: string) {
    return s.replace(path.extname(s), x);
}

function call(s: string, cont = false) {
    const secs = s.split(' ')
    const fn = secs.shift();
    if(!fn) return;
    console.log(s);
    const com = new Deno.Command(fn, { args: secs, stdout: 'inherit', stdin: 'inherit', stderr: 'inherit' });
    const resp = com.outputSync();
    if(!resp.success) {
        Deno.exit(1);
    }
    return resp;
}

function scanDir(p: string, need?: RegExp, ignore?: RegExp): string[] {
    let fnames: string[] = [];
    
    const files = Deno.readDirSync(p);
    for(const file of files) {
        if(file.isDirectory) {
            fnames = fnames.concat(scanDir(path.join(p, file.name), need, ignore));
        } else {
            if((ignore && file.name.match(ignore)) || (need && !file.name.match(need))) {
                continue;
            }
            
            fnames.push(path.join(p, file.name));
        }
    }

    return fnames;
}

const MBR = 'build/mbr.bin';
const KERNEL = 'build/kernel.bin';
const BOOT = 'build/os.bin';
const LINKER_SCRIPT = 'linker.ld';

const INCLUDE = 'include';

const assemblyFiles: string[] = scanDir('src/', /\.asm$/, /^s\_/).sort((a,b)=>a.charCodeAt(0)-b.charCodeAt(0));
const cFiles: string[] = scanDir('src/', /\.c$/);

// hardcode the MBR because it's like one file
call(`nasm src/boot/s_main.asm -fbin -o ${MBR}`);

for(const asm of assemblyFiles)
    call(`nasm ${asm} -felf -o build/${ext(base(asm), '.o')}`);

for(const c of cFiles)
    call(`gcc -I ${INCLUDE} -m32 -o build/${ext(base(c), '.o')} -fno-pie -ffreestanding -c ${c}`);

call(`ld -m elf_i386 -T ${LINKER_SCRIPT} -o ${KERNEL} -Ttext 0x1000 --oformat binary ${assemblyFiles.concat(cFiles).map((v)=>path.join('build/', ext(base(v), '.o'))).join(' ')}`);

const fMBR = Deno.readFileSync(MBR);
const fKERNEL = Deno.readFileSync(KERNEL);

const fBOOT = new Uint8Array(fMBR.length + fKERNEL.length);
fBOOT.set(fMBR);
fBOOT.set(fKERNEL, fMBR.length);

Deno.writeFileSync(BOOT, fBOOT);

call(`qemu-system-i386 -drive file=${BOOT},format=raw,index=0,media=disk -m 512M`);