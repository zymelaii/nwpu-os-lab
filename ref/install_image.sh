#!bash
# DO NOT run this script FROM SHELL MANUALLY
MBR_BIN=$1
BOOT_BIN=$2
LOADER_BIN=$3
KERN_BIN=$4
IMAGE=$5
MOUNTPOINT="obj/testmnt"
dd if=$MBR_BIN of=$IMAGE bs=1 count=446 conv=notrunc
loop_device=`sudo losetup -f` # find a vacant slot for loop dev
sudo losetup -P $loop_device $IMAGE
sudo dd if=$BOOT_BIN of=${loop_device}p2 bs=1 count=448 seek=62 conv=notrunc
sudo dd if=$BOOT_BIN of=${loop_device}p4 bs=1 count=448 seek=62 conv=notrunc
mkdir -pv $MOUNTPOINT
ls ${MOUNTPOINT} | xargs rm -f
sudo mount ${loop_device}p2 $MOUNTPOINT
sudo cp -vf $LOADER_BIN ${MOUNTPOINT}/
sudo cp -vf $KERN_BIN ${MOUNTPOINT}/
sudo umount $MOUNTPOINT
sudo mount ${loop_device}p4 $MOUNTPOINT
sudo cp -vf $LOADER_BIN ${MOUNTPOINT}/
sudo cp -vf $KERN_BIN ${MOUNTPOINT}/
sudo umount $MOUNTPOINT
sudo losetup -d $loop_device
