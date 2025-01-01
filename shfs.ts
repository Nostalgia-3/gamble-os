// This is a parser/generator for the shitty filesystem

/********************************** ShittyFS ***********************************

I have no care for anything that real filesystems are designed for. This is
purely for loading programs from the external world. SFS partitions start with
an Identification Block (table 1.1), which points to an Information Block (table
1.2).

Table 1.1: ShFS Identification Block
|-------|-----------|----------------------------------------------------------|
| Size  | Name      | Meaning                                                  |
|-------+-----------+----------------------------------------------------------|
|     2 | JMP Short | x86 assembly to JMP an 8-bit offset. Equal to 0xEBxx.    |
|-------+-----------+----------------------------------------------------------|
|     2 | Info      | The zero-indexed logical sector of the info block in the |
|       | Block     | partition.                                               |
|-------+-----------+----------------------------------------------------------|
|     4 | Signature | The signature used to detect the ShFS partition. Equal   |
|       |           | to 0x53684653.                                           |
|-------+-----------+----------------------------------------------------------|
|     8 | Label     | The label displayed in operating systems.                |
|-------+-----------+----------------------------------------------------------|
|   496 | Code      | A super simple bootloader, or zeros if this the          |
|       |           | partition isn't bootable.                                |
|-------|-----------|----------------------------------------------------------|

Table 1.2: ShFS Information Block
|-------|-----------|----------------------------------------------------------|
| Size  | Name      | Meaning                                                  |
|-------+-----------+----------------------------------------------------------|
|     4 | Name      | Lorem ipsum or something                                 |
|-------|-----------|----------------------------------------------------------|

*******************************************************************************/