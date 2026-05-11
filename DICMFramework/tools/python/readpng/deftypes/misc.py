"""Classes for HMI definitions not yet moved to their own modules."""

import sys
import re
from lib.defreader import DefError
from constants import DISPLAY_LENGTH

# Requires that PYTHONPATH is set to full path to "DICMFramework/lib" as the file 
# is generated together with other ddm2 parameter files

import ddm2_parameter_list as ddm2_list

class Variable:
    """Defines a HMI engine variable.

    A Variable object represents a value stored by the HMI engine in RAM that can
    written to or read from by menu actions and screen conditions.

    To allow storing non-integer values a step length is included in each Variable
    object. This together with the scale step function allows the definition data
    to specify set-points and target values literally and leave conversion to the
    correct number of steps to Readpng.

    This class should be used only for local variables. Variables that correspond
    to DDMP2 parameters should instead use the VarDDMP subclass.

    Attributes
    ----------
    index : int
        Variable index value used by HMI engine.
    name : str
        Variable name used in definition files.
    step : float
        Variable step length.
    default : int
        Variable default value. In number of steps.

    Methods
    -------
    scale_step
        Returns number of variable steps equal to input value.

    """
    def __init__(self, index, name, step, default):
        """Initializer for Variable class.

        Parameters
        ----------
        index : int
            Variable index value used by HMI engine.
        name : str
            Variable name used in definition files.
        step : float
            Variable step length.
        default : int
            Variable default value. In number of steps.

        """
        self.index = index
        self.name = name
        self.step = step
        self.default = default
        self.dynamic_param = None

    def c_repr(self):
        """Returns a string representation of the variable understood by the HMI engine."""
        string = "\t{\n"
        string += "#ifdef HMI_VARSTATE_DEBUG_NAMES\n"
        string += "\t\t.name = \"" + self.name + "\",\n"
        string += "#endif\n"
        string += "\t\t.default_value = " + str(self.default) + ",\n"
        string += "\t\t.step = " + str(self.step) + "f,\n"
        string += "\t},\n"

        return string

    def c_index_repr(self):
        """Returns a string representation of the index understood the HMI engine."""
        string = "\tVARSTATE_" + str.upper(self.name) + "_INDEX = " + str(self.index) + ",\n"

        return string

    def scale_step(self, value, reader):
        """Returns value in number of variable steps.

        This method is used to convert float set-points and target values provided
        in HMI definition files into integer step counts for use internal by the
        HMI engine.

        Arguments
        ---------
        value : float
            Value to scale.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        int
            Number of steps equivalent to value.

        Raises
        ------
        DefError
            If value can not be represented by variable step length.

        """

        value = float(value)/self.step
        if not value.is_integer():
            raise DefError("Value must be a whole number of steps. is {} steps".format(value),
                           reader.name, reader.line_no)
        value = int(value)

        return value

