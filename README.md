# NachOS-Vigier-SOII

### nachOS-Vigier-SOII
Not Another Completely Heuristic Operating System, functional operating system developed as a project in the subject of OS II

We implemented:

### Locks
Condition variables
Channels
Virtual memory
We implemented some virtual memory upgrades:

### TLB
Demand Loading
Swap
Page Replacement Policy: LRU, FIFO
Scheduler
Multilevel priority queue

### Filesystem
Nachos provides a very limited file system and we build around that a lot of nice features:
Larger and extensible files (up to the disk space)
Directory hierarchy
### Userland
Some of the programs that you can run as an user are:

ls
cd
touch
cp
mkdir
rm
rmdir

# Install

**Dependencies**

(Debian based)

`g++ cpp gcc g++-11 gcc-11 cmake make gcc-mipsel-linux-gnu`

**Compiling**

Run `make` in the repository path.

# Using nachOS

## Testing

For all this tests you can give a random seed to the clock for 'random' (but repeatable) interrupts to occur with the `rs` flag, for example:

`$ threads/nachos -rs 1000`

`$ filesys/nachos -rs 829`

### Threads

Run:

`threads/nachos -tt`

### Filesys

Run:

`filesys/nachos -f` to format the disk.

`filesys/nachos -tf` performance test for the filesystem:

- Create a file, open it, write a bunch of chunks of bytes, read a bunch of chunks of bytes, close the file and remove the file.

`filesys/nachos -tfs` concurrent performance test for the filesystem:

- Create, write, read, close and remove but do it concurrently.

`filesys/nachos -tfc` create a lot of files to fit all the possible sectors in the disk.

`filesys/nachos -td` test the hierarchy of the file system creating a buch of directories and travelling through them. Simulates the Linux directory format.

## Executing nachOS

To execute nachOS run:

`filesys/nachos -f` to format the disk.

Then run the binary in filesys with the shell program:

`filesys/nachos -x userland/shell`

### Userland

To execute any of the following commands as a background process prepend the '&' character to:
ls
cd
touch
cp
mkdir
rm
rmdir
