#!/bin/sh
# ==========================================================================
#  put_iso_on_phone.sh
#  Copies a downloaded Adventure.iso into the PPSSPP rom folder.
#
#  Run it wherever the .iso ended up:
#    - In Termux on the phone:      sh put_iso_on_phone.sh ~/storage/downloads/Adventure.iso
#    - On a PC with the phone on USB: sh put_iso_on_phone.sh /path/to/Adventure.iso
#
#  The simplest way of all, though, is to just use your phone's Files app:
#  move Adventure.iso into  /storage/emulated/0/Ppsspp rom  .
# ==========================================================================
set -e

ROM="/storage/emulated/0/Ppsspp rom"
SRC="${1:-Adventure.iso}"

if [ ! -f "$SRC" ]; then
    echo "Could not find the ISO at: $SRC"
    echo "Usage: sh put_iso_on_phone.sh /path/to/Adventure.iso"
    exit 1
fi

if [ ! -d "$ROM" ]; then
    echo "PPSSPP rom folder not found at: $ROM"
    echo "Open PPSSPP once so it creates its folders, then try again."
    exit 1
fi

cp "$SRC" "$ROM/Adventure.iso"
echo "Done.  Adventure.iso is now in:"
echo "  $ROM"
echo "Open PPSSPP and launch ADVENTURE."
