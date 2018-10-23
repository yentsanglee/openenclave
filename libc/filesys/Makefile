
FSNAME=myext2
BLOCKS=20000
LOOP=/dev/loop0

all: create mount populate unmount

create: $(FSNAME)

$(FSNAME):
	dd if=/dev/zero of=$(FSNAME) bs=512 count=$(BLOCKS)
	losetup $(LOOP) $(FSNAME)
	mkfs.ext2 $(LOOP)
	losetup -d $(LOOP)

mount: create
	losetup $(LOOP) $(FSNAME)
	mkdir -p /mnt/$(FSNAME)
	mount $(LOOP) /mnt/$(FSNAME)

populate:
	mkdir -p /mnt/$(FSNAME)/a/b/c
	cp Makefile /mnt/$(FSNAME)/a/b/c

unmount:
	umount /mnt/$(FSNAME)
	losetup -d $(LOOP)