class VarDDMP(Variable):
    """Defines a HMI engine variable linked to a DDMP2 parameter.

    Attributes
    ----------
    index : int
        Variable index value used by HMI engine.
    name : str
        Variable name used in definition files.
    step : float
        Variable step length.
    default : int
        Variable default value. In number of steps.
    parameter_id : str
        ID of linked DDMP2 variable.
    data_type : str
        DDMP2 data type used by parameter.
    c_name : str
        A c-compatible name for the Variable.

    Methods
    -------
    scale_step
        Returns number of variable steps equal to input value.

    """

    def __init__(self, index, name, step, default, parameter_id, data_type):
        """Initializer for VarDDMP class.

        Parameters
        ----------
        index : int
            Variable index value used by HMI engine.
        name : str
            Variable name used in definition files.
        step : float
            Variable step length.
        default : int
            Variable default value. In number of steps.
        parameter_id : int
            ID of linked DDMP2 variable.
        data_type : str
            DDMP2 data type used by parameter.

        """
        super().__init__(index, name, step, default)
        
        # First we check if parameter string is in a dynamic format for instance value
        dynamic_match = True if (len(re.split(r'[<>]', parameter_id)) == 3) else False
        
        if dynamic_match:
            m = re.split(r'[<>]', parameter_id)
            loc_parameter_id = m[0] + "0" + m[2]
            self.dynamic_param = m[1]
        else:
            loc_parameter_id = parameter_id
        
        # ddm2_list.ddm_params_dict supports only parameters with instance 0.
        # We should override the 'parameter_id' name using the instance 0 so we can find the paramateter of intrest in ddm_params_dict.
        ddm2_parameter_instance = 0
        ddm2_parameter_name, ddm2_parameter_instance = self.generate_ddm2_parameter_base_name(loc_parameter_id)

        self.parameter_id = ddm2_list.ddm_params_dict[str.upper(ddm2_parameter_name)]
        # apply back the instance number
        self.parameter_id = self.parameter_id | (ddm2_parameter_instance << 8)
        # Add dynamic info
        if dynamic_match:
            # Bit 31 hold information about dynamic part
            self.parameter_id = self.parameter_id | (1 << 31)
        self.data_type = data_type
        self.c_name = "ddmp_par_" + self.name

    def update_param_id(self, dyn_map_index):
        self.parameter_id = self.parameter_id | (dyn_map_index << 8)
        
    def generate_ddm2_parameter_base_name(self, parameter_id):
        # first integer in 'parameter_id'.
        ddm2_parameter_instance_field = re.search(r'\d+', parameter_id)

        # location of the instance value in 'parameter_id'
        ddm2_parameter_instance_index = ddm2_parameter_instance_field.start()
        # instance value as string
        ddm2_parameter_instance_str = ddm2_parameter_instance_field.group()
        ddm2_parameter_instance = int(ddm2_parameter_instance_str)
        if ddm2_parameter_instance > 0xFF:
            raise EventError("Invalid instance: " + ddm2_parameter_instance_str + "for parameter: " + parameter_id + ". Maximum supported instances are 255")

        # i.e. parameter_id = AC1TEMP
        # i.e. AC
        ddm2_parameter_class_substr = parameter_id[:ddm2_parameter_instance_index]
        # i.e. TEMP
        ddm2_parameter_name_substr = parameter_id[ddm2_parameter_instance_index + len(ddm2_parameter_instance_str):]
        ddm2_parameter_override_instance_value = '0'

        # i.e. generate AC0TEMP from AC1TEMP
        ddm2_parameter_name = ddm2_parameter_class_substr + ddm2_parameter_override_instance_value + ddm2_parameter_name_substr
        return ddm2_parameter_name, ddm2_parameter_instance
    
    def c_ddmp_repr(self, next_variable):
        """Returns a string representation of the DDMP2 parameter data understood by the HMI engine.
        
        Since HMI engine publish and subscribe lists are linked lists c_ddmp_repr()
        takes the next variable in the list as a parameter so it can write out the
        corresponding cname-pointer. next_variable can also be None if this is the
        last variable in the list.

        Note that both c_repr() and c_ddmp_repr() need to be written to HMI engine files.

        Parameters
        ----------
        next_variable : VarDDMP, None
            Next VarDDMP-object in linked list being written.

        """
        string = "static const VARSTATE_DDMP_PARAMETER " + self.c_name + " =\n"
        string += "{\n"
        string += "#ifdef HMI_VARSTATE_DEBUG_NAMES\n"
        string += "\t\t.name = \"" + self.name + "\",\n"
        string += "#endif\n"
        string += "\t.parameter_id = " + hex(self.parameter_id) + ",\n"
        string += "\t.type = " + self.data_type + ",\n"
        string += "\t.var_index = " + str(self.index) + ",\n"

        if next_variable is not None:
            string += "\t.next_entry = &" + next_variable.c_name + ",\n"
        else:
            string += "\t.next_entry = NULL,\n"

        string += "};\n"
        return string

class State:
    """Defines a HMI menu state.

    A State object represents a possible state for the HMI menu. Each menu state
    consists of a screen to show the user and a set of possible actions indexed by
    menu events which are integers generated by timers, DDMP messages, or user
    actions.

    Attributes
    ----------
    name : str
        Menu state name used in definition files.
    cname : str
        A c-compatible name for the menu state.
    file : str
        Path to menu state definition file.
    screen : Screen
        Screen used by menu state.
    actions : dict of {int: Action}
        All menu state actions indexed by triggering event index.

    Methods
    -------
    c_repr
        Returns a string representation of this menu state understood by the HMI engine.

    """

    def __init__(self, name, file):
        """Initializer for State class.

        Parameters
        ----------
        name : str
            Menu state name used in definition files.
        file : str
            Path to menu state definition file.

        """
        self.name = name
        self.cname = "st_" + name
        self.file = file
        self.screen = None
        self.actions = {}

    def c_repr(self):
        """Returns a string representation of the menu state understood by the HMI engine."""
        string = "static const MENU_STATE " + self.cname + " =\n"
        string += "{\n"
        string += "#ifdef HMI_MENU_DEBUG_NAMES\n"
        string += "\t.name = \"" + self.cname + "\",\n"
        string += "#endif // HMI_MENU_DEBUG_NAMES\n"
        string += "\t.screen = &" + self.screen.cname + ",\n"
        string += "\t.actions =\n"
        string += "\t{\n"

        for event in self.actions:
            string += "\t\t[" + str(event.index) + "] = &" + self.actions[event].cname + ",\n"

        string += "\t},\n"
        string += "};\n"

        return string

