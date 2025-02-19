// @ts-types="npm:@types/ws"
import { WebSocketServer } from 'npm:ws';
import p from 'node:path';
import { existsSync } from 'node:fs';

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

/**
 * Return the last portion of a path. This is typically used
 * to get the file name from an array
 * @param path the path to evaluate
 */
export function base(path: string) {
    return p.basename(path);
}

/**
 * Replace the extension of a file with another. This includes
 * the period
 * @param file the file to replace the extension with
 * @param ext the new extension to use
 */
export function ext(file: string, ext: string) {
    return file.replace(p.extname(file), ext);
}

/**
 * Runs a command in the command line, returning the response from
 * the command line. Optionally, you can specify whether to continue
 * if the command fails
 * @param command the command to run
 * @param cont whether to continue running if the program fails
 */
export function call(command: string, cont = false) {
    const secs = command.split(' ');
    const fn = secs.shift();
    if(!fn) return;
    verbose(`\x1b[90m[\x1b[32m?\x1b[90m]\x1b[0m ${command}`);
    const com = new Deno.Command(fn, { args: secs, stdout: 'inherit', stdin: 'inherit', stderr: 'inherit' });
    const resp = com.outputSync();
    if(!resp.success) {
        if(!cont) Deno.exit(1);
    }
    return resp;
}

/**
 * Search a directory for any files that exist in a directory, or optionally
 * respect the two regexes sent. Returns a list of relative filenames to the
 * path sent (e.g. searching `path1` will return `['file1', 'path1/file2']`)
 * @param path the path to search
 * @param need a regex that is needed to be returned
 * @param ignore a regex that filters items to be ignored
 */
export function scanDir(path: string, need?: RegExp, ignore?: RegExp): string[] {
    let fnames: string[] = [];
    
    const files = Deno.readDirSync(path);
    for(const file of files) {
        if(file.isDirectory) {
            fnames = fnames.concat(scanDir(p.join(path, file.name), need, ignore));
        } else {
            if((ignore && file.name.match(ignore)) || (need && !file.name.match(need))) {
                continue;
            }
            
            fnames.push(p.join(path, file.name));
        }
    }

    return fnames;
}

/**
 * Start an external server to run commands from another device
 * from the current runner
 * @param port the port to start the server on (default is 8086)
 */
export function startExServer(port: number = 8086) {
    const server = new WebSocketServer({
        port
    });

    server.on('connection', (s) => {
        s.on('message', (v) => {
            const msg = v.toString();

            if(msg.startsWith('r::')) {
                s.send((call(msg.slice(3), true)?.success ?? true) ? 0 : 1);
            }
        });
    });

    server.on('listening', () => {
        console.log(`Listening on ws://127.0.0.1:8086`);
    });
}

/**
 * Call a from an external server
 * @param s the command to run
 * @param ip the ip (default is 127.0.0.1) of the external server
 * @param port the port (default is 8086) of the external server
 */
export function exCall(s: string, ip?: string, port = 8086) {
    try {
        const ws = new WebSocket(`ws://${ip ?? '127.0.0.1'}:${port}`);
        
        ws.addEventListener('open', () => {
            verbose(`\x1b[90m[\x1b[34m>\x1b[90m]\x1b[0m ${s}`);
            ws.send(`r::${s}`);
            ws.close();
        });
    
        ws.addEventListener('message', (ev) => {
            console.log(ev.data);
            if(ev.data != 0) {
                console.log(`\x1b[90m[\x1b[31m%\x1b[90m]\x1b[0m Failure`);
            }
        })
    
        ws.addEventListener('error', (ev) => {
            console.log((ev as ErrorEvent).message);
            Deno.exit(1);
        });
    } catch (e) {
        console.error(`\x1b[90m[\x1b[31m%\x1b[90m]\x1b[0m Failed to send`);
        console.error(e);
    }
}

/**
 * Recursively create directories if they don't exist
 * @param s The directory
 */
export function dirExist(s: string, shouldExist: boolean = true) {
    if(!existsSync(s) && shouldExist) {
        verbose(`\x1b[90m[\x1b[32m@\x1b[90m]\x1b[0m mkdir -r "${s}"`);
        Deno.mkdirSync(s, { recursive: true });
    } else if(existsSync(s) && !shouldExist) {
        verbose(`\x1b[90m[\x1b[32m@\x1b[90m]\x1b[0m rmdir -r "${s}"`);
        Deno.removeSync(s, { recursive: true });
    }
}

export function removeDir(s: string) {
    if(!existsSync(s)) return;
    verbose(`\x1b[90m[\x1b[32m@\x1b[90m]\x1b[0m rmdir -r "${s}"`);
    Deno.removeSync(s, { recursive: true });
}

export function copyFile(src: string, dest: string) {
    if(!existsSync(src)) return false;
    verbose(`\x1b[90m[\x1b[32m@\x1b[90m]\x1b[0m COPY "${src}" "${dest}"`);
    Deno.copyFileSync(src, dest);
    
    return true;
}

export function assert(truthy: boolean, reason?: string) {
    if(!truthy) {
        console.error(`\x1b[90m[\x1b[31m%\x1b[90m]\x1b[0m ${reason ?? 'No reason given.'}`);
        Deno.exit(1);
    }
}

let vb = true;
export function setVerbose(b: boolean = true) {
    vb = b;
}

export function verbose(s: string) {
    if(!vb) return;
    console.log(s);
}