This project was about understanding how a file system works behind the scenes of the OS. Instead of using the available APIs (opendir, readdir, fopen, etc.), the program reads the EXT2 disk image and completes one of four processes.

The following processes are:

- **-info**: display the EXT2 metadata  
- **-root**: list all root directory nodes  
- **-traverse**: recursively traverse the file system  
- **-file `<absolute-path>`**: locate and print the contents of a file stored on the disk  

To navigate through the disk, the `lseek()` and `read()` functions were heavily used.

The superblock is read starting at byte offset 1024 and holds the following information: block size, inode size, total number of blocks and inodes, and the number of blocks and inodes per group.

The block group descriptors store metadata such as the block and inode bitmap locations, inode table location, free block count, and free inode count.

The disk is parsed bit by bit, where each bit corresponds to one block or inode. Nested loops are used to iterate through each byte and each individual bit within the allocation bitmaps.

Overall, this project demonstrates a deep understanding of how operating systems store, locate, and retrieve data on disk. Manually parsing an EXT2 file system from a raw disk image, without relying on OS file APIs, showcases the ability to work with low-level binary data, complex disk structures, and memory management in C. The project required careful debugging, strict attention to memory allocation, and a strong grasp of systems programming concepts.