class Screen:
    """Defines a HMI screen.

    A screen object represents a set of graphics with attached conditions
    specifying which graphic objects should be displayed to user based on time
    and HMI variable state.

    Attributes
    ----------
    name : str
        Screen name used in definition files.
    cname : str
        A c-compatible name for the screen.
    first_background : Background
        First object in the linked list of backgrounds belonging to screen.
    first_sprite : Sprite, None
        First object in the linked list of sprites belonging to screen.
    first_string : Value, None
        First object in the linked list of strings belonging to screen.
    variables : List of Variable
        List of variables used by screen strings and conditions.

    Methods
    -------
    c_repr
        Returns a string representation of this screen understood by the HMI engine.
    get_fb
        Returns screen frame buffer coordinates.
    set_fb
        Sets screen frame buffer coordinates.

    """

    def __init__(self, name):
        """Initializer for Screen class.

        Parameters
        ----------
        name : str
            Screen name used in definition files.

        """
        self.name = name
        self.cname = "screen_" + name
        self.first_background = None
        self.first_sprite = None
        self.first_string = None
        self.set_fb("0deg", None, None, None, None)
        self.variables = []

    def c_repr(self):
        """Returns a string representation of the screen understood by the HMI engine."""
        string = "const SCREEN_DEF " + self.cname + " =\n"
        string += "{\n"
        string += "#ifdef HMI_SCREEN_DEBUG_NAMES\n"
        string += "\t.name = \"" + self.cname + "\",\n"
        string += "#endif\n"
        string += "\t.palette = &" + self.first_background.bitmap.palette_cname + ",\n"
        string += "\t.first_background = &" + self.first_background.cname + ",\n"

        if self.first_sprite is None:
            string += "\t.first_sprite = NULL,\n"
        else:
            string += "\t.first_sprite = &" + self.first_sprite.cname + ",\n"

        if self.first_string is None:
            string += "\t.first_string = NULL,\n"
        else:
            string += "\t.first_string = &" + self.first_string.cname + ",\n"

        string += "\t.fb_x_pos = " + str(self.fb_x_start) + ",\n"
        string += "\t.fb_y_pos = " + str(self.fb_y_start) + ",\n"
        string += "\t.fb_x_size = " + str(self.fb_x_length) + ",\n"
        string += "\t.fb_y_size = " + str(self.fb_y_length) + ",\n"
        string += "\t.num_state_indexes = " + str(len(self.variables)) + ",\n"

        string += "\t.state_indexes = {"
        for variable in self.variables:
            string += str(variable.index) + ", "
        if string[-2:] == ", ":
            string = string[:-2]
        string += "},\n"

        string += "};\n"
        return string

    def get_fb(self):
        """Get frame buffer position and size."""
        return (self.fb_x_start, self.fb_y_start, self.fb_x_length, self.fb_y_length)

    def set_fb(self, rotation, x_start, y_start, x_length, y_length):
        """Set screen frame buffer position and size. Rotate provided coordinates if requested.

        set_fb() sets the screen frame buffer. If rotation is not 0deg coordinates
        are first translated into the rotated coordinate frame. This rotation is used
        to compensate for the display not being mounted in the correct orientation
        on the HMI hardware.

        Parameters
        ----------
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Rotation applied to frame buffer coordinates.
        x_start, y_start : int
            Top left corner of frame buffer. In pixels, Before rotation.
        x_lenth, y_length : int
            Size of frame buffer. In pixels, Before rotation.

        """

        if rotation == "0deg":
            self.fb_x_start = x_start
            self.fb_y_start = y_start
            self.fb_x_length = x_length
            self.fb_y_length = y_length
        elif rotation == "90deg":
            self.fb_x_start = y_start
            self.fb_y_start = DISPLAY_LENGTH - x_start - x_length
            self.fb_x_length = y_length
            self.fb_y_length = x_length
        elif rotation == "180deg":
            self.fb_x_start = DISPLAY_LENGTH - x_start - x_length
            self.fb_y_start = DISPLAY_LENGTH - y_start - y_length
            self.fb_x_length = x_length
            self.fb_y_length = y_length
        elif rotation == "270deg":
            self.fb_x_start = DISPLAY_LENGTH - y_start - y_length
            self.fb_y_start = x_start
            self.fb_x_length = y_length
            self.fb_y_length = x_length
        else:
            raise ValueError("{} is not a valid rotation.".format(rotation))


    def update_var_list(self, first_condition):
        """Checks condition list against variable list and adds any missing variables.

        Checks all conditions in linked list starting from first_condition against
        list of known state variables. Adds any missing variables to this list.

        first_condition can be None in which case this method returns without doing
        anything.

        Arguments
        ---------
        first_condition: Condition
            First condition in list of conditions to check against screen variable
            list.

        """

        if first_condition is not None:
            condition = first_condition

            # Some condition types have no variable so check if attribute exists first.
            if hasattr(condition, "variable") and condition.variable not in self.variables:
                self.variables.append(condition.variable)

            while condition.next_condition is not None:
                condition = condition.next_condition
                if hasattr(condition, "variable") and condition.variable not in self.variables:
                    self.variables.append(condition.variable)

