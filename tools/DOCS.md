# NOTE
this directory contains some tools which are being deprecated, notably mkpart, wad_tool and wpart as a result of my effort to sunset support for wadfs and replace it with fat16.

# tools
this directory just contains some tools I wrote to make developing this less of a pain

## mkpart
mkpart is the util which makes the partition table for the image
usage: `
./build/mkpart <image file>
`
the partition table it writes to the file is hardcoded for now

## wad_tool
wad_tool is a tool for packing and unpacking wads
usage: `
./build/wad_tool pack <wad file> IWAD <input files>
./build/wad_tool unpack <wad file>
`

## wpart
wpart is a util that just dumps a given file to a partition in an mbr disk image
usage: `
./build/wpart <image file> <partition number> <input file>
`