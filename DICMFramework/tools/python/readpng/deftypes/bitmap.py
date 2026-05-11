import png
import sys
import os
import re


class Bitmap:
    def __init__(self, filename, rotation, prefix = ''):
        self.filename = filename

        # Use the filename without extension as the C identifier.
        self.name = prefix + os.path.splitext(os.path.basename(filename))[0].lower()
        self.cname = 'bm_' + re.sub('[^A-Za-z0-9]', '_', os.path.splitext(filename)[0].lower())

        # Read image data.
        reader = png.Reader(filename = filename)
        data = reader.read_flat()

        # Check image format.
        if(data[3]['bitdepth'] > 8):
            print("ERROR: Bit depths above 8 bits are not supported.")
            sys.exit(1)
        if(data[3]['greyscale'] != False):
            print("ERROR: Greyscale bitmaps are not supported.")
            sys.exit(1)
        if(data[3]['alpha'] != False):
            print("ERROR: Alpha channels are not supported.")
            sys.exit(1)

        # Store image data.
        self.width = data[0]
        self.height = data[1]
        self.pixel_data = data[2]
        self.palette_data = data[3]['palette']

        # Rotate image data.
        if rotation == '0deg':
            None
        elif rotation == '90deg':
            self._rotate_90()
        elif rotation == '180deg':
            self._rotate_180()
        elif rotation == '270deg':
            self._rotate_270()
        else:
            print("WARNING: No valid rotation specified.")

        # Fix transparency index and compress image data.
        size_o = len(self.pixel_data)
        self._fix_trans(self.pixel_data, self.palette_data)
        self.pixel_data = compress_rle(self.pixel_data)
        size_c = len(self.pixel_data) * 2

        # Encode and store palette data.
        for p in range(0, len(self.palette_data)):
            self.palette_data[p] = palette_to_rgb565(self.palette_data[p])

        # Pad palette to 256 colors. (TODO: Waste of space?)
        #if len(self.palette_data) < 256:
        #   self.palette_data = self.palette_data + [0] * (256 - len(self.palette_data))

        # Print statistics
        print("Read bitmap: %s [%s] %d to %d bytes (%.1f%%)"%(self.name, self.cname, size_o, size_c, 100 * size_c / size_o))


    def _rotate_90(self):
        rotated = []

        for x in range(0, self.width):
            for y in range(0, self.height):
                rotated.append(self.pixel_data[(y * self.width) + (self.width - 1 - x)])

        self.pixel_data = rotated
        self.width, self.height = self.height, self.width


    def _rotate_180(self):
        self.pixel_data.reverse()


    def _rotate_270(self):
        self._rotate_180()
        self._rotate_90()


    def _fix_trans(self, pixels, palette):
        transparent = None

        # Find transparent palette entry.
        for i in range(0, len(palette)):
            if palette[i][0:3] == (255, 0, 255):
                transparent = i
                break

        if transparent is not None and transparent > 0:
            # Move transparent entry to index 0 by swapping places.
            palette[0], palette[transparent] = palette[transparent], palette[0]

            # Also swap places within pixel data.
            for i in range(0, len(pixels)):
                if pixels[i] == 0:
                    pixels[i] = transparent
                elif pixels[i] == transparent:
                    pixels[i] = 0


def palette_to_rgb565(rgb):
    # Encodes 8-bit RGB into byte swapped RGB565 representation:
    # +--+--+--+--+--+--+--+--+  +--+--+--+--+--+--+--+--+  +--+--+--+--+--+--+--+--+
    # |R7|R6|R5|R4|R3|R2|R1|R0|  |G7|G6|G5|G4|G3|G2|G1|G0|  |B7|B6|B5|B4|B3|B2|B1|B0|
    # +--+--+--+--+--+--+--+--+  +--+--+--+--+--+--+--+--+  +--+--+--+--+--+--+--+--+
    #
    #                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    #                |G4|G3|G2|B7|B6|B5|B4|B3|R7|R6|R5|R4|R3|G7|G6|G5| <- RGB565
    #                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    r = (rgb[0])                         & 0x00f8
    g = ((rgb[1] << 12) | (rgb[1] >> 5)) & 0xe007
    b = (rgb[2] << 5)                    & 0x1f00

    return (r | g | b)


def compress_rle(original):
    compressed = []
    value = original[0]
    count = 0

    for p in range(1, len(original)):
        if original[p] != value:
            # Different value. Write symbol.
            compressed.append(count << 8 | value)
            value = original[p]
            count = 0
        elif count >= 255:
            # Maximum count. Write symbol.
            compressed.append(count << 8 | value)
            count = 0
        else:
            # Same as last value.
            count = count + 1;

    # Write final symbol.
    compressed.append(count << 8 | value)

    return compressed

