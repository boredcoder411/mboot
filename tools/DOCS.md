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

## psf
psf is a util to take a png and make it into a .psf file, which is the font format mboot uses
usage: `
./build/psf <image file>
`
the input image file needs to be in png format, and psf assumes each glyph is 8x8 and the image file has 16x8 glyphs
to build psf you need libpng to be installed

## wpart
wpart is a util that just dumps a given file to a partition in an mbr disk image
usage: `
./build/wpart <image file> <partition number> <input file>
`

## imf
imf is the tool to turn pngs into mboot's image format: imf. (its structure is defined in `stage2/imf.h`)
