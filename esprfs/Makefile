#
# Makefile for the linux ramfs routines.
#

obj-m += esprfs.o

file-mmu-y := file-mmu.o


esprfs-objs += inode.o $(file-mmu-y) kcompressor.o


all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