class Background:
    """Defines a HMI background.

    A background defines the static graphical data of a screen.

    A screen must have at least one background. Multiple backgrounds are allowed in
    which case one is selected based on the attached conditions. If the background
    is changed the screen must be completely redrawn.

    Attributes
    ----------
    bitmap : Bitmap
        Bitmap containing background graphical data.
    cname : str
        A c-compatible name for the background.
    first_condition : Condition, None
        First object in the linked list of conditions belonging to background.
    next_background : Background, None
        Next Background in linked list of backgrounds. None if last object in list.

    Methods
    -------
    c_repr
        Returns a string representation of this background understood by the HMI engine.

    """

    def __init__(self, bitmap):
        """Initializer for Background class.

        Parameters
        ----------
        bitmap : Bitmap
            Bitmap containing background graphical data.

        """
        self.bitmap = bitmap
        self.cname = "bg_" + bitmap.name
        self.first_condition = None
        self.next_background = None

    def c_repr(self):
        """Returns a string representation of the background understood by the HMI engine."""
        string = "static const SCREEN_BACKGROUND " + self.cname + " =\n"
        string += "{\n"
        string += "\t.bitmap = &" + self.bitmap.cname + ",\n"

        if self.first_condition is None:
            string += "\t.first_condition = NULL,\n"
        else:
            string += "\t.first_condition = &" + self.first_condition.cname + ",\n"

        if self.next_background is None:
            string += "\t.next_background = NULL, \n"
        else:
            string += "\t.next_background = &" + self.next_background.cname + ",\n"

        string += "};\n"
        return string

