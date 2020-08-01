#!/bin/bash

TOP=$(readlink -f "$(dirname "$0")/../..")

# Sources for generation
: ${FIRMWARE_PACKAGE:="$TOP/platform/penutil/fw_package"}
: ${PLAT_GEN_DIR:="$TOP/platform/gen"}
: ${LINUX_PACKAGE:="$PLAT_GEN_DIR/penutil-linux"}

# Products generated
: ${GEN_DIR:="$PLAT_GEN_DIR/dsc-hpe-spp"}
: ${GEN_LINUX_DIR:="$GEN_DIR/Linux"}
: ${GEN_WIN_DIR:="$GEN_DIR/"}
: ${GEN_PKG:=${GEN_DIR}.tar.xz}
: ${FW_GEN_DIR:="$GEN_DIR/dsc_fw"}

# Always start clean
rm -fr "$GEN_DIR"
mkdir -p "$GEN_DIR"
mkdir -p "$FW_GEN_DIR"
mkdir -p "$GEN_LINUX_DIR"

#Copy NICFWData.xml
rsync -r --delete --delete-excluded --copy-links \
  --exclude="*.o" \
  "$FIRMWARE_PACKAGE/" "$FW_GEN_DIR/"

cp $TOP/nic/naples_fw.tar $FW_GEN_DIR/
cp $PLAT_GEN_DIR/penutil-windows.zip $GEN_DIR/

rsync -r --delete --delete-excluded --copy-links \
  --exclude="*.c" \
  --exclude="Makefile*" \
  --exclude="*.o" \
  "$LINUX_PACKAGE/" "$GEN_LINUX_DIR/"

# Generate tarball of the prepared package
cd "$GEN_DIR/.."
tar -cJ --exclude=.git -f "$GEN_PKG" "$(basename "$GEN_DIR")"
