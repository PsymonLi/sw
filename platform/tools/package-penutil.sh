#!/bin/bash

TOP=$(readlink -f "$(dirname "$0")/../..")

# Sources for generation
: ${PENUTIL_DIR:="$TOP/platform/penutil"}
: ${FIRMWARE_PACKAGE:="$PENUTIL_DIR/fw_package"}
: ${PLAT_GEN_DIR:="$TOP/platform/gen"}
: ${LINUX_PACKAGE:="$PLAT_GEN_DIR/penutil-linux"}
: ${ESXI65_PACKAGE:="$PLAT_GEN_DIR/esxi_rel_drop_6.5"}
: ${ESXI67_PACKAGE:="$PLAT_GEN_DIR/esxi_rel_drop_6.7"}
: ${ESXI70_PACKAGE:="$PLAT_GEN_DIR/esxi_rel_drop_7.0"}

if [ -z "$1"]; then
	VERSION=`git describe --abbrev=0 --tags`
	echo VERSION from git: $VERSION
else
	VERSION=$1
	echo VERSION provided: $VERSION
fi

# Products generated
: ${GEN_DIR:="$PLAT_GEN_DIR/dsc-hpe-spp"}
: ${GEN_SOURCE_DIR:="$GEN_DIR/source"}
: ${GEN_LINUX_DIR:="$GEN_DIR/Linux"}
: ${GEN_WIN_DIR:="$GEN_DIR/"}
: ${GEN_PKG:=${GEN_DIR}.tar.xz}
: ${FW_GEN_DIR:="$GEN_DIR/dsc_fw"}

# Always start clean
rm -fr "$GEN_DIR"
mkdir -p "$GEN_DIR"
mkdir -p "$FW_GEN_DIR"
mkdir -p "$GEN_SOURCE_DIR"
mkdir -p "$GEN_LINUX_DIR"

#Copy NICFWData.xml
#rsync -r --delete --delete-excluded --copy-links \
#  --exclude="*.o" \
#  "$FIRMWARE_PACKAGE/" "$FW_GEN_DIR/"
#echo Creating $FW_GEN_DIR/NICFWData.xml for VERSION: $VERSION
#
#LD_LIBRARY_PATH=$LINUX_PACKAGE $LINUX_PACKAGE/penutil -c $VERSION -p $FW_GEN_DIR

if [ -f "$TOP/nic/dsc_fw_${VERSION}.tar" ]; then
	cp $TOP/nic/dsc_fw_${VERSION}.tar $FW_GEN_DIR/
fi

if [ -f "$PLAT_GEN_DIR/penutil-windows.zip" ]; then
	cp $PLAT_GEN_DIR/penutil-windows.zip $GEN_DIR/
fi

cp $PLAT_GEN_DIR/penutil-windows.zip $GEN_DIR/
cp $ESXI65_PACKAGE/*penutil*.* $GEN_DIR/
cp $ESXI67_PACKAGE/*penutil*.* $GEN_DIR/
cp $ESXI70_PACKAGE/*penutil*.* $GEN_DIR/

rsync -r --delete --delete-excluded --copy-links \
  --exclude="*.o" \
  --exclude="*.so" \
  "$PENUTIL_DIR/" "$GEN_SOURCE_DIR/"

rsync -r --delete --delete-excluded --copy-links \
  --exclude="*.c" \
  --exclude="Makefile*" \
  --exclude="*.o" \
  "$LINUX_PACKAGE/" "$GEN_LINUX_DIR/"

# Generate tarball of the prepared package
cd "$GEN_DIR/.."
tar -cJ --exclude=.git -f "$GEN_PKG" "$(basename "$GEN_DIR")"