class Sprite:
    """Defines a HMI sprite.

    Sprites provide the dynamic elements of the HMI graphics.

    Each sprite has a set of 0 or more conditions which determine if the sprite
    should be drawn or not. New sprites can be drawn on the screen dynamically
    without having to redraw the whole screen.

    Sprites can not be deleted from the screen. To "remove" a sprite it must be
    painted over by a new sprite.

    Attributes
    ----------
    bitmap : Bitmap
        Bitmap containing sprite graphical data.
    cname : str
        A c-compatible name for the sprite.
    first_condition : Condition, None
        First object in the linked list of conditions belonging to sprite.
    next_sprite : Sprite, None
        Next Sprite in linked list of sprites. None if last object in list.

    Methods
    -------
    c_repr
        Returns a string representation of this sprite understood by the HMI engine.
    get_pos
        Returns sprite coordinates.
    set_pos
        Sets sprite coordinates.

    """

    def __init__(self, bitmap, rotation, x_pos, y_pos):
        """Initializer for Sprite class.

        Parameters
        ----------
        bitmap : Bitmap
            Bitmap containing background graphical data.
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Rotation applied to sprite coordinates.
        x_pos, y_pos : int
            Top left corner of sprite. In pixels, Before rotation.

        """
        self.bitmap = bitmap
        self.cname = "sp_" + bitmap.name
        self.set_pos(rotation, x_pos, y_pos)
        self.first_condition = None
        self.next_sprite = None

    def c_repr(self):
        """Returns a string representation of the sprite understood by the HMI engine."""
        string = "static const SCREEN_SPRITE " + self.cname + " =\n"
        string += "{\n"
        string += "\t.x_pos = " + str(self.x_pos) + ",\n"
        string += "\t.y_pos = " + str(self.y_pos) + ",\n"
        string += "\t.bitmap = &" + self.bitmap.cname + ",\n"

        if self.first_condition is None:
            string += "\t.first_condition = NULL,\n"
        else:
            string += "\t.first_condition = &" + self.first_condition.cname + ",\n"

        if self.next_sprite is None:
            string += "\t.next_sprite = NULL, \n"
        else:
            string += "\t.next_sprite = &" + self.next_sprite.cname + ",\n"

        string += "};\n"
        return string

    def get_pos(self):
        """Get sprite position."""
        return (self.x_pos, self.y_pos)

    def set_pos(self, rotation, x_pos, y_pos):
        """Set sprite position. Rotate provided coordinates if requested.

        set_pos() sets the sprite position. If rotation is not 0deg coordinates
        are first translated into the rotated coordinate frame. This rotation is used
        to compensate for the display not being mounted in the correct orientation
        on the HMI hardware.

        Parameters
        ----------
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Rotation applied to sprite coordinates.
        x_pos, y_pos : int
            Top left corner of sprite. In pixels, Before rotation.

        """
        if rotation == "0deg":
            self.x_pos = x_pos
            self.y_pos = y_pos
        elif rotation == "90deg":
            self.x_pos = y_pos
            self.y_pos = DISPLAY_LENGTH - x_pos - self.bitmap.height
        elif rotation == "180deg":
            self.x_pos = DISPLAY_LENGTH - x_pos - self.bitmap.width
            self.y_pos = DISPLAY_LENGTH - y_pos - self.bitmap.height
        elif rotation == "270deg":
            self.x_pos = DISPLAY_LENGTH - y_pos - self.bitmap.width
            self.y_pos = x_pos
        else:
            raise ValueError("{} is not a valid rotation value.".format(rotation))


