KRFFS
=====

A FUSE userspace single-level file system in a file based on the first-fit
linked list memory allocator presented in the book _The C Programming Language_
by Brian Kernighan and Dennis Ritchie.

## Prerequisites

* FUSE libraries, headers, and utilities
* GCC or Clang compilers with pkg-config and GNU Make installed
* dd to generate empty file system files

### Fuse Installation

#### Linux

Use a package manager of your OS distribution and install FUSE libraries,
development files, and utilities.

#### Mac OS X

Download and install the [FUSE for macOS](https://osxfuse.github.io) package.

#### Windows

Download and install the latest version of [Dokany](https://github.com/dokan-dev/dokany/releases),
a user mode file system library for Windows with a FUSE wrapper.

Support for Windows is lackluster. You need to generate a VS project on your
own.

## Compilation

To compile the FUSE KRFFS program and all supporting utilities (to create,
check, and defragment the file system), use the following

    make

To remove compiled files

    make clean

## Usage

1. Create an empty 10-megabyte file `file_system.krffs` and initialize a new
   KRFFS file system on it with the compiled `mkfs.krffs` utility.

        dd if=/dev/zero of=file_system.krffs bs=1048576 count=1
        mkfs.krffs file_system.krffs

   You can do the same by calling a helper script

        chmod +x create_file_system.krffs.sh
        ./create_file_system.krffs.sh

2. Mount the file system from the `file_system.krffs` file into an empty
   directory `mount_point`.

        ./krffs file_system.krffs mount_point

   If the `mount_point` directory is not empty, remove the files from it.

        rm -rf mount_point/*
        rm mount_point/.gitignore
        ./krffs file_system.krffs mount_point

   One useful option is to tell the program to print debug information for each
   file system operation with the `-d` flag.

        ./krffs file_system.krffs mount_point -d

   You can also check the list of all FUSE options by passing just the `-h` flag.

        ./krffs -h

3. Go inside the directory `mount_point`, experiment with the file system by
   creating files and writing or reading data from them. Attempt to rename or
   remove files. Try to use unsupported operations by attempting to create a new
   directory.

   Sample commands

        cd mount_point
        touch hello.txt
        echo "hello, world" > hello.txt
        cat hello.txt
        echo "#include <stdio.h>" > test.c
        echo "bye" > hello.txt
        rm hello.txt
        mv test.c hello.c
        cp hello.c hello_backup.c
        mkdir sample_directory
        nano hello_backup.c
        ...

4. Leave the `mount_point` directory and unmount it.

        cd ..
        sudo umount mount_point

   On some systems, a `fusermount` command can be used to unmount FUSE mount
   points without administrative privileges.

        fusermount -u mount_point

## Tasks

Use the provided template `fsck.krffs.c` and finish writing the file system
check utility. The application should go through the file system structure and
report about any data inconsistencies. The checks that you need to do are
described in the comments of the `fsck.krffs.c` file.

#### What to Submit

1. In your private course repository that was given to you by the instructor
   during the lecture, create the path `project-4/`.

2. Put the `fsck.krffs.c` into the course repository.

3. Commit and push your repository through Git. Submit the last commit URL to
   Canvas before the deadline.

#### Deadline

Check Canvas for information about the deadlines.

## Resources

* [FUSE, Filesystem in Userspace](http://fuse.sourceforge.net)
* [FUSE for OS X](https://osxfuse.github.io)
* [Dokany](https://github.com/dokan-dev/dokany)

## Documentation

    man fuse
    man mount
    man umount
    man fusermount
    man pkg-config

