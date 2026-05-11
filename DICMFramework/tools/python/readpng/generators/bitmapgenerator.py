import os
import re
import sys
from deftypes.bitmap import Bitmap


class BitmapGenerator:
    """
    Reads bitmaps from specified directories and builds data structures for size
    and compressed pixel data for each bitmap, fonts stored in the font
    directory, and unique palettes from all bitmaps.

    Example
    -------
    from bitmapgenerator import BitmapGenerator

    # Load all bitmap data
    bg = BitmapGenerator('bitmaps', 'fonts', '0deg')

    # All bitmap references:
    print(bg.bitmaps.keys())

    # Bitmaps are referenced by their file names, without extension. To fetch
    # the C definition name for the file "MySprite.png":
    print(bg.bitmaps['mysprite'].cname)

    # Fetching the palette used by a bitmap:
    print(bg.bitmaps['mysprite'].palette_cname)

    # Fonts are named by the directory they are located in.
    print(bg.fonts.keys())

    # Generate data files
    bg.write_bitmap_data('bitmap_data.h', 'bitmap_data.c')
    """


    def __init__(self, bitmap_path, font_path, rotation):
        self.bitmaps = {}
        self.fonts = {}
        self.palettes = []
        self.rotation = rotation

        self._read_bitmaps(bitmap_path)
        self._read_fonts(font_path)

    def _read_bitmap(self, file, prefix = ''):
        # Read bitmap data.
        bmp = Bitmap(file, self.rotation, prefix)

        # Make sure that there are no duplicates.
        if bmp.cname in self.bitmaps:
            print("ERROR: Duplicate bitmap identifier " + bmp.cname + ".")
            sys.exit(1)

        # Search for an identical palette in the palette list.
        palette_index = None
        for p in range(0, len(self.palettes)):
            if self.palettes[p] == bmp.palette_data:
                palette_index = p

        # If the palette was not found, add it to the list.
        if palette_index == None:
            palette_index = len(self.palettes)
            self.palettes.append(bmp.palette_data)

        # Add the selected palette name to the bitmap.
        bmp.palette_cname = 'palette_' + str(palette_index)

        # Add the bitmap.
        self.bitmaps[bmp.name] = bmp

        return bmp


    def _read_bitmaps(self, path):
        # Scan all files in given directory.
        for file in os.listdir(path):
            name = os.path.splitext(file)
            file = path + '/' + file

            # Attempt to add all regular files ending with '.png'.
            if os.path.isfile(file) and name[1].lower() == '.png':
                self._read_bitmap(file)


    def _read_font(self, prefix, path):
        if prefix in self.fonts:
            print("ERROR: Duplicate font identifier " + prefix + ".")
            sys.exit(1)

        font = Font(prefix)

        # Process and store all bitmaps.
        for file in os.listdir(path):
            name = os.path.splitext(file)
            file = path + '/' + file

            if os.path.isfile(file) and name[1].lower() == '.png':
                # Fetch bitmap data.
                bmp = self._read_bitmap(file, prefix = prefix + '_')

                # The name of the file is the ASCII code.
                idx = int(name[0])
                if not font.set_glyph(idx, bmp):
                    print("ERROR: Glyph index out of range.")
                    sys.exit(1)

        # Store glyph names.
        self.fonts[font.name] = font


    def _read_fonts(self, path):
        # Scan all files in given directory.
        for file in os.listdir(path):
            name = os.path.splitext(file)
            file = path + '/' + file

            # Each directory should contain a set of glyphs for a single font.
            if os.path.isdir(file):
                self._read_font(name[0], file)


    def write_bitmap_data(self, header_filename, source_filename):
        # Write header file
        f = open(header_filename, 'w')
        guarddef = re.sub('[^A-Za-z0-9]', '_', os.path.basename(header_filename).upper()) + '_'

        f.write('/*! \\file ' + os.path.basename(header_filename) + '\n')
        f.write(' *  \\brief Bitmap data header. This file is machine generated - do not edit!\n')
        f.write(' */\n')
        f.write('\n')
        f.write('#ifndef ' + guarddef + '\n')
        f.write('#define ' + guarddef + '\n')
        f.write('\n')
        f.write('/** Includes ******************************************************************/\n')
        f.write('#include <stdint.h>\n')
        f.write('\n')
        f.write('/** Defines *******************************************************************/\n')
        f.write('\n')
        f.write('/** Typedefs ******************************************************************/\n')
        f.write('\n')
        f.write('//! Bitmap data struct\n')
        f.write('typedef struct BITMAP_DEF\n')
        f.write('{\n')
        f.write('\tconst uint8_t x_size;               //!< Width in pixels.\n')
        f.write('\tconst uint8_t y_size;               //!< Height in pixels.\n')
        f.write('\tconst uint16_t *pixel_data;         //!< Compressed pixel data.\n')
        f.write('} BITMAP_DEF;\n')
        f.write('\n')
        f.write('//! Font data struct\n')
        f.write('typedef struct FONT_DEF\n')
        f.write('{\n')
        f.write('\tconst BITMAP_DEF * const *glyph;    //!< List of glyph sprites.\n')
        f.write('} FONT_DEF;\n')
        f.write('\n')
        f.write('//! Palette data struct\n')
        f.write('typedef struct PALETTE_DEF\n')
        f.write('{\n')
        f.write('\tconst uint16_t *color_data;         //!< Precalculated palette data.\n')
        f.write('} PALETTE_DEF;\n')
        f.write('\n')
        
        f.write('/** Variables *****************************************************************/\n')
        f.write('\n')

        for key in self.bitmaps.keys():
            f.write('extern const BITMAP_DEF ' + self.bitmaps[key].cname + ';\n')

        f.write('\n')

        for p in range(0, len(self.palettes)):
            f.write('extern const PALETTE_DEF palette_' + str(p) + ';\n')

        f.write('\n')

        for key in self.fonts.keys():
            f.write('extern const FONT_DEF font_' + key + ';\n')

        f.write('\n')
        f.write('#endif /* ' + guarddef + ' */\n')

        f.close()

        # Write source file
        f = open(source_filename, 'w')
        f.write("/* clang-format off */" + "\n")
        f.write('/*! \\file ' + os.path.basename(source_filename) + '\n')
        f.write(' *  \\brief Bitmap data definitions. This file is machine generated - do not edit!\n')
        f.write(' */\n')
        f.write('\n')
        f.write('/** Includes ******************************************************************/\n')
        f.write('#include "extflash.h"\n')
        f.write('\n')
        f.write('#include "' + os.path.basename(header_filename) + '"\n')
        f.write('\n')
        f.write('/** Variables *****************************************************************/\n')
        f.write('\n')

        for key in self.bitmaps.keys():
            bmp = self.bitmaps[key]
            data = bmp.cname + '_data'
            f.write('static const EXTFLASH uint16_t ' + data + '[] =\n{\n\t' + format_16bit(bmp.pixel_data) + '\n};\n\n')

        for p in range(0, len(self.palettes)):
            f.write('static const EXTFLASH uint16_t palette_' + str(p) + '_data[] =\n{\n\t' + format_16bit(self.palettes[p]) + '\n};\n\n')

        for key in self.fonts.keys():
            font = self.fonts[key]
            f.write('static const EXTFLASH BITMAP_DEF * const ' + font.cname + '_data[] =\n{\n')
            for i in range(0, len(font.glyphs)):
                glyph = font.glyphs[i]
                if glyph != None:
                    f.write('\t[' + str(i) + '] = &' + glyph.cname + ',\n')
            f.write('};\n\n')

        for key in self.bitmaps.keys():
            bmp = self.bitmaps[key]
            data = bmp.cname + '_data'
            f.write(f'const EXTFLASH BITMAP_DEF {bmp.cname} = {{ .x_size = {bmp.width}, .y_size = {bmp.height}, .pixel_data = {data} }};\n')

        f.write('\n')

        for p in range(0, len(self.palettes)):
            key = 'palette_' + str(p)
            f.write(f'const EXTFLASH PALETTE_DEF {key} = {{ .color_data = {key}_data }};\n')

        f.write('\n')

        for key in self.fonts.keys():
            f.write(f'const EXTFLASH FONT_DEF font_{key} = {{ .glyph = font_{key}_data }};\n')

        f.close()


