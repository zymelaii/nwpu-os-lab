# first create a disk of roughly 10MB
IMAGE=test.img
dd if=/dev/zero of=${IMAGE} bs=512 count=30720
sfdisk ${IMAGE} < part1.sfdisk
# use losetup to virtualize the image file as an block device
loop_device=`sudo losetup -f` # find a vacant loop device slot
sudo losetup -P ${loop_device} ${IMAGE}
# seems that there is no automatic approach to format the disks by their type field, so manually do this
# to make students' life easier, still use FAT12 as primary working partition
sudo mkfs.vfat -F 12 ${loop_device}p1
sudo mkfs.vfat -F 12 -s 1 -r 224 -M 0xF0 -h 0 -D 0x80 ${loop_device}p2
sudo mkfs.vfat -F 12 -s 1 -r 224 -M 0xF0 -h 0 -D 0x80 ${loop_device}p4
sudo mkfs.ext2 ${loop_device}p5
sudo mkfs.minix ${loop_device}p6
sudo mkfs.vfat -F 12 ${loop_device}p7
# ! NOTE: partition 2&4 are for this lab, others are for extra lab/for fun

# remember to release the device
sudo losetup -d ${loop_device}
