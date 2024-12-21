#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

# later, when we call make to build the write app, we need to know 
# a basedire as e.g. "make busybox" changes the directory
# we cannot determin where this script is running (which should be the same directory)
# as this script is also used by the github-runner and there, the directory is completely different
# we need the dir on the local host!
${FINDER_APP_DIR}/..

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
echo "Make kernel"
    # TODO-done: build the kernel
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
# TODO-done (added by mh11) --> do this
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO-done: Create necessary base directories
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO-done:  Configure busybox
    make distclean 
    make defconfig
else
    cd busybox
fi

# TODO-done: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX="$OUTDIR"/rootfs ARCH={$ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a "$OUTDIR"/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a "$OUTDIR"/rootfs/bin/busybox | grep "Shared library"

# TODO-done: Add library dependencies to rootfs

cd ${FINDER_APP_DIR}/../arm64_libs

cp ./ld-linux-aarch64.so.1  ${OUTDIR}/rootfs/lib/ld-linux-aarch64.so.1
cp ./libm.so.6 ${OUTDIR}/rootfs/lib64/libm.so.6
cp ./libresolv.so.2 ${OUTDIR}/rootfs/lib64/libresolv.so.2
cp ./libc.so.6 ${OUTDIR}/rootfs/lib64/libc.so.6

# TODO-done: Make device nodes
# there are many options to create the device nodes (or not).
# i decided to create them, even if they already exist.
# so, I delete them first, if they exist.
rm -f ${OUTDIR}/dev/null
rm -f ${OUTDIR}/dev/ttyAMA0
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/ttyAMA0 c 5 1 

# TODO-done: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=$CROSS_COMPILE

# TODO-done: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
# from the step above, where already in out finder-app directory
cp ./finder.sh ${OUTDIR}/rootfs/home/.
cp ./finder-test.sh ${OUTDIR}/rootfs/home/.
cp ./writer  ${OUTDIR}/rootfs/home/.

#and the assignment related files
mkdir -p ${OUTDIR}/rootfs/home/conf
cp conf/username.txt ${OUTDIR}/rootfs/home/conf/username.txt
cp conf/assignment.txt ${OUTDIR}/rootfs/home/conf/assignment.txt
cp ./autorun-qemu.sh ${OUTDIR}/rootfs/home/autorun-qemu.sh

# TODO-done: Chown the root directory
sudo chown root -R ${OUTDIR}/rootfs
sudo chgrp root -R ${OUTDIR}/rootfs

# TODO-done: Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio

