#!/bin/bash

TOP=$(readlink -f "$(dirname "$0")/../..")

# Sources for generation
: ${PENUTIL_DIR:="$TOP/platform/penutil"}
: ${FIRMWARE_PACKAGE:="$PENUTIL_DIR/fw_package"}
: ${PLAT_GEN_DIR:="$TOP/platform/gen"}
: ${LINUX_PACKAGE:="$PLAT_GEN_DIR/penutil-linux"}
: ${ESXI67_PACKAGE:="$PLAT_GEN_DIR/esxi_rel_drop_6.7"}
: ${ESXI70_PACKAGE:="$PLAT_GEN_DIR/esxi_rel_drop_7.0"}

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
rsync -r --delete --delete-excluded --copy-links \
  --exclude="*.o" \
  "$FIRMWARE_PACKAGE/" "$FW_GEN_DIR/"

cp $TOP/nic/naples_fw.tar $FW_GEN_DIR/
cp $PLAT_GEN_DIR/penutil-windows.zip $GEN_DIR/
mv $ESXI67_PACKAGE/*penutil*.* $GEN_DIR/
mv $ESXI70_PACKAGE/*penutil*.* $GEN_DIR/

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
