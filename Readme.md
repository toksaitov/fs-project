COM 341, Operating Systems
==========================
# Project #3

![Visualizing a File System Data Structure](https://raw.githubusercontent.com/rachmiroff/images/refs/heads/main/auca/com-341/fall-2025/fs-project/fsviz_output.png)

After building the most basic user-space programs such as a toy shell, a task information utility, and basic file and text manipulation programs, thanks to your hard work on the course, the infamous instructor [Sergei Rachmiroff](https://i.imgur.com/hLHngQQ.jpg) has decided to move to yet another challenging element of any decent operating system: a file system. Sergei decided not to go with some existing famous file system from the `fat` or `ext` family, instead creating his own design. He created what he called KRFFS, based on the first-fit linked list memory allocator presented in the book _The C Programming Language_ by Brian Kernighan and Dennis Ritchie, mostly because he was a big fan of K&R and their book that popularized the C language (and the tradition to write hello-world as a first program). KRFFS stands for Kernighan's and Ritchie's File File System. The term "File File System" means that currently it is a file system that uses a simple file as backing storage and not a real storage device. It also utilizes [FUSE](https://github.com/libfuse/libfuse) to build and run a file system driver in userspace. Using FUSE and a file as a disk simplifies the compilation process and allows any existing Unix-like system to be used for development, debugging, and testing, improving the overall iteration time. Hopefully, one day, when the system is ready and can use real disk storage, KRFFS will turn to KRFS as it will be a file system to manage a disk, not a file system to manage a file.

Aside from non-standard backing storage, the current implementation of the system is very limited and misses some basic functionality one may expect any decent modern file system to support. Which major functionality is missing? Try to figure it out yourself by building and using the system on our server. After that, help Sergei prepare some common file system support programs, such as a program to initialize a new file system (`mkfs.krffs.c`), a program to check the consistency of the file system (`fsck.krffs.c`), a program to edit a file system (`edfs.krffs.c`), and a program to defragment the file system (`defrag.krffs.c`), as its design is prone to external fragmentation. Each year students help with just one program. Your year of study got lucky building the defragmentation utility called `defrag.krffs.c`.

In this work, we hope you will

* Learn about the concept of file systems.
* Learn about popular contemporary file systems.
* Learn about file system mounting on Unix.
* Learn about fundamental algorithms such as linked allocation and data structures like free lists for managing storage, including memory and disk space.
* Learn about the strengths and weaknesses of these approaches, particularly regarding implementation complexity, performance, and data fragmentation.
* Learn about the concepts of internal and external fragmentation.
* Learn about the role of the virtual memory subsystem in isolating and extending process memory.
* Learn about the ability of the virtual memory subsystem to abstract disk access through mapping functions such as `mmap`.
* Learn about the most common Unix system calls for working with files.
* Learn about FUSE and how to implement file system drivers in userspace.
* Learn about the strengths and weaknesses of userspace drivers in terms of convenience, safety, and performance.

## Required Tools

On your machine, you will need a GNU/Linux environment with basic Unix command-line utilities (ensure you have `dd` installed), GCC or Clang compilers, GNU Make, and most importantly FUSE libraries, headers, and utilities. If you want to work on your machine, use your OS distribution's package manager to install FUSE and pkg-config libraries with development files and utilities. Connect to our course server if you want an environment where all the prerequisites are already installed. You may also find the usual tools such as `ssh` with `scp` and `git` useful for transferring files between the server and your personal machine or submitting results to your GitHub repository provided by the instructor.

We strongly recommend working in a GNU/Linux environment, specifically the one available on our course server. However, it is also possible to work on macOS or Windows. macOS users will need to install [FUSE-T](https://www.fuse-t.org), and Windows users will need to install [Dokany](https://github.com/dokan-dev/dokany/releases), which provides a FUSE emulation layer. The project has been tested on the latest versions of both systems (for Windows, use the Visual Studio Solution `krffs.sln` in the `vs-dokany` directory to build), but support for these platforms is limited.

## Compilation

To compile the FUSE KRFFS program and all supporting utilities (to create and check the file system) on *nix platforms, use the following command (ensure to install the prerequisites first).

    make

To remove compiled files:

    make clean

## Usage

1. Create an empty 10-megabyte file `file_system.krffs` and initialize a new KRFFS file system on it with the compiled `mkfs.krffs` utility. You can also change the size to anything else reasonable. Remember that on the server you are using a shared environment.

        dd if=/dev/zero of=file_system.krffs bs=1048576 count=1
        ./mkfs.krffs file_system.krffs

   You can do the same by calling a helper script.

        chmod +x create_file_system.krffs.sh
        ./create_file_system.krffs.sh

2. Mount the file system from the `file_system.krffs` file into an empty directory `mount_point`.

        ./krffs file_system.krffs mount_point

   If the `mount_point` directory is not empty, remove the files from it.

        rm -rf mount_point/*
        rm mount_point/.gitignore
        ./krffs file_system.krffs mount_point

   One useful option is to tell the program to print debug information for each file system operation with the `-d` flag.

        ./krffs file_system.krffs mount_point -d

   You can also check the list of all FUSE options by passing just the `-h` flag.

        ./krffs -h

   You can see if your system is mounted by calling `df`. Your system should be somewhere at the bottom of the list.

        df -h

3. Go inside the directory `mount_point` and experiment with the file system by creating files and writing or reading data from them. Attempt to rename or remove files. Try to use unsupported operations. Find out what our file system is not capable of doing.

   Sample commands:

        cd mount_point
        touch hello.txt
        ls
        echo "hello, world" > hello.txt
        cat hello.txt
        echo "bye" >> hello.txt
        cat hello.txt
        rm hello.txt
        ls
        echo -e '#include <stdio.h>\n\nint main() {\n    printf("Hello, World!\\n");\n    return 0;\n}' > test.c
        mv test.c hello.c
        make hello
        ./hello
        SIZE=1M ; head -c $SIZE /dev/urandom > file_$SIZE.bin
        cp file_1M.bin file_1M_2.bin
        SIZE=4M ; head -c $SIZE /dev/urandom > file_$SIZE.bin
        rm file_1M_2.bin
        mkdir sample_dir # Huh?
        ...

4. Leave the `mount_point` directory and unmount it.

        cd ..
        sudo umount mount_point

   On some operating systems, a `fusermount` command can be used to unmount FUSE mount points without administrative privileges.

        fusermount -u mount_point

5. Visualize the file system structure with the `fsviz.krffs` program. The program uses the termbox2 [library](https://github.com/termbox/termbox2) to create an interactive interface. You can scroll with arrow keys if the visualization is too wide. Press CTRL+C to exit the program.

        ./fsviz.krffs file_system.krffs

## What to Do

Read Section 8.7 _A Storage Allocator_ from the famous K&R book to learn more about the algorithms that inspired the creation of our system. Look into our course book, specifically Section 14.4 _Allocation Methods_, comparing different ways to manage allocated and unallocated space. Study the file system by working with it and analyzing its core files in `krffs_file_system.c`, `krffs_node.c`, `krffs_allocator.c`, `krffs_platform.c`, `krffs_utilities.c`, and `krffs_fuse_operations.c`. Try to draw parallels to ideas presented in the books. Figure out the problems with our approach, specifically external fragmentation that happens when users start removing files. After realizing the problem, finish writing the code under the TODO comment in `defrag.krffs.c`. Check the file system by running the visualization tool `./fsviz.krffs` and accessing files at the `mount_point`. The files should not be corrupted. If you are confident in your defragmentation logic, commit and push your results to the GitHub private repository provided by the instructor to run the grader. Check the grader to ensure your solution passes. If it passes and you are confident in the quality of your code, submit the URL pointing to the last successful commit to Moodle.

### What to Submit

1. Update the `defrag.krffs.c` file by implementing the defragmentation code to replace the TODO comment.

2. Commit and push your changes to the repository through Git. Submit the URL of your last commit on GitHub to Moodle before the deadline.

### Deadline

Check Moodle for information regarding the deadlines.

## Additional Information

### Web Resources

* [FUSE, Filesystem in Userspace](https://github.com/libfuse/libfuse)
* [FUSE-T, FUSE for macOS](https://www.fuse-t.org)
* [Dokany](https://github.com/dokan-dev/dokany)
* [termbox2](https://github.com/termbox/termbox2)

### Documentation

    man 4 fuse
    man 1 fusermount
    man 2 mount
    man 2 umount
    man 2 open
    man 2 read
    man 2 write
    man 2 mknod
    man 2 creat
    man 2 fsync
    man 2 truncate
    man 2 unlink
    man 2 rename
    man 2 chmod
    man 2 chown
    man 2 readdir
    man 2 statfs
    man 2 mmap
    man 1 pkg-config

### Books

* _C Programming Language, 2nd Edition by Brian W. Kernighan, Dennis M. Ritchie_

* _Operating System Concepts, 10th Edition by Avi Silberschatz_
