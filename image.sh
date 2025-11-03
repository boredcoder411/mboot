#!/bin/bash
set -e

DISK_IMG="image.img"
BOOT_FILE="boot.bin"
DISK_SIZE_MB=64
SECTOR_SIZE=512
VOL_LABEL="DATA"

FILES_TO_ADD=("$@")

BOOT_SIZE_BYTES=$(stat -c%s "$BOOT_FILE" 2>/dev/null || stat -f%z "$BOOT_FILE")
BOOT_SECTORS=$(( (BOOT_SIZE_BYTES + SECTOR_SIZE - 1) / SECTOR_SIZE ))
DISK_SECTORS=$(( (DISK_SIZE_MB * 1024 * 1024) / SECTOR_SIZE ))

echo "Boot.bin: $BOOT_SIZE_BYTES bytes = $BOOT_SECTORS sectors"
echo "Disk size: $DISK_SIZE_MB MB = $DISK_SECTORS sectors"

echo "Creating empty disk image..."
dd if=/dev/zero of="$DISK_IMG" bs=1M count="$DISK_SIZE_MB" status=progress

echo "Writing boot.bin to first partition..."
dd if="$BOOT_FILE" of="$DISK_IMG" bs=$SECTOR_SIZE conv=notrunc status=progress

byte() { printf "\\$(printf '%03o' "$1")"; }

le32() {
  local n=$1
  byte $(( n        & 0xFF ))
  byte $(( (n>>8)   & 0xFF ))
  byte $(( (n>>16)  & 0xFF ))
  byte $(( (n>>24)  & 0xFF ))
}

P1_START=1
P1_SIZE=$BOOT_SECTORS
P2_START=$(( P1_START + P1_SIZE ))
P2_SIZE=$(( DISK_SECTORS - P2_START ))

write_partition() {
  local OFFSET=$1 START_LBA=$2 SIZE=$3 TYPE=$4

  {
    byte 0x00
    byte 0xFE; byte 0xFF; byte 0xFF
    byte $TYPE
    byte 0xFE; byte 0xFF; byte 0xFF
    le32 $START_LBA
    le32 $SIZE
  } | dd of="$DISK_IMG" bs=1 seek=$OFFSET conv=notrunc status=none
}

echo "Creating MBR partition table..."
write_partition $((0x1BE)) $P1_START $P1_SIZE 0x00
write_partition $((0x1CE)) $P2_START $P2_SIZE 0x06
printf '\x55\xAA' | dd of="$DISK_IMG" bs=1 seek=510 conv=notrunc status=none

echo "Creating FAT16 filesystem on second partition..."
mkfs.fat -F16 -n "$VOL_LABEL" --offset $P2_START "$DISK_IMG"

if [ ${#FILES_TO_ADD[@]} -gt 0 ]; then
  echo "Adding files to FAT16 partition using mtools..."

  OFFSET_BYTES=$((P2_START * SECTOR_SIZE))
  export MTOOLSRC=$(mktemp)
  echo "drive z: file=\"$DISK_IMG\" offset=$OFFSET_BYTES" > "$MTOOLSRC"

  for f in "${FILES_TO_ADD[@]}"; do
    if [ -f "$f" ]; then
      echo "Copying $f..."
      mcopy -i "$DISK_IMG@@$OFFSET_BYTES" "$f" ::/
    else
      echo "Warning: file not found: $f"
    fi
  done

  rm -f "$MTOOLSRC"
  echo "Files added successfully!"
else
  echo "No files specified to add."
fi

echo "Disk image $DISK_IMG created successfully!"
echo " - Partition 1: boot.bin ($BOOT_SECTORS sectors)"
echo " - Partition 2: FAT16 ($P2_SIZE sectors)"

