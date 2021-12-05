# balong-nvtool

Hisilicon Balong NVRAM image editing utility.

This utility can read and edit NVRAM images. Both NVRAM file (nv.bin) and NVRAM parititions (nvdload, nvdefault, nvbacklte) are supported. "nvimg" is a file partition with jffs/yaffs file system and is not suitale for editing directly. You should extract "nv.bin" from it first. This file is usually available inside the device as `/mnvm2:0/nv.bin`.

One can get NVRAM image contents, extract all or selected NVRAM items and component files. It's also possible to edit any NVRAM item or load it from external file. The program contains names for all known NVRAM items.  
There's also v4 OEM and SIMLOCK code brute force functionality.

`modem-bin` directory contains binary files for the on-device Linux. It can edit `/mnvm2:0/nv.bin` file directly and correctly recalculate all checksums.

This software is in English.
Please ask questions only about the program itself. No questions about devices, boot pins, loaders, NVRAM backups allowed.
