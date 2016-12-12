> Show disk operations in accessing a file in the following file systems.
Let the file is `\a\b.txt`

1. **Journal File System** 
    - The first inode we need is the inode for the root. This is stored in the super/master block.
    - From root inode we access the data blocks. There are generally 12 direct blocks which holds the data and 3 pointers to indirect block pointers. (Single, Double, Triple indirect)
    - From root inode open the data block and search for `a` directory.
    - Once we find an entry to `a` we can read the inode number for 
    
2. **Fast File System** 

3. **Log-Structured File System** 
