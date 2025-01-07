import { Buffer } from "node:buffer";

type DirectoryEntry = { len: number, filename: string, lba: number, dataLength: number, };

function parseDirectoryEntry(buf: Buffer, off: number) {
    const len           = buf.readUint8(off);           // +1
    const lba           = buf.readUint32LE(off+2);      // +8 (u32le + u32be)
    const dataLength    = buf.readUint32LE(off+10);     // +8 (u32le + u32be)
    const filenameLen   = buf.readUint8(off+32);        // +1
    const filename      = buf.subarray(off+33,off+filenameLen+33).toString();

    return {
        len,
        filename,
        lba, dataLength,
    };
}

function translatePathName(s: string) {
    if(s == '\x00') return '.';
    if(s == '\x01') return '..';
    else return s;
}

function searchDir(s: string, d: DirectoryEntry) {
    let dir = d;
    let i = 0;
    const lba = d.lba;
    const dlen = d.dataLength;
    while(dir.filename != s) {
        currentSector = readSectors(lba*4, 4);
        // .subarray(lbs*lba+i, lbs*lba+dlen+i)
        dir = parseDirectoryEntry(currentSector, i);
        if(dir.len == 0) {
            return null;
        }
        i+=dir.len;
    }

    return dir;
}

function listDir(d: DirectoryEntry): Record<string,unknown>[] {
    let dir = d;
    let i = 0;
    const files = [];
    let lba = d.lba;
    const dlen = d.dataLength;
    while(dir.len != 0) {
        currentSector = readSectors(lba*4, 4);

        // f.subarray(lbs*lba+i, lbs*lba+dlen+i)
        dir = parseDirectoryEntry(currentSector, i);

        if(dir.len == 0) {
            break;
        }

        files.push({ name: translatePathName(dir.filename), len: dir.len });
        i+=dir.len;
    }

    return files;
}

const j = Buffer.from(Deno.readFileSync('build/gambleos-1218.iso'));

function readSectors(c: number, count: number) {
    return j.subarray(c*512, (c+count)*512);
}

let currentSector = readSectors(64, 1);

// get the volume descriptor
const version   = currentSector.readUInt8(0);
const str       = currentSector.subarray(1, 6).toString('ascii');
const vdv       = currentSector.readUInt8(6);

if(version != 1 || str != 'CD001' || vdv != 1) {
    console.error(`Invalid data`);
    Deno.exit(1);
}

// const pathTableSize = s1.readUint32LE(132);
// const lPathTable = s1.readUInt32LE(140);
// const pathTable = parsePathTable(f.subarray(lPathTable*lbs, lPathTable*lbs + pathTableSize), 0);

const lbs = currentSector.readUint16LE(128);
const root = parseDirectoryEntry(currentSector, 156);

console.log(listDir(root));

// const sys = searchDir('SYS', root);

// if(sys == null) {
//     console.error(`Couldn't find SYS`);
//     Deno.exit(1);
// }

// console.log(listDir(sys));