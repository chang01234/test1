# firmware/hmi/projects/full_climate/readpng
Readpng program used for generating HMI interface data from definitions.

# Usage instructions

## Requirements

### Python
Readpng requires Python version 3.3 or newer. Download and install from
https://www.python.org/downloads/.

Note that Windows specific instructions in this readme assume that the py
launcher has been installed and added to path. You might need to prefix all
commands with "py" to make them launch inside the calling terminal.

### PyPNG
Readpng requires the PyPNG python package. This is most easily installed
using pip by running "pip install --user pypng"

On Windows run "py -m pip install --user pypng"

## Running
Readpng should be run from a terminal as "readpng.py defdir rotation [outdir]".

Defdir specifies the directory containing all HMI interface definition files.
This directory must contain the sub-directories bitmaps, fonts, menus, and
screens and the file vars.txt. For details on these see the HMI interface
definition documentation.

Rotation specifies the orientation of the HMI hardware display. Valid values
are 0deg, 90deg, 180deg, and 270deg. These specify the clockwise rotation of
all graphic elements in degrees.

Specifying outdir is optional. If provided .c files will be written to this
directory and .h files will be written to directory outdir/Inc. If outdir is
not provided Readpng will attempt to find the default fc_data directory.

To build SHARC-test HMI data run "readpng.py data/sharc 90deg"