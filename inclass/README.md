> Show disk operations in accessing a file in the following file systems.

Let the file is `\a\b\c.txt`

1. **Unix File System** 
    - The first inode we need is the inode for the root. This is stored in the super/master block.
    - From root inode we can access the data blocks(first). There are generally 12 direct blocks which holds the data and 3 pointers to indirect block pointers. (Single, Double, Triple indirect)
    - From root inode open the data block and search for `a` directory.
    - Once we find an entry to `a` we can read the indoe for `a`
    - From inode `a`, read the data from data blocks and find address of inode `b`
    - Read inode for `b`. Goto to the data blocks and find inode for `c`
    - Read inode for `c` and the data from the data blocks pointers.    
    
2. **Fast File System**
    
    Builds on top of some of the shortcomings on the original unix file system. Improves seek time and  disk management with hueristics and helps fragmantation. Introduced cylinder groups and data blocks of the same files arae placed across same cylinder groups in the disk. Also files under the same directory are placed in the same cylinder group. Addinollly inodes of the files are also place in the same cylinder group as the data block .Directories ar distributed accross all the cylinder groups to keep space for files. The disk access would look like

    For the above path `\a\b\c.txt` the cylinder group placement would be for example:
    
        Groups | Inode | Data
        --- | --- | ---
        0 | / | /
        1 | a | a
        2 | b, c  | b,c 

    - Find the root inode in cylinder group 0. 
    - Read the data for `root` inode. This data block would also be in the same cylinder group. Get the inode address for `a`  
    - Searhc inode for `a` in cylinder group `1`. Read data block from the same cylinder group and get inode address for `b`
    - Search inode for `b` in cylinder group `2`. Read data block from the same cylinder group and get inode address for `c`.
    - As `c` is a file under `b` it will be in the same cylinder group. 
    - Search inode for `c` in cylinder group `2`. Read data block from the same cylinder group.

        
3. **Log-Structured File System** 

    Any changes to files are not made to data files, but these changes are put in a log segment. This log segment may hold logs for multiple files and we can dump this to disk sequentially. This can be done periodically or once the log segment fills up. As there is no data file ever in the file system. Whever there is a read of the file, FS fetches the logs from the log segments and then recontructs the file from the log semgents and gives is to the user. This reconstructed file is also stored in memory with cache to get quick access on next read.

    On disk we always write sequentially and never overwrite. Thus there is no disk seek required. 

    LFS uses one more level of indirection called inode maps to map the inode location. These inode maps are places in checkpoint region and also cahed for performance. To access file:

    - The first on-disk data structure we must read is the checkpoint region. The checkpoint region contains pointers (i.e., disk addresses) to the entire inode map, and thus LFS then reads in the entire inode map and caches it in memory. 
    - After this point, when given an inode number of a file, LFS simply looks up the inode-number to inode-diskaddress mapping in the imap, and reads in the most recent version of the inode. 
    - To read a block from the file, at this point, LFS proceeds exactly as a typical UNIX file system, by using direct pointers or indirect pointers or doubly-indirect pointers as need be. 
    - In the common case, LFS should perform the same number of I/Os as a typical file system when reading a file from disk; the entire imap is cached and thus the extra work LFS does during a read is to look up the inode’s address in the imap

