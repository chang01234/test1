#! /usr/bin/env python3

"""usage readpng.py defdir rotation [outdir]

Generates data files required by HMI engine based on definitions found in defdir.

Rotation of display on HMI hardware must be specified. Valid values are
0deg, 90deg, 180deg, and 270deg. These specify the clockwise rotation of all
graphic elements in degrees.

Specifying outdir is optional. If provided .c files will be written to this
directory and .h files will be written to directory outdir/Inc. If outdir is
not provided Readpng will attempt to find the default fc_data directory.

"""

import os
import sys

# Check that we have all required settings
import hmiversion

# Remove all printouts if set
QUIET = os.environ.get("QUIET", False)
if QUIET:
   sys.stdout=open(os.devnull, "w")

# TODO: Make all generators check that all expected numbers in provided definition files are actually numbers. And provide a more useful error message if they are not.
# TODO: More thourougly sanitize all c names of disallowed characters.
from generators.bitmapgenerator import BitmapGenerator
from generators.screengenerator import ScreenGenerator
from generators.variablegenerator import VariableGenerator
from generators.menugenerator import MenuGenerator
from generators.eventgenerator import EventGenerator
from lib.misc import write_data_interface

try:
    import png
except ImportError:
    print("Could not find pypng. See readme for installation instructions.")
    sys.exit(1)

if len(sys.argv) < 3:
    print(__doc__)
    sys.exit(2)

if os.path.isdir(sys.argv[1]):
    defdir = sys.argv[1]
else:
    print("Could not find defdir.")
    sys.exit(2)

if sys.argv[2] == "0deg":
    rotation = sys.argv[2]
    draw_direction = "DRAW_DIRECTION_UP"
elif sys.argv[2] == "90deg":
    rotation = sys.argv[2]
    draw_direction = "DRAW_DIRECTION_LEFT"
elif sys.argv[2] == "180deg":
    rotation = sys.argv[2]
    draw_direction = "DRAW_DIRECTION_DOWN"
elif sys.argv[2] == "270deg":
    rotation = sys.argv[2]
    draw_direction = "DRAW_DIRECTION_RIGHT"
else:
    print("Unknown rotation " + sys.argv[2] + ".")
    sys.exit(2)

if len(sys.argv) == 3:
    if os.path.isdir("../engine/fc_data"):
        outdir = "../engine/fc_data"
    else:
        print("Could not find default fc_data directory.")
        sys.exit(1)
else:
    outdir = sys.argv[3]
    if not os.path.isdir(sys.argv[3]):
        os.mkdir(outdir)
        os.mkdir(os.path.join(outdir, "inc"))


vg = VariableGenerator(os.path.join(defdir, "vars.txt"))
eg = EventGenerator(os.path.join(defdir, "events.txt"), vg.variables)
bg = BitmapGenerator(os.path.join(defdir, "bitmaps"), os.path.join(defdir, "fonts"), rotation)
sg = ScreenGenerator(os.path.join(defdir, "screens"), rotation, bg.bitmaps, bg.fonts, vg.variables)
mg = MenuGenerator(os.path.join(defdir, "menus"), vg.variables, eg.events, sg.screens)

vg.write_variable_data(os.path.join(outdir, "inc/varstate_data.h"), os.path.join(outdir, "varstate_data.c"))
eg.write_event_data(os.path.join(outdir, "inc/event_data.h"), os.path.join(outdir, "event_data.c"))
sg.write_screen_data(os.path.join(outdir, "inc/screen_data.h"), os.path.join(outdir, "screen_data.c"))
bg.write_bitmap_data(os.path.join(outdir, "inc/bitmap_data.h"), os.path.join(outdir, "bitmap_data.c"))
mg.write_menu_data(os.path.join(outdir, "inc/menu_data.h"), os.path.join(outdir, "menu_data.c"))

write_data_interface(outdir, "data_interface", len([evt for evt in eg.events.values() if evt.internal == False]), draw_direction, "inc")

print("DONE!")
