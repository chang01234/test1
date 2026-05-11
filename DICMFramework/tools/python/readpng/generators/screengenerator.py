"""Provides ScreenGenerator class used to parse screen definition files."""

import os
import re

from lib.defreader import DefReader, DefError
from deftypes.misc import Screen, Background, Sprite, Value
from deftypes.conditions import *
from constants import FB_MAX_SIZE


class ScreenGenerator:
    """Generates screen data for the HMI engine from definition files.

    A ScreenGenerator is used to parse HMI screen definition files. Definitions
    are read in from the directory specified during __init__ and then provided to
    other generators trough the screens attribute.

    The write_screen_data method is used to write out contained data to .c/.h files
    in a format understood by the HMI engine.

    Attributes
    ----------
    screens : Dict of (str: Screen)
        A dict of all generated screens indexed by screen name.

    Methods
    -------
    write_screen_data
        Write ScreenGenerator contents to file for use by HMI engine.

    """

    def __init__(self, screen_path, rotation, bitmaps, fonts, variables):
        """Initializer for ScreenGenerator class.

        New ScreenGenerator will contain one Screen object for each .txt file found
        in directory given by screen_path. Each Screen will have the same name as the
        definition file it was generated from.

        Rotation must be specified and can be used if display on the HMI hardware
        has not been mounted in an upright orientation. Note that ScreenGenerator
        will only change the position of graphic objects on the display. Rotation
        of bitmaps done by BitmapGenerator.

        Name-indexed dicts of available bitmaps, fonts, and variables must be given.
        These will typically be provided by their respective generators.

        Parameters
        ----------
        screen_path : str
            Path to folder containing screen definition files.
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Clockwise rotation applied to all screen object coordinates.
        bitmaps : Dict of (str: Bitmap)
            A dict of available bitmaps indexed by bitmap name.
        fonts : Dict of (str: Font)
            A dict of available fonts indexed by font name.
        variables : Dict of (str: Variable)
            A dict of available variables indexed by variable name.

        Raises
        ------
        DefError
            Raised if ScreenGenerator fails to parse any definition file.

        """

        self._bitmaps = bitmaps
        self._fonts = fonts
        self._variables = variables

        self.screens = {}
        self._backgrounds = {}
        self._sprites = {}
        self._strings = {}
        self._conditions = {}

        for file_name in os.listdir(screen_path):
            file = os.path.join(screen_path, file_name)
            if os.path.isfile(file) and os.path.splitext(file_name)[1].lower() == ".txt":
                self._add_screen(file, rotation)


    def _add_background(self, line, reader):
        """ Returns a new Background object created from definition in line.

        New Background will also be added to ScreenGenerator dict of backgrounds
        indexed by background cname.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new background.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        Background
            New Background object.

        """

        # Check that there are enough fields.
        if len(line) < 2:
            raise DefError("Insufficient parameters for background definition.", reader.name, reader.line_no)

        # Create background
        if line[1] not in self._bitmaps:
            raise DefError("Could not find bitmap", reader.name, reader.line_no)
        background = Background(self._bitmaps[line[1]])

        # Find a name that's not taken.
        if background.cname in self._backgrounds:
            background.cname += "_0"
            suffix = 0
            while background.cname in self._backgrounds:
                suffix += 1
                if suffix <= 10:
                    background.cname = background.cname[:-1] + str(suffix)
                elif suffix <= 100:
                    background.cname = background.cname[:-2] + str(suffix)
                else:
                    background.cname = background.cname[:-3] + str(suffix)

        print("\t Added background: " + background.cname)

        # Add _conditions if any.
        for field in line[2:]:
            new_condition = self._add_condition(field, reader)
            if background.first_condition is None:
                background.first_condition = new_condition
            else:
                condition.next_condition = new_condition
            condition = new_condition

        # Add to list of all backgrounds.
        self._backgrounds[background.cname] = background

        return background

    def _add_condition(self, definition, reader):
        """ Returns a new Condition object created from definition.

        New Condition will also be added to ScreenGenerator dict of conditions
        indexed by condition cname.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new condition.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        Condition
            New Condition object.

        """

        # Catch toggle type conditions first since reader.parse_condition() doesn't understand them.
        if definition.startswith("toggle"):
            data = [str.strip() for str in definition.split(":")]
            if len(data) != 3:
                raise DefError("Incorrect number of parameters for toggle type condition.", reader.name, reader.line_no)
            condition = DelayPeriodic(*data[1:])

        # If not a toggle type condition generate new condition using reader.parse_condition().
        else:
            (cnd_type, var, value, offset) = reader.parse_condition(definition, self._variables)
            if cnd_type == "==":
                condition = CompEqual(var, value, offset)
            elif cnd_type == "!=":
                condition = CompNotEqual(var, value, offset)
            elif cnd_type == "<=":
                condition = CompLessEqual(var, value, offset)
            elif cnd_type == ">=":
                condition = CompMoreEqual(var, value, offset)
            elif cnd_type == "<":
                condition = CompLess(var, value, offset)
            elif cnd_type == ">":
                condition = CompMore(var, value, offset)
            elif cnd_type == "<_time":
                condition = DelayOff(value)
            elif cnd_type == ">_time":
                condition = DelayOn(value)
            else:
                raise DefError("{} is not valid screen condition type.".format(cnd_type), reader.name, reader.line_no)

        # Find a name that's not taken.
        suffix = 0
        while condition.cname in self._conditions:
            suffix += 1
            if suffix <= 10:
                condition.cname = condition.cname[:-1] + str(suffix)
            elif suffix <= 100:
                condition.cname = condition.cname[:-2] + str(suffix)
            else:
                condition.cname = condition.cname[:-3] + str(suffix)

        print("\t\t With condition: " + condition.cname)

        # Add to list of all conditions.
        self._conditions[condition.cname] = condition

        return condition

    def _add_screen(self, file, rotation):
        """ Add new Screen object created from definition in file to screens dict.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new background.
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Clockwise rotation applied to all screen object coordinates.

        """

        # Create screen.
        file_name = os.path.basename(file)
        screen_name = os.path.splitext(file_name)[0].lower()
        if screen_name in self.screens:
            raise DefError("Multiple screens with same name", file_name, None)
        screen = Screen(screen_name)
        self.screens[screen_name] = screen

        print("Adding screen: " + screen.name)

        # Parse screen definition by line.
        with DefReader(file) as screen_def:
            for line in screen_def:
                if line[0] == "framebuffer":

                    if screen.fb_x_length is not None:
                        raise DefError("Screen has multiple frame buffer definitions.", screen_def.name, screen_def.line_no)

                    screen.set_fb(rotation, *(int(field) for field in line[1:]))

                    if screen.fb_x_length*screen.fb_y_length > FB_MAX_SIZE:
                        raise DefError("Frame buffer to large.", screen_def.name, screen_def.line_no)

                    print("\t Added frame buffer: " + str(screen.get_fb()))

                elif line[0] == "background":

                    new_background = self._add_background(line, screen_def)
                    screen.update_var_list(new_background.first_condition)

                    if screen.first_background is None:
                        screen.first_background = new_background
                    else:
                        background.next_background = new_background

                    background = new_background

                elif line[0] == "sprite":

                    new_sprite = self._add_sprite(line, rotation, screen_def)
                    screen.update_var_list(new_sprite.first_condition)

                    if screen.first_sprite is None:
                        screen.first_sprite = new_sprite
                    else:
                        sprite.next_sprite = new_sprite

                    sprite = new_sprite

                elif line[0] == "value":

                    new_value = self._add_value(line, rotation, screen_def)
                    screen.update_var_list(new_value.first_condition)

                    if new_value.variable not in screen.variables:
                        screen.variables.append(new_value.variable)

                    if screen.first_string is None:
                        screen.first_string = new_value
                    else:
                        value.next_string = new_value

                    value = new_value

                else:
                    raise DefError("Unknown screen object type.", screen_def.name, screen_def.line_no)

        # Do some sanity checks
        if screen.fb_x_length is None:
            raise DefError("Screen has no frame buffer.", screen_def.name, None)
        if screen.first_background is None:
            raise DefError("Screen has no background.", screen_def.name, None)

        # Check that framebuffer fits on screen.
        (fb_end_x, fb_end_y) = (screen.fb_x_start + screen.fb_x_length - 1, screen.fb_y_start + screen.fb_y_length - 1)
        if screen.fb_x_start < 0 or screen.fb_y_start < 0:
            raise DefError("Frame buffer starts outside screen.", screen_def.name, None)
        if fb_end_x >= 240 or fb_end_y >= 240:
            raise DefError("Frame buffer ends outside screen.", screen_def.name, None)

        # Check that all sprites are inside frame buffer.
        sprite = screen.first_sprite
        while sprite is not None:

            if sprite.x_pos < screen.fb_x_start or sprite.y_pos < screen.fb_y_start:
                raise DefError("Sprite {} starts outside frame buffer.".format(sprite.cname), screen_def.name, None)

            (sp_end_x, sp_end_y) = (sprite.x_pos + sprite.bitmap.width - 1, sprite.y_pos + sprite.bitmap.height - 1)
            if sp_end_x > fb_end_x or sp_end_y > fb_end_y:
                raise DefError("Sprite {} ends outside frame buffer.".format(sprite.cname), screen_def.name, None)

            sprite = sprite.next_sprite

        #TODO: Check that strings are inside frame buffer.

    def _add_sprite(self, line, rotation, reader):
        """ Returns a new Sprite object created from definition in line.

        New Sprite will also be added to ScreenGenerator dict of sprites
        indexed by sprite cname.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new sprite.
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Clockwise rotation applied to all screen object coordinates.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        Sprite
            New Sprite object.

        """

        # Check that there are enough fields.
        if len(line) < 4:
            raise DefError("Insufficient parameters for sprite definition.", reader.name, reader.line_no)

        # Create sprite
        if line[1] not in self._bitmaps:
            raise DefError("Could not find bitmap", reader.name, reader.line_no)
        sprite = Sprite(self._bitmaps[line[1]], rotation, int(line[2]), int(line[3]))

        # Find a name that's not taken.
        if sprite.cname in self._sprites:
            sprite.cname += "_0"
            suffix = 0
            while sprite.cname in self._sprites:
                suffix += 1
                if suffix <= 10:
                    sprite.cname = sprite.cname[:-1] + str(suffix)
                elif suffix <= 100:
                    sprite.cname = sprite.cname[:-2] + str(suffix)
                else:
                    sprite.cname = sprite.cname[:-3] + str(suffix)

        sprite_end = (sprite.x_pos + sprite.bitmap.width - 1, sprite.y_pos + sprite.bitmap.height - 1)
        print("\t Added sprite: " + sprite.cname + " at position " + str(sprite.get_pos()) + " to " + str(sprite_end))

        # Add _conditions if any.
        for field in line[4:]:
            new_condition = self._add_condition(field, reader)
            if sprite.first_condition is None:
                sprite.first_condition = new_condition
            else:
                condition.next_condition = new_condition
            condition = new_condition

        # Add to list of all sprites.
        self._sprites[sprite.cname] = sprite

        return sprite

    def _add_value(self, line, rotation, reader):
        """ Returns a new Value object created from definition in line.

        New Value will also be added to ScreenGenerator dict of strings
        indexed by value cname.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new value.
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Clockwise rotation applied to all screen object coordinates.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        Value
            New Value object.

        """

        # Check that there are enough fields.
        if len(line) < 6:
            raise DefError("Insufficient parameters for value definition.", reader.name, reader.line_no)

        # Create sprite
        if line[1] not in self._fonts:
            raise DefError("Could not find font", reader.name, reader.line_no)
        # Check offset and len specification
        basevar = line[6]
        spec = basevar.split(":")
        if len(spec) > 1:
            basevar = spec[0]
            if len(spec) != 3:
                print(spec)
                raise DefError("offset:length not specified", reader.name, reader.line_no)
            spec = spec[1:]
        else:
            spec = None    
        if basevar not in self._variables:
            raise DefError("Could not find variable", reader.name, reader.line_no)
        value = Value(self._fonts[line[1]], rotation, int(line[2]), int(line[3]), int(line[4]), int(line[5]), self._variables[basevar], line[7], spec)

        # Find a name that's not taken.
        if value.cname in self._strings:
            value.cname += "_0"
            suffix = 0
            while value.cname in self._strings:
                suffix += 1
                if suffix <= 10:
                    value.cname = value.cname[:-1] + str(suffix)
                elif suffix <= 100:
                    value.cname = value.cname[:-2] + str(suffix)
                else:
                    value.cname = value.cname[:-3] + str(suffix)

        print("\t Added value: " + value.cname + " at position " + str(value.get_pos()))

        # Add conditions if any.
        for field in line[8:]:
            new_condition = self._add_condition(field, reader)
            if value.first_condition is None:
                value.first_condition = new_condition
            else:
                condition.next_condition = new_condition
            condition = new_condition

        # Add to list of all strings.
        self._strings[value.cname] = value

        return value

    def write_screen_data(self, header_file, source_file):
        """Exports ScreenGenerator contents in a format understood by the HMI engine.

        ScreenGenerator contents will be written to source_file while header_file
        will extern names needed by other HMI data files.

        Parameters
        ----------
        header_file, source_file : str
            Full paths to source and header file to be written.

        """

        with open(header_file, "w") as f:
            guarddef = re.sub('[^A-Za-z0-9]', '_', os.path.basename(header_file).upper()) + '_'

            f.write("/*! \\file " + os.path.basename(header_file) + "\n")
            f.write(" *  \\brief Screen data header. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("#ifndef " + guarddef + "\n")
            f.write("#define " + guarddef + "\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write('#include <stdint.h>\n')
            f.write('#include <stdbool.h>\n')
            f.write('#include "bitmap_data.h"\n')
            f.write("\n")
            f.write("/** Defines *******************************************************************/\n")
            f.write("#define SCREEN_MAX_STATE_VARS       30  //!< Maximum number of unique system state variables allowed in conditions for a single screen.\n")
            f.write("\n")
            f.write("/** Typedefs ******************************************************************/\n")
            f.write("\n")
            f.write("//! Screen orientation.\n")
            f.write("typedef enum DRAW_DIRECTION_ENUM\n")
            f.write("{\n")
            f.write("\tDRAW_DIRECTION_UP,                  //!< Screen is not rotated.\n")
            f.write("\tDRAW_DIRECTION_RIGHT,               //!< Screen is rotated 90 degrees.\n")
            f.write("\tDRAW_DIRECTION_DOWN,                //!< Screen is rotated 180 degrees.\n")
            f.write("\tDRAW_DIRECTION_LEFT,                //!< Screen is rotated 270 degrees.\n")
            f.write("} DRAW_DIRECTION_ENUM;\n")
            f.write("\n")
            f.write("//! Enumeration of possible compare types.\n")
            f.write("typedef enum SCREEN_CONDITION_TYPE\n")
            f.write("{\n")
            f.write("\tSCREEN_COMPARE_EQUAL,                            //!< True if var_value == condition_value.\n")
            f.write("\tSCREEN_COMPARE_NOT_EQUAL,                        //!< True if var_value != condition_value.\n")
            f.write("\tSCREEN_COMPARE_LESS_THAN,                        //!< True if var_value < condition_value.\n")
            f.write("\tSCREEN_COMPARE_GREATER_THAN,                     //!< True if var_value > condition_value.\n")
            f.write("\tSCREEN_COMPARE_LESS_THAN_OR_EQUAL,               //!< True if var_value <= condition_value.\n")
            f.write("\tSCREEN_COMPARE_GREATER_THAN_OR_EQUAL,            //!< True if var_value >= condition_value.\n")
            f.write("\tSCREEN_DELAY_ON,                                 //!< True while screen_tick > delay.\n")
            f.write("\tSCREEN_DELAY_OFF,                                //!< True while screen_tick < delay.\n")
            f.write("\tSCREEN_DELAY_PERIODIC,                           //!< True while (screen_tick % delay) < on_time.\n")
            f.write("\tSCREEN_COMPARE_EQUAL_WITH_OFFSET,                //!< True if var_value:offset:len == condition_value.\n")
            f.write("\tSCREEN_COMPARE_NOT_EQUAL_WITH_OFFSET,            //!< True if var_value:offset:len != condition_value.\n")
            f.write("\tSCREEN_COMPARE_LESS_THAN_WITH_OFFSET,            //!< True if var_value:offset:len < condition_value.\n")
            f.write("\tSCREEN_COMPARE_GREATER_THAN_WITH_OFFSET,         //!< True if var_value:offset:len > condition_value.\n")
            f.write("\tSCREEN_COMPARE_LESS_THAN_OR_EQUAL_WITH_OFFSET,   //!< True if var_value:offset:len <= condition_value.\n")
            f.write("\tSCREEN_COMPARE_GREATER_THAN_OR_EQUAL_WITH_OFFSET //!< True if var_value:offset:len >= condition_value.\n")
            f.write("} SCREEN_CONDITION_TYPE;\n")
            f.write("\n")
            f.write("//! Enumeration of all possible string object types.\n")
            f.write("typedef enum SCREEN_STRING_TYPE\n")
            f.write("{\n")
            f.write("\tSCREEN_STRING_TYPE_VALUE,\n")
            f.write("\tSCREEN_STRING_TYPE_VALUE_WITH_OFFSET,\n")
            f.write("} SCREEN_STRING_TYPE;\n")
            f.write("\n")
            f.write("//! Struct used to hold type specific definition data for a string object used to render a number.\n")
            f.write("typedef struct SCREEN_STRING_VALUE\n")
            f.write("{\n")
            f.write("\tchar *format_string;    //!< printf-style format string used to convert number to string.\n")
            f.write("\tuint8_t var_index;      //!< Storage index of state variable to render.\n")
            f.write("} SCREEN_STRING_VALUE;\n")
            f.write("\n")
            f.write("//! Struct used to hold type specific definition data for a string object used to render a number.\n")
            f.write("typedef struct SCREEN_STRING_VALUE_WITH_OFFSET\n")
            f.write("{\n")
            f.write("\tchar *format_string;    //!< printf-style format string used to convert number to string.\n")
            f.write("\tuint8_t var_index;      //!< Storage index of state variable to render.\n")
            f.write("\tuint8_t offset;         //!< Offset of data to render.\n")
            f.write("\tuint8_t len;            //!< Length of data to render.\n")
            f.write("} SCREEN_STRING_VALUE_WITH_OFFSET;\n")
            f.write("\n")
            f.write("//! Struct used to hold type specific definition data for a condition that compares a predefined value to system state.\n")
            f.write("typedef struct SCREEN_CONDITION_COMPARE\n")
            f.write("{\n")
            f.write("\tint32_t value;      //!< Value to compare against.\n")
            f.write("\tuint8_t var_index;  //!< Storage index of state variable to compare with.\n")
            f.write("} SCREEN_CONDITION_COMPARE;\n")
            f.write("\n")
            f.write("//! Struct used to hold type specific definition data for a condition that compares a predefined value to system state with offset.\n")
            f.write("typedef struct SCREEN_CONDITION_COMPARE_OFFSET\n")
            f.write("{\n")
            f.write("\tint32_t value;      //!< Value to compare against.\n")
            f.write("\tuint8_t var_index;  //!< Storage index of state variable to compare with.\n")
            f.write("\tuint8_t offset;         //!< Offset of data to compare.\n")
            f.write("\tuint8_t len;            //!< Length of data to compare.\n")
            f.write("} SCREEN_CONDITION_COMPARE_OFFSET;\n")
            f.write("\n")
            f.write("//! Struct used to hold type specific definition data for a condition that is true/false for a predefined period of time.\n")
            f.write("typedef struct SCREEN_CONDITION_DELAY\n")
            f.write("{\n")
            f.write("\tuint32_t length;    //!< Time in system ticks before condition activates.\n")
            f.write("} SCREEN_CONDITION_DELAY;\n")
            f.write("\n")
            f.write("//! Struct used to hold type specific definition data for a condition that switches periodically between true false.\n")
            f.write("typedef struct SCREEN_CONDITION_PERIODIC\n")
            f.write("{\n")
            f.write("\tuint32_t period;    //!< Duration of one period in system ticks.\n")
            f.write("\tuint32_t on_time;   //!< Number of ticks of each period for which condition should be true. Starting from first tick in period.\n")
            f.write("} SCREEN_CONDITION_PERIODIC;\n")
            f.write("\n")
            f.write("//! Struct defining a comparison used when checking if a screen object should be drawn or not.\n")
            f.write("typedef struct SCREEN_CONDITION\n")
            f.write("{\n")
            f.write("\tconst struct SCREEN_CONDITION *next_condition;  //!< Next condition in linked list. NULL if this is last condition in list.\n")
            f.write("\tunion\n\t{\n")
            f.write("\t\tSCREEN_CONDITION_COMPARE compare;                  //!< Type specific data for compare conditions.\n")
            f.write("\t\tSCREEN_CONDITION_DELAY delay;                      //!< Type specific data for delay conditions.\n")
            f.write("\t\tSCREEN_CONDITION_PERIODIC period;                  //!< Type specific data for periodic conditions.\n")
            f.write("\t\tSCREEN_CONDITION_COMPARE_OFFSET compare_offset;    //!< Type specific data for compare (with offset) conditions.\n")
            f.write("\t};\n")
            f.write("\tSCREEN_CONDITION_TYPE type;                     //!< Condition type.\n")
            f.write("} SCREEN_CONDITION;\n")
            f.write("\n")
            f.write("//! Struct defining a single background used by a screen.\n")
            f.write("typedef struct SCREEN_BACKGROUND\n")
            f.write("{\n")
            f.write("\tconst BITMAP_DEF *bitmap;                       //!< Background bitmap data.\n")
            f.write("\tconst SCREEN_CONDITION *first_condition;        //!< First entry in linked list of draw conditions.\n")
            f.write("\tconst struct SCREEN_BACKGROUND *next_background;//!< Next background in linked list. NULL if this is last background in list.\n")
            f.write("} SCREEN_BACKGROUND;\n")
            f.write("\n")
            f.write("//! Struct defining a single sprite to be used as part of a screen.\n")
            f.write("typedef struct SCREEN_SPRITE\n")
            f.write("{\n")
            f.write("\tconst BITMAP_DEF *bitmap;                   //!< Sprite bitmap data.\n")
            f.write("\tconst SCREEN_CONDITION *first_condition;    //!< First entry in linked list of draw conditions.\n")
            f.write("\tconst struct SCREEN_SPRITE *next_sprite;    //!< Next sprite in linked list. NULL if this is last sprite in list.\n")
            f.write("\tuint8_t x_pos;                              //!< X coordinate for image top left corner.\n")
            f.write("\tuint8_t y_pos;                              //!< Y coordinate for image top left corner.\n")
            f.write("} SCREEN_SPRITE;\n")
            f.write("\n")
            f.write("//! Struct defining a single string to be rendered using font engine.\n")
            f.write("typedef struct SCREEN_STRING\n")
            f.write("{\n")
            f.write("\tconst FONT_DEF *font;                       //!< Font definition to use when rendering string.\n")
            f.write("\tconst SCREEN_CONDITION *first_condition;    //!< First entry in linked list of draw conditions.\n")
            f.write("\tconst struct SCREEN_STRING *next_string;    //!< Next string in linked list. NULL if this is last string in list.\n")
            f.write("\tunion\n\t{\n")
            f.write("\t\tSCREEN_STRING_VALUE value;                     //!< Type specific data for value type strings.\n")
            f.write("\t\tSCREEN_STRING_VALUE_WITH_OFFSET value_offset;  //!< Type specific data for value type offset strings.\n")
            f.write("\t};\n")
            f.write("\tSCREEN_STRING_TYPE type;            //!< String type.\n")
            f.write("\tuint8_t x_pos;                      //!< X coordinate for string area top left corner.\n")
            f.write("\tuint8_t y_pos;                      //!< Y coordinate for string area top left corner.\n")
            f.write("\tuint8_t x_size;                     //!< String area length in X direction.\n")
            f.write("\tuint8_t y_size;                     //!< String area length in Y direction.\n")
            f.write("} SCREEN_STRING;\n")
            f.write("\n")
            f.write("//! Struct defining a single screen.\n")
            f.write("typedef struct SCREEN_DEF\n")
            f.write("{\n")
            f.write("#ifdef HMI_SCREEN_DEBUG_NAMES\n")
            f.write("\tconst char *name;\n")
            f.write("#endif\n")
            f.write("\tconst PALETTE_DEF *palette;                     //!< Palette to be used by screen.\n")
            f.write("\tconst SCREEN_BACKGROUND *first_background;      //!< First entry in linked list of possible backgrounds for screen.\n")
            f.write("\tconst SCREEN_SPRITE *first_sprite;              //!< First entry in linked list of sprites for screen. NULL if screen contains no sprites,\n")
            f.write("\tconst SCREEN_STRING *first_string;              //!< First entry in linked list of strings for screen. NULL if screen contains no strings\n")
            f.write("\tuint8_t fb_x_pos;                               //!< X coordinate for frame buffer top left corner.\n")
            f.write("\tuint8_t fb_y_pos;                               //!< Y coordinate for frame buffer top left corner.\n")
            f.write("\tuint8_t fb_x_size;                              //!< Frame buffer length in X direction.\n")
            f.write("\tuint8_t fb_y_size;                              //!< Frame buffer length in Y direction.\n")
            f.write("\tuint8_t num_state_indexes;                      //!< Number of unique state indexes used by object conditions.\n")
            f.write("\tuint8_t state_indexes[SCREEN_MAX_STATE_VARS];   //!< List of unique state indexes used by object conditions.\n")
            f.write("} SCREEN_DEF;\n")
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            f.write("\n")

            for screen in self.screens.values():
                f.write("extern const SCREEN_DEF " + screen.cname + ";\n")

            f.write("\n")
            f.write("#endif /* " + guarddef + " */\n")

        with open(source_file, "w") as f:
            f.write("/* clang-format off */" + "\n")
            f.write("/*! \\file " + os.path.basename(source_file) + "\n")
            f.write(" *  \\brief Screen data definitions. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write("\n")
            f.write("#include <stdint.h>\n")
            f.write("#include <stddef.h>\n")
            f.write("#include <stdbool.h>\n")
            f.write("\n")
            f.write('#include "' + os.path.basename(header_file) + '"\n')
            f.write('#include "bitmap_data.h"\n')
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            f.write("/** Conditions ****************************************************************/\n")
            f.write("\n")

            # Condition, background, sprite, and string objects contain pointers to
            # other objects of the same type which means c definitions must be written
            # to file in the correct order to not generate GCC errors. They also
            # point at each other so the order must be be conditions followed by
            # backgrounds, sprites, and strings finally followed by screens.

            conditions = list(self._conditions.values())    # This creates a copy
            cnames = []
            while len(conditions) != 0:
                for condition in conditions:
                    if condition.next_condition is None or condition.next_condition.cname in cnames:
                        f.write(condition.c_repr())
                        f.write("\n")
                        conditions.remove(condition)
                        cnames.append(condition.cname)

            f.write("/** Backgrounds ***************************************************************/\n")
            f.write("\n")

            backgrounds = list(self._backgrounds.values())  # This creates a copy.
            cnames = []
            while len(backgrounds) != 0:
                for background in backgrounds:
                    if background.next_background is None or background.next_background.cname in cnames:
                        f.write(background.c_repr())
                        f.write("\n")
                        backgrounds.remove(background)
                        cnames.append(background.cname)

            f.write("/** Sprites *******************************************************************/\n")
            f.write("\n")

            sprites = list(self._sprites.values())  # This creates a copy.
            cnames = []
            while len(sprites) != 0:
                for sprite in sprites:
                    if sprite.next_sprite is None or sprite.next_sprite.cname in cnames:
                        f.write(sprite.c_repr())
                        f.write("\n")
                        sprites.remove(sprite)
                        cnames.append(sprite.cname)

            f.write("/** Strings *******************************************************************/\n")
            f.write("\n")

            strings = list(self._strings.values())  # This creates a copy.
            cnames = []
            while len(strings) != 0:
                for string in strings:
                    if string.next_string is None or string.next_string.cname in cnames:
                        f.write(string.c_repr())
                        f.write("\n")
                        strings.remove(string)
                        cnames.append(string.cname)

            f.write("/** Screens *******************************************************************/\n")
            f.write("\n")

            for screen in self.screens.values():
                f.write(screen.c_repr())
                f.write("\n")