class Value:
    """Defines a HMI value-type string.

    Value objects are used to draw numbers on the screen using a font. Values are
    taken from HMI variables and converted into font indexes using a printf-style
    format specifier.

    Each value has a set of 0 or more conditions which determine if the value
    should be drawn or not. New values can be drawn on the screen dynamically
    without having to redraw the whole screen.

    Values are positioned on the screen with a position and size. This represents
    the draw area for the value which will be centered inside the area.

    Attributes
    ----------
    font : Font
        Font used to draw value.
    cname : str
        A c-compatible name for the value.
    variable : Variable
        Variable containing value to be displayed.
    format_string : str
        A printf-type format string used to convert variable value into a string to
        display.
    first_condition : Condition, None
        First object in the linked list of conditions belonging to value.
    next_string : Value, None
        Next object in linked list of HMI strings. None if last object in list.

    Methods
    -------
    c_repr
        Returns a string representation of this value understood by the HMI engine.
    get_pos
        Returns value coordinates.
    set_pos
        Sets value coordinates.

    """

    def __init__(self, font, rotation, x_pos, y_pos, x_size, y_size, variable, format_string, offset=None):
        """Initializer for Value class.

        Parameters
        ----------
        font : Font
            Font used to draw value.
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Rotation applied to value coordinates.
        x_pos, y_pos : int
            Top left corner of value. In pixels, Before rotation.
        x_size, y_size : int
            Maximum size of value string. In pixels, Before rotation.
        variable : Variable
            Variable containing value to be displayed.
        format_string : str
            A printf-type format string used to convert variable value into a string to

        """
        self.font = font
        self.set_pos(rotation, x_pos, y_pos, x_size, y_size)
        self.variable = variable
        self.format_string = format_string
        self.cname = variable.name + "_" + font.name
        self.first_condition = None
        self.next_string = None
        self.offset = offset

    def c_repr(self):
        """Returns a string representation of the value string understood by the HMI engine."""
        string = "static const SCREEN_STRING " + self.cname + " =\n"
        string += "{\n"
        string += "\t.x_pos = " + str(self.x_pos) + ",\n"
        string += "\t.y_pos = " + str(self.y_pos) + ",\n"
        string += "\t.x_size = " + str(self.x_size) + ",\n"
        string += "\t.y_size = " + str(self.y_size) + ",\n"
        string += "\t.font = &" + self.font.cname + ",\n"

        if self.first_condition is None:
            string += "\t.first_condition = NULL,\n"
        else:
            string += "\t.first_condition = &" + self.first_condition.cname + ",\n"
        if self.offset is None:
            string += "\t.type = SCREEN_STRING_TYPE_VALUE,\n"
            string += "\t.value =\n"
        else:
            string += "\t.type = SCREEN_STRING_TYPE_VALUE_WITH_OFFSET,\n"
            string += "\t.value_offset =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t\t.format_string = " + self.format_string + ",\n"
        if self.offset is not None:
            string += "\t\t.offset = " + self.offset[0] + ",\n"
            string += "\t\t.len = " + self.offset[1] + ",\n"
        string += "\t},\n"

        if self.next_string is None:
            string += "\t.next_string = NULL, \n"
        else:
            string += "\t.next_string = &" + self.next_string.cname + ",\n"

        string += "};\n"
        return string

    def get_pos(self):
        """Get value position and size."""
        return (self.x_pos, self.y_pos, self.x_size, self.y_size)

    def set_pos(self, rotation, x_pos, y_pos, x_size, y_size):
        """Set value position and size. Rotate provided coordinates if requested.

        set_pos() sets the value string position. If rotation is not 0deg coordinates
        are first translated into the rotated coordinate frame. This rotation is used
        to compensate for the display not being mounted in the correct orientation on
        the HMI hardware.

        Parameters
        ----------
        rotation : { "0deg", "90deg", "180deg", "270deg" }
            Rotation applied to value string coordinates.
        x_pos, y_pos : int
            Top left corner of value string. In pixels, Before rotation.
        x_size, y_size : int
            Maximum size of value string. In pixels, Before rotation.

        """
        if rotation == "0deg":
            self.x_pos = x_pos
            self.y_pos = y_pos
            self.x_size = x_size
            self.y_size = y_size
        elif rotation == "90deg":
            self.x_pos = y_pos
            self.y_pos = DISPLAY_LENGTH - x_pos - x_size
            self.x_size = y_size
            self.y_size = x_size
        elif rotation == "180deg":
            self.x_pos = DISPLAY_LENGTH - x_pos - x_size
            self.y_pos = DISPLAY_LENGTH - y_pos - y_size
            self.x_size = x_size
            self.y_size = y_size
        elif rotation == "270deg":
            self.x_pos = DISPLAY_LENGTH - y_pos - y_size
            self.y_pos = x_pos
            self.x_size = y_size
            self.y_size = x_size
        else:
            raise ValueError("{} is not a valid rotation value.".format(rotation))