def format_16bit(items):
    wrap = 0
    formatted = ''

    for i in items:
        formatted += '0x%04x,' % i
        wrap += 1
        if wrap == 8:
            wrap = 0
            formatted += '\n\t'
        else:
            formatted += ' '

    return formatted.strip()


class Font:
    """
    A font is a simple list of bitmaps, one bitmap for each glyph. it has the
    following attributes:

    name:               The same name that the font was initialized with.
    cname:              A C-style identifier created from its name.
    glyphs:             Bitmaps for all glyphs. Each position represents an ASCII code.
    max_glyph_width:    The largest 'width' attribute of any added bitmap.
    max_glyph_height:   The largest 'height' attribute of any added bitmap.
    """


    def __init__(self, name):
        self.name = name.lower()
        self.cname = 'font_' + re.sub('[^A-Za-z0-9]', '_', name).lower()
        self.glyphs = [None] * 256
        self.max_glyph_width = 0
        self.max_glyph_height = 0


    def set_glyph(self, ascii_code, glyph_bitmap):
        added = False

        if ascii_code >= 0 and ascii_code <= 255:
            self.glyphs[ascii_code] = glyph_bitmap

            if self.max_glyph_width < glyph_bitmap.width:
                self.max_glyph_width = glyph_bitmap.width
            if self.max_glyph_height < glyph_bitmap.height:
                self.max_glyph_height = glyph_bitmap.height

            added = True

        return added
