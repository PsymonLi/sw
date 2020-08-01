
#!/bin/bash

#
# Compile HPE SPP penutil on Windows.
#

set -x

# Where am I
TOP=$(readlink -f "$(dirname "$0")/../..")

# Sources for generation
: ${TEMP_BUILDDIR:="penutil-$(uuidgen)"}
: ${DRIVERS_SRC:="$TOP/platform/penutil"}
: ${COMMONINC_SRC:="$TOP/platform/drivers/windows/CommonInc"}
: ${REMOTE_DRIVERS_SRC:="C:/Builds/$TEMP_BUILDDIR/platform"}
: ${REMOTE_DRIVERS_SRCSTR:="\"$REMOTE_DRIVERS_SRC"\"}
: ${REMOTE_COMMONINC_SRC:="C:/Builds/$TEMP_BUILDDIR/platform/drivers/windows"}
: ${REMOTE_COMMONINC_SRCSTR:="\"$REMOTE_COMMONINC_SRC"\"}
: ${REMOTE_DRIVERS_BUILDSCRIPT:="$REMOTE_DRIVERS_SRC/penutil/PenUtilBuild.ps1"}
: ${REMOTE_DRIVERS_SOLUTION:="$REMOTE_DRIVERS_SRC/penutil/penutil_win.sln"}
: ${REMOTE_DRIVERS_SOLSTR:="\"$REMOTE_DRIVERS_SOLUTION"\"}
: ${BUILD_VM_NAME:="192.168.66.35"}
: ${BUILD_VM_USER:="Administrator"}
: ${BUILD_VM_PW:="pen123!"}

# Products generated
: ${GEN_DIR:="$TOP/platform/gen/penutil-windows"}
: ${GEN_PKG:="$GEN_DIR.zip"}
: ${GEN_REMOTE_ARTIFACTS:="$REMOTE_DRIVERS_SRC/penutil/ArtifactsZipped"}
: ${GEN_REMOTE_BUILDLOGS:="$REMOTE_DRIVERS_SRC/penutil/BuildLogs"}
: ${GEN_REMOTE_ARTIFACTS_STR:="\"$GEN_REMOTE_ARTIFACTS"\"}
: ${GEN_REMOTE_BUILDLOGS_STR:="\"$GEN_REMOTE_BUILDLOGS"\"}

# Always start clean
rm -rf "$GEN_DIR"
mkdir -p "$GEN_DIR"
rm -f "$GEN_PKG_UNSIGNED"


# Set version string
if [ -n "$SW_VERSION" ] ; then
	VER=$SW_VERSION
else
	VER=`git describe --tags`
fi

# Remove remote build source folder in case it exists (probably never since its name is based on a uuid).
sshpass -p $BUILD_VM_PW ssh -o StrictHostKeyChecking=no "$BUILD_VM_USER"@"$BUILD_VM_NAME" "rmdir " "$REMOTE_DRIVERS_SRCSTR" "/s /q"

# Copy source code to the remote build source folder.
sshpass -p $BUILD_VM_PW ssh -o StrictHostKeyChecking=no "$BUILD_VM_USER"@"$BUILD_VM_NAME" "mkdir " "$REMOTE_DRIVERS_SRCSTR" 
sshpass -p $BUILD_VM_PW scp -o StrictHostKeyChecking=no -pr "$DRIVERS_SRC" "$BUILD_VM_USER"@"$BUILD_VM_NAME":"$REMOTE_DRIVERS_SRC"
sshpass -p $BUILD_VM_PW ssh -o StrictHostKeyChecking=no "$BUILD_VM_USER"@"$BUILD_VM_NAME" "mkdir " "$REMOTE_COMMONINC_SRCSTR" 
sshpass -p $BUILD_VM_PW scp -o StrictHostKeyChecking=no -pr "$COMMONINC_SRC" "$BUILD_VM_USER"@"$BUILD_VM_NAME":"$REMOTE_COMMONINC_SRC"

# Start remote Build script
if [ -z "$VER" ] ; then
	sshpass -p $BUILD_VM_PW ssh -o StrictHostKeyChecking=no "$BUILD_VM_USER"@"$BUILD_VM_NAME" "powershell -f" "$REMOTE_DRIVERS_BUILDSCRIPT" "-SolutionNameAndPath" "$REMOTE_DRIVERS_SOLSTR"
else
	sshpass -p $BUILD_VM_PW ssh -o StrictHostKeyChecking=no "$BUILD_VM_USER"@"$BUILD_VM_NAME" "powershell -f" "$REMOTE_DRIVERS_BUILDSCRIPT" "-SolutionNameAndPath" "$REMOTE_DRIVERS_SOLSTR" "-VerStr " "$VER"
fi

if [ 0 -eq $? ] ; then
# Copy remote build artifacts
    sshpass -p $BUILD_VM_PW scp -o StrictHostKeyChecking=no -r "$BUILD_VM_USER"@"$BUILD_VM_NAME":"$GEN_REMOTE_ARTIFACTS_STR" "$GEN_DIR"
fi

# Copy remote build logs
sshpass -p $BUILD_VM_PW scp -o StrictHostKeyChecking=no -pr "$BUILD_VM_USER"@"$BUILD_VM_NAME":"$GEN_REMOTE_BUILDLOGS_STR" "$GEN_DIR"

# Copy package from the ArtifactsZipped folder
cd "$GEN_DIR/.."
cp "$GEN_DIR/ArtifactsZipped/Artifacts.zip" .
mv -f Artifacts.zip "$GEN_PKG"