class Event:

    def __init__(self, index, state_index, type, name, args, variables):
        self.index = index
        self.state_index = state_index
        self.type = type
        self.name = name
        self.internal = False

        self.event_name = "MENU_EVENT_" + str.upper(name)
        self.type_name = "EVENT_TYPE_" + str.upper(type)
        self.def_name = str.upper(name)

        if type == "key_press":
            if len(args) < 1:
                raise EventError("Insufficient parameters for key press event.")
            self.keys = args

        elif type == "key_release":
            if len(args) < 1:
                raise EventError("Insufficient parameters for key release event.")
            self.keys = args

        elif type == "key_hold":
            if len(args) < 3:
                raise EventError("Insufficient parameters for key hold event.")
            self.delay = args[0]
            self.repeat = args[1]
            self.keys = args[2:]

        elif type == "rotation":
            if len(args) < 2:
                raise EventError("Insufficient parameters for rotation event.")
            self.encoder = args[0]
            self.steps = args[1]
            if len(args) == 2:
               self.keys = None
            else:
                self.keys = args[2:]

        elif type == "var_change":
            if len(args) < 1:
                raise EventError("Insufficient parameters for var change event.")

            var_names = args

            # Translate variable name to varstate index.
            self.vars = []
            for name in var_names:
                if name in variables:
                    self.vars.append(variables[name].index)
                else:
                    raise EventError("Variable " + name + " not defined.")

        elif type == "state_entry":
            if len(args) > 0:
                raise EventError("Too many parameters for state entry event.")

        elif type == "state_exit":
            if len(args) > 0:
                raise EventError("Too many parameters for state exit event.")

        elif type == "idle":
            if len(args) != 1:
                raise EventError("Wrong number of parameters for idle event.")
            self.delay = args[0]
            # Check if immediate value or variable reference is used
            if args[0] in variables:
                self.delay = variables[args[0]].index | 0x80000000

        elif type == "triggered":
            self.internal = True
            if len(args) != 0:
                raise EventError("Wrong number of parameters for triggered event.")

        elif type == "delay":
            if len(args) != 2:
                raise EventError("Wrong number of parameters for delay event.")
            self.delay = args[0]
            # Check if immediate value or variable reference is used
            if args[0] in variables:
                self.delay = variables[args[0]].index | 0x80000000
            self.repeat = args[1]

        else:
            raise EventError("Unknown event type.")

    def c_repr(self):
        string = "\t[" + self.def_name + "] =\n\t{\n"
        string += "#ifdef HMI_EVENT_DEBUG_NAMES\n"
        string += "\t\t.name = \"" + self.def_name + "\",\n"
        string += "#endif\n"
        string += "\t\t.type = " + self.type_name + ",\n"
        string += "\t\t.state_index = " + str(self.state_index) + ",\n"
        string += "\t\t.menu_event = " + self.event_name + ",\n"

        if self.type == "key_press":
            string += "\t\t.key_press =\n\t\t{\n"
            string += "\t\t\t.num_buttons = " + str(len(self.keys)) + ",\n"
            string += "\t\t\t.buttons = { " + ", ".join(map(str, self.keys)) + " },\n"
            string += "\t\t},\n"

        elif self.type == "key_release":
            string += "\t\t.key_press =\n\t\t{\n"
            string += "\t\t\t.num_buttons = " + str(len(self.keys)) + ",\n"
            string += "\t\t\t.buttons = { " + ", ".join(map(str, self.keys)) + " },\n"
            string += "\t\t},\n"

        elif self.type == "key_hold":
            string += "\t\t.key_hold =\n\t\t{\n"
            string += "\t\t\t.repeat = " + str(self.repeat) + ",\n"
            string += "\t\t\t.num_buttons = " + str(len(self.keys)) + ",\n"
            string += "\t\t\t.buttons = { " + ", ".join(map(str, self.keys)) + " },\n"
            string += "\t\t\t.delay = " + str(self.delay) + ",\n"
            string += "\t\t},\n"

        elif self.type == "rotation":
            string += "\t\t.rotation =\n\t\t{\n"
            string += "\t\t\t.encoder = " + str(self.encoder) + ",\n"
            string += "\t\t\t.steps = " + str(self.steps) + ",\n"
            if self.keys is None:
                string += "\t\t\t.num_buttons = 0,\n"
                string += "\t\t\t.buttons = { 0 },\n"
            else:
                string += "\t\t\t.num_buttons = " + str(len(self.keys)) + ",\n"
                string += "\t\t\t.buttons = { " + ", ".join(map(str, self.keys)) + " },\n"
            string += "\t\t},\n"

        elif self.type == "var_change":
            string += "\t\t.var_change = {\n"
            string += "\t\t\t.num_vars = " + str(len(self.vars)) + ",\n"
            string += "\t\t\t.vars = { " + ", ".join(map(str, self.vars)) + " },\n"
            string += "\t\t},\n"

        elif self.type == "triggered":
            None

        elif self.type == "state_entry":
            None

        elif self.type == "state_exit":
            None

        elif self.type == "idle":
            string += "\t\t.idle_event =\n\t\t{\n"
            string += "\t\t\t.delay = " + str(self.delay) + "\n"
            string += "\t\t},\n"

        elif self.type == "delay":
            string += "\t\t.delay_event =\n\t\t{\n"
            string += "\t\t\t.repeat = " + str(self.repeat) + ",\n"
            string += "\t\t\t.delay = " + str(self.delay) + "\n"
            string += "\t\t},\n"

        string += "\t},\n"
        return string


class EventError(Exception):
    def __init__(self, message):
        super().__init__(message)
        self.message = message

