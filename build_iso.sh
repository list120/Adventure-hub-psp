#!/bin/sh
# ==========================================================================
#  Build Lost Rune Adventure and pack it into a PSP .iso for PPSSPP.
#  Run this on a PC / Linux / macOS / WSL with the PSP SDK installed:
#
#      chmod +x build_iso.sh
#      ./build_iso.sh
#
#  Set SKIP_MAKE=1 to only pack the ISO from an existing build (used by CI).
#  Result:  Adventure.iso   (copy it to  /storage/emulated/0/Ppsspp rom )
# ==========================================================================
set -e

TARGET=adventure

if [ "${SKIP_MAKE:-0}" = "1" ]; then
    echo "[1/4] Skipping compile (SKIP_MAKE=1) - using existing build output."
else
    echo "[1/4] Compiling with the PSP SDK ..."
    make
fi

# Pick an ISO building tool.
ISO_TOOL=""
for t in genisoimage mkisofs xorriso; do
    if command -v "$t" >/dev/null 2>&1; then ISO_TOOL="$t"; break; fi
done
if [ -z "$ISO_TOOL" ]; then
    echo "ERROR: need genisoimage, mkisofs or xorriso to build the .iso."
    echo "       Your EBOOT.PBP is built and still usable in PSP/GAME/<folder>/."
    exit 1
fi

echo "[2/4] Finding the bootable executable ..."
BOOT=""
for f in EBOOT.BIN "$TARGET.prx" "$TARGET.elf"; do
    if [ -f "$f" ]; then BOOT="$f"; break; fi
done
if [ -z "$BOOT" ]; then
    echo "ERROR: could not find EBOOT.BIN / $TARGET.prx / $TARGET.elf after build."
    exit 1
fi
echo "      using $BOOT"

echo "[3/4] Laying out the PSP_GAME tree ..."
rm -rf iso
mkdir -p iso/PSP_GAME/SYSDIR
cp "$BOOT" iso/PSP_GAME/SYSDIR/EBOOT.BIN
if [ -f PARAM.SFO ]; then cp PARAM.SFO iso/PSP_GAME/PARAM.SFO; fi

echo "[4/4] Packing Adventure.iso ..."
case "$ISO_TOOL" in
    xorriso)
        xorriso -as mkisofs -quiet -V "ADVENTURE" -o Adventure.iso \
                -iso-level 3 -l iso ;;
    *)
        "$ISO_TOOL" -quiet -V "ADVENTURE" -o Adventure.iso -iso-level 3 -l iso ;;
esac

echo
echo "Done!  Adventure.iso  is ready."
echo "Copy it to:  /storage/emulated/0/Ppsspp rom"
