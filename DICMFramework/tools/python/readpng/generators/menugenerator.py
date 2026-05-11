"""Provides MenuGenerator class used to parse variable definition file."""

import os
import re

from lib.defreader import DefReader, DefError
from deftypes.misc import State, Event
from deftypes.actions import *

class MenuGenerator:
    """Generates menu data for the HMI engine from definition files.

    A MenuGenerator is used to parse HMI menu definition files. Definitions
    are read in from the directory specified during __init__ and then provided to
    other generators trough the states attribute.

    The write_menu_data method is used to write out contained data to .c/.h files
    in a format understood by the HMI engine.

    Attributes
    ----------
    states : Dict of (str: State)
        A dict of all generated menu states indexed by state name.

    Methods
    -------
    write_menu_data
        Write MenuGenerator contents to file for use by HMI engine.

    """

    def __init__(self, menu_path, variables, events, screens):
        """Initializer for MenuGenerator class.

        New MenuGenerator will contain one State object for each .txt file found
        in directory given by menu_path. Each State will have the same name as the
        definition file it was generated from.

        Name-indexed dicts of available variables, events, and screens must be given.
        These will typically be provided by their respective generators.

        Parameters
        ----------
        menu_path : str
            Path to folder containing menu definition files.
        variables : Dict of (str: Variable)
            A dict of available variables indexed by variable name.
        events : Dict of (str : Event)
            A dict of available menu events indexed by event name.
        screens : Dict of (str: Screen)
            A dict of available screens indexed by screen name.

        Raises
        ------
        DefError
            Raised if MenuGenerator fails to parse any definition file.

        """

        self.states = {}
        self._actions = {}

        self._variables = variables
        self._events = events
        self._screens = screens
        self._boot_state = None
        self._global_state = None

        # First create the full list of states to have targets for state change actions.
        for file_name in os.listdir(menu_path):
            file = os.path.join(menu_path, file_name)
            if os.path.isfile(file) and os.path.splitext(file_name)[1].lower() == ".txt":
                new_state = State(os.path.splitext(file_name)[0].lower(), file)
                self.states[new_state.name] = new_state

        # Populate all states with screens and actions.
        for state in self.states.values():
            self._parse_state(state)

        if self._boot_state is None:
            raise DefError("No boot state specified.", None, None)
        if self._global_state is None:
            print("No global state specified.")

    def _add_action(self, line, reader):
        """ Returns a new Action object created from definition in line.

        New Action will also be added to MenuGenerator dict of actions
        indexed by action cname.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new action.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        Action
            New Action object.

        """

        # Create new action.
        if line[1] == "state":
            if len(line) != 3:
                raise DefError("Incorrect number of parameters for change menu state definition.", reader.name, reader.line_no)
            if line[2] not in self.states:
                raise DefError("Could not find state {}".format(line[2]), reader.name, reader.line_no)
            action = StateChange(self.states[line[2]])

        elif line[1] == "var_set":
            if len(line) != 4:
                raise DefError("Incorrect number of parameters for variable set definition.", reader.name, reader.line_no)
            if line[2] not in self._variables:
                raise DefError("Could not find variable {}".format(line[2]), reader.name, reader.line_no)
            var = self._variables[line[2]]
            action = VarSet(var, var.scale_step(line[3], reader))

        elif line[1] == "var_change":
            if len(line) != 5:
                raise DefError("Incorrect number of parameters for variable change definition.", reader.name, reader.line_no)
            if line[2] not in self._variables:
                raise DefError("Could not find variable {}".format(line[2]), reader.name, reader.line_no)
            var = self._variables[line[2]]
            action = VarChange(var, var.scale_step(line[3], reader), var.scale_step(line[4], reader))

        elif line[1] == "leap_year":
            if len(line) != 4:
                raise DefError("Incorrect number of parameters for variable change definition.", reader.name, reader.line_no)
            if line[2] not in self._variables:
                raise DefError("Could not find variable {}".format(line[2]), reader.name, reader.line_no)
            if line[3] not in self._variables:
                raise DefError("Could not find variable {}".format(line[3]), reader.name, reader.line_no)
            action = LeapYear(self._variables[line[2]], self._variables[line[3]])

        elif line[1] == "var_copy":
            if len(line) != 4:
                raise DefError("Incorrect number of parameters for variable copy definition.", reader.name, reader.line_no)
            if line[2] not in self._variables:
                raise DefError("Could not find variable {}".format(line[2]), reader.name, reader.line_no)
            # Check offset and len specification format <variable>[:offset:len]
            basevar = line[3]
            offset = basevar.split(":")
            if len(offset) > 1:
                basevar = offset[0]
                if len(offset) != 3:
                    print(offset)
                    raise DefError("offset:length not specified", reader.name, reader.line_no)
                offset = offset[1:]
            else:
                offset = None    
            
            if basevar not in self._variables:
                raise DefError("Could not find variable {}".format(line[3]), reader.name, reader.line_no)
            action = VarCopy(self._variables[line[2]], self._variables[basevar], offset)

        elif line[1] == "ddmp_set":
            if len(line) != 4:
                raise DefError("Incorrect number of parameters for DDMP set definition.", reader.name, reader.line_no)
            if line[2] not in self._variables:
                raise DefError("Could not find variable {}".format(line[2]), reader.name, reader.line_no)
            if line[3] not in self._variables:
                raise DefError("Could not find variable {}".format(line[3]), reader.name, reader.line_no)
            action = DDMPSet(self._variables[line[2]], self._variables[line[3]])

        elif line[1] == "idle_reset":
            if len(line) != 2:
                raise DefError("Incorrect number of parameters for idle reset definition.", reader.name, reader.line_no)
            action = IdleReset()
        elif line[1] == "no_action":
            if len(line) != 2:
                raise DefError("Incorrect number of parameters for no action definition.", reader.name, reader.line_no)
            action = NoAction()
        elif line[1] == "delayed_start":
            if len(line) != 4:
                raise DefError("Incorrect number of parameters for delayed start definition.", reader.name, reader.line_no)
            if line[2] not in self._events:
                raise DefError("Could not find event {}".format(line[2]), reader.name, reader.line_no)
            if int(line[3]) < 1:
                raise DefError("Delay must be one or higher.", reader.name, reader.line_no)
            action = DelayedStart(self._events[line[2]], int(line[3]))

        elif line[1] == "delayed_stop":
            if len(line) != 3:
                raise DefError("Incorrect number of parameters for delayed stop definition.", reader.name, reader.line_no)
            if line[2] not in self._events:
                raise DefError("Could not find event {}".format(line[2]), reader.name, reader.line_no)
            action = DelayedStop(self._events[line[2]])

        elif line[1] == "display_on":
            if len(line) != 2:
                raise DefError("Incorrect number of parameters for display on definition.", reader.name, reader.line_no)
            action = DisplayOn()

        elif line[1] == "display_off":
            if len(line) != 2:
                raise DefError("Incorrect number of parameters for display off definition.", reader.name, reader.line_no)
            action = DisplayOff()
        
        elif line[1] == "timer_stop":
            if len(line) != 3:
                raise DefError("Incorrect number of parameters for timer stop definition.", reader.name, reader.line_no)
            if line[2] not in self._events:
                raise DefError("Could not find event {}".format(line[2]), reader.name, reader.line_no)
            action = TimerStop(self._events[line[2]])
        
        elif line[1] == "timer_start":
            if len(line) != 3:
                raise DefError("Incorrect number of parameters for timer start definition.", reader.name, reader.line_no)
            if line[2] not in self._events:
                raise DefError("Could not find event {}".format(line[2]), reader.name, reader.line_no)
            action = TimerStart(self._events[line[2]])

        elif line[1] == "display_brightness_set":
            if len(line) != 3:
                raise DefError("Incorrect number of parameters for display brightness set definition.", reader.name, reader.line_no)
            if line[2] not in self._variables:
                raise DefError("Could not find variable {}".format(line[2]), reader.name, reader.line_no)
            action = DisplayBrightnessSet(self._variables[line[2]])

        elif line[1] == "save_hmi_settings":
            if len(line) != 2:
                raise DefError("Incorrect number of parameters for save hmi settings definition.", reader.name, reader.line_no)
            action = SaveHMISettings()

        else:
            raise DefError("Unknown action type", reader.name, reader.line_no)

        # Find a name that's not taken.
        suffix = 0
        while action.cname in self._actions:
            suffix += 1
            if suffix <= 10:
                action.cname = action.cname[:-1] + str(suffix)
            elif suffix <= 100:
                action.cname = action.cname[:-2] + str(suffix)
            else:
                action.cname = action.cname[:-3] + str(suffix)

        #print("\t\t With condition: " + condition.cname)

        # Add to list of all actions.
        self._actions[action.cname] = action

        return action

    def _add_branch(self, line, reader):
        """ Returns a new Branch object created from definition in line.

        New Branch will also be added to MenuGenerator dict of actions
        indexed by branch cname.

        Parameters
        ----------
        line : Tuple of str
            Definition line for new branch.
        reader : DefReader
            DefReader instance being parsed.

        Returns
        -------
        Branch
            New Branch object.

        """

        # Create new branch action
        (cnd_type, var, value, offset) = reader.parse_condition(line[1], self._variables)
        if cnd_type == "==":
            action = BranchEqual(var, value, offset)
        elif cnd_type == "!=":
            action = BranchNotEqual(var, value, offset)
        elif cnd_type == "<=":
            action = BranchLessEqual(var, value, offset)
        elif cnd_type == ">=":
            action = BranchMoreEqual(var, value, offset)
        elif cnd_type == "<":
            action = BranchLess(var, value, offset)
        elif cnd_type == ">":
            action = BranchMore(var, value, offset)
        else:
            raise DefError("{} is not valid branch type.".format(cnd_type), reader.name, reader.line_no)

        # Find a name that's not taken.
        suffix = 0
        while action.cname in self._actions:
            suffix += 1
            if suffix <= 10:
                action.cname = action.cname[:-1] + str(suffix)
            elif suffix <= 100:
                action.cname = action.cname[:-2] + str(suffix)
            else:
                action.cname = action.cname[:-3] + str(suffix)

        #print("\t\t With action: " + action.cname)

        # Add branch action to list of all actions.
        self._actions[action.cname] = action

        return action

    def _parse_state(self, state):
        """Initializes fields in state from its definition file.

        Arguments
        ---------
        state : State
            State object to be initialized.

        """

        event = None
        previous_action = None
        new_action = None
        branches = []
        branch_ends = []
        with DefReader(state.file) as state_def:
            for line in state_def:

                # Parse menu definition line.
                if line[0] == "boot_state":
                    if self._boot_state is not None:
                        raise DefError("Found multiple boot state declarations. Only one allowed.", state_def.name, state_def.line_no)
                    self._boot_state = state
                # Parse menu definition line.
                elif line[0] == "global_state":
                    if self._global_state is not None:
                        raise DefError("Found multiple global state declarations. Only one allowed.", state_def.name, state_def.line_no)
                    self._global_state = state

                elif line[0] == "screen":
                    if len(line) != 2:
                        raise DefError("Incorrect number of parameters for screen definition.", state_def.name, state_def.line_no)
                    if state.screen is not None:
                        raise DefError("Menu state can only have one screen.", state_def.name, state_def.line_no)
                    if line[1] not in self._screens:
                        raise DefError("Could not find screen {}".format(line[1]), state_def.name, state_def.line_no)
                    state.screen = self._screens[line[1]]

                elif line[0] == "event":
                    if len(line) != 2:
                        raise DefError("Incorrect number of parameters for event definition.", state_def.name, state_def.line_no)
                    if line[1] not in self._events:
                        raise DefError("Could not find event.", state_def.name, state_def.line_no)
                    event = self._events[line[1]]
                    previous_action = None
                    new_action = None
                    branches = []
                    branch_ends = []

                elif line[0] == "action":
                    if len(line) < 2:
                        raise DefError("Incorrect number of parameters for action definition.", state_def.name, state_def.line_no)
                    new_action = self._add_action(line, state_def)
                    #print("Parsing " + new_action.cname);

                elif line[0] == "branch":
                    if len(line) != 2:
                        raise DefError("Incorrect number of parameters for branch definition.", state_def.name, state_def.line_no)
                    new_action = self._add_branch(line, state_def)
                    #branch = True
                    #print("Parsing " + new_action.cname);
                    #print("Append branch action " + new_action.cname)
                    branches.append(new_action)

                elif line[0] == "branch_end":
                    #print("Parsing branch_end");
                    if len(line) != 1:
                        raise DefError("Incorrect number of parameters for branch_end definition.", state_def.name, state_def.line_no)
                    if len(branches) == 0:
                        raise DefError("Could not find matching branch definition", state_def.name, state_def.line_no)
                    #previous_action = branches.pop()
                    if previous_action is None:
                        raise DefError("Could not find previous_action definition", state_def.name, state_def.line_no)
                    a = branches.pop()
                    branch_ends.append(a)
                    #print ("Append in branch_ends action " + a.cname)

                else:
                    raise DefError("Unknown menu definition.", state_def.name, state_def.line_no)

                # Link new action to correct parent.
                if new_action is not None:

                    #print("Set last_action " + last_action.cname + ".next_action to " + new_action.cname)
                    if previous_action is None:
                        # First action in each event
                        if event is None:
                            raise DefError("Action must belong to an event.", state_def.name, state_def.line_no)
                        else:
                            #print ("\nAdded new action "+  new_action.cname + " to state " + state.cname)
                            print("Added handling of event " + event.def_name + " in state " + state.cname)
                            state.actions[event] = new_action
                            event = None
                    else:
                        # Normal handling of actions
                        if isinstance(previous_action, Branch):
                            previous_action.target = new_action
                            #print("Set previous_action(1) " + previous_action.cname + ".target to " + new_action.cname)
                        else:
                            # Branch handling
                            while len(branch_ends) > 0:
                                b = branch_ends.pop()
                                b.next_action = new_action
                                #print("Set branch next_action(2) " + b.cname + ".next_action to " + new_action.cname)
                            previous_action.next_action = new_action
                            #print("Set previous_action(2) " + previous_action.cname + ".next_action to " + new_action.cname)
                    previous_action = new_action
                    new_action = None
                        

    def write_menu_data(self, header_file, source_file):
        """Exports MenuGenerator contents in a format understood by the HMI engine.

        MenuGenerator contents will be written to source_file while header_file
        will extern names needed by other HMI data files.

        Parameters
        ----------
        header_file, source_file : str
            Full paths to source and header file to be written.

        """

        with open(header_file, "w") as f:
            guarddef = re.sub('[^A-Za-z0-9]', '_', os.path.basename(header_file).upper()) + '_'

            f.write("/*! \\file " + os.path.basename(header_file) + "\n")
            f.write(" *  \\brief Menu data header. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("#ifndef " + guarddef + "\n")
            f.write("#define " + guarddef + "\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write('#include <stdint.h>\n')
            f.write('#include <stdbool.h>\n')
            f.write('#include "screen_data.h"\n')
            f.write("\n")
            f.write("/** Defines *******************************************************************/\n")
            f.write("#define MAX_MENU_EVENTS 45  //!< Maximum allowed number of menu events.\n")
            f.write("\n")
            f.write("/** Typedefs ******************************************************************/\n")
            f.write("\n")
            f.write("//! Enumeration of all possible menu action types.\n")
            f.write("typedef enum MENU_ACTION_TYPE\n")
            f.write("{\n")
            f.write("\tMENU_ACTION_BRANCH_EQUAL,                   //!< Take target action if state value =  value.\n")
            f.write("\tMENU_ACTION_BRANCH_NOT_EQUAL,               //!< Take target action if state value != value.\n")
            f.write("\tMENU_ACTION_BRANCH_LESS_THAN,               //!< Take target action if state value <  value.\n")
            f.write("\tMENU_ACTION_BRANCH_LESS_THAN_OR_EQUAL,      //!< Take target action if state value <= value.\n")
            f.write("\tMENU_ACTION_BRANCH_GREATER_THAN,            //!< Take target action if state value >  value.\n")
            f.write("\tMENU_ACTION_BRANCH_GREATER_THAN_OR_EQUAL,   //!< Take target action if state value >= value.\n")
            f.write("\tMENU_ACTION_STATE_SET,                      //!< Set menu state to new_state.\n")
            f.write("\tMENU_ACTION_VAR_SET,                        //!< Set state variable with var_index to value.\n")
            f.write("\tMENU_ACTION_VAR_CHANGE,                     //!< Change value of state variable with var_index by value.\n")
            f.write("\tMENU_ACTION_LEAP_YEAR,                      //!< Check if current year is leap year \n")
            f.write("\tMENU_ACTION_VAR_COPY,                       //!< Copy value from source to  variable.\n")
            f.write("\tMENU_ACTION_DDMP_SET,                       //!< Send DDMP set with value equal to indexed var.\n")
            f.write("\tMENU_ACTION_IDLE_RESET,                     //!< Reset idle timer for HMIs without buttons.\n")
            f.write("\tMENU_ACTION_DELAYED_START,                  //!< Register a delayed event.\n")
            f.write("\tMENU_ACTION_DELAYED_STOP,                   //!< Cancel a delayed event.\n")
            f.write("\tMENU_ACTION_TIMER_START,                    //!< Start/restart a timer (delay) event.\n")
            f.write("\tMENU_ACTION_TIMER_STOP,                     //!< Start a timer (delay) event.\n")
            f.write("\tMENU_ACTION_DISPLAY_ON,                     //!< Makes display controller leave sleep mode.\n")
            f.write("\tMENU_ACTION_DISPLAY_OFF,                    //!< Put display controller in sleep mode.\n")
            f.write("\tMENU_ACTION_DISPLAY_BRIGHTNESS_SET,         //!< Set display brightness equal to indexed var.\n")
            f.write("\tMENU_ACTION_SAVE_HMI_SETTINGS,              //!< Save HMI settings to non-volatile memory\n")
            f.write("\tMENU_ACTION_VAR_COPY_OFFSET,                //!< Copy value from source (with offset:len) to variable.\n")
            f.write("\tMENU_ACTION_NO_ACTION                       //!< no action definition\n")
            f.write("} MENU_ACTION_TYPE;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a branch type menu action.\n")
            f.write("typedef struct MENU_DATA_BRANCH\n")
            f.write("{\n")
            f.write("\tconst struct MENU_ACTION *target;   //!< Next action to take if branch condition is true,\n")
            f.write("\tint32_t value;                      //!< Value to compare against system state.\n")
            f.write("\tuint8_t var_index;                  //!< Index of state variable to compare with.\n")
            f.write("\tuint8_t offset;                     //!< Offset in bytes of data in variable.\n")
            f.write("\tuint8_t len;                        //!< Length in bytes of data in variable.\n")
            f.write("} MENU_DATA_BRANCH;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a variable set type menu action.\n")
            f.write("typedef struct MENU_DATA_VAR_SET\n")
            f.write("{\n")
            f.write("\tint32_t value;      //!< New state variable value.\n")
            f.write("\tuint8_t var_index;  //!< Index of state variable to change.\n")
            f.write("} MENU_DATA_VAR_SET;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a variable change type menu action.\n")
            f.write("typedef struct MENU_DATA_VAR_CHANGE\n")
            f.write("{\n")
            f.write("\tint32_t value;      //!< Amount to increase state variable with. Negative values decrease.\n")
            f.write("\tint32_t limit;      //!< Upper limit beyond which state variable should not increase further. Lower limit for negative changes.\n")
            f.write("\tuint8_t var_index;  //!< Index of state variable to change.\n")
            f.write("} MENU_DATA_VAR_CHANGE;\n")
            f.write("\n")
            f.write("//! Struct Handling leap year.\n")
            f.write("typedef struct MENU_DATA_LEAP_YEAR\n")
            f.write("{\n")
            f.write("\tuint8_t current_year_index; //!< Current year value index. \n")
            f.write("\tuint8_t var_index;          //!< Return variable index. \n")
            f.write("} MENU_DATA_LEAP_YEAR;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a variable copy type menu action.\n")
            f.write("typedef struct MENU_DATA_VAR_COPY\n")
            f.write("{\n")
            f.write("\tfloat scale_factor;     //!< Scale factor to convert from source to target.\n")
            f.write("\tuint8_t var_index;      //!< Index of state variable to change.\n")
            f.write("\tuint8_t source_index;   //!< Index of state variable to copy from.\n")
            f.write("} MENU_DATA_VAR_COPY;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a variable copy (with offset:len) type menu action.\n")
            f.write("typedef struct MENU_DATA_VAR_COPY_OFFSET\n")
            f.write("{\n")
            f.write("\tfloat scale_factor;     //!< Scale factor to convert from source to target.\n")
            f.write("\tuint8_t var_index;      //!< Index of state variable to change.\n")
            f.write("\tuint8_t source_index;   //!< Index of state variable to copy from.\n")
            f.write("\tuint8_t offset;         //!< Offset of data to copy from.\n")
            f.write("\tuint8_t len;            //!< Length of data to copy.\n")
            f.write("} MENU_DATA_VAR_COPY_OFFSET;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a ddmp set type menu action.\n")
            f.write("typedef struct MENU_DATA_DDMP_SET\n")
            f.write("{\n")
            f.write("\tuint32_t parameter_id;              //!< Id of parameter to set.\n")
            f.write("\tuint8_t parameter_length;           //!< DDMP2 parameter data length.\n")
            f.write("\tuint8_t var_index;                  //!< Index of var to read set value from.\n")
            f.write("} MENU_DATA_DDMP_SET;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a ddmp set type menu action.\n")
            f.write("typedef struct MENU_DATA_DELAYED_START\n")
            f.write("{\n")
            f.write("\tuint8_t event_index;                //!< Index of event to trigger.\n")
            f.write("\tuint32_t delay;                     //!< Delay in ms.\n")
            f.write("} MENU_DATA_DELAYED_START;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a ddmp set type menu action.\n")
            f.write("typedef struct MENU_DATA_DELAYED_STOP\n")
            f.write("{\n")
            f.write("\tuint8_t event_index;                //!< Index of delayed event to cancel.\n")
            f.write("} MENU_DATA_DELAYED_STOP;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a ddmp set type menu action.\n")
            f.write("typedef struct MENU_DATA_TIMER_START\n")
            f.write("{\n")
            f.write("\tuint8_t event_index;                //!< Index of delayed event to cancel.\n")
            f.write("} MENU_DATA_TIMER_START;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a ddmp set type menu action.\n")
            f.write("typedef struct MENU_DATA_TIMER_STOP\n")
            f.write("{\n")
            f.write("\tuint8_t event_index;                //!< Index of delayed event to cancel.\n")
            f.write("} MENU_DATA_TIMER_STOP;\n")
            f.write("\n")
            f.write("//! Struct holding type-specific data for a display brightness set type menu action.\n")
            f.write("typedef struct MENU_DATA_DISPLAY_BRIGHTNESS_SET\n")
            f.write("{\n")
            f.write("\tuint8_t var_index;                  //!< Index of var to read brightness value from.\n")
            f.write("} MENU_DATA_DISPLAY_BRIGHTNESS_SET;\n")
            f.write("\n")
            f.write("//! Struct defining a single menu action.\n")
            f.write("typedef struct MENU_ACTION\n")
            f.write("{\n")
            f.write("#ifdef HMI_MENU_DEBUG_NAMES\n")
            f.write("\tconst char *name;                       //!< Name of state, for debugging\n")
            f.write("#endif\n")
            f.write("\tconst struct MENU_ACTION *next_action;  //!< Next action in linked list. NULL if this is last action in list.\n")
            f.write("\tunion\n\t{\n")
            f.write("\t\tMENU_DATA_BRANCH branch;            //!< Type-specific data for branch actions.\n")
            f.write("\t\tconst struct MENU_STATE *new_state; //!< Type-specific data for change state actions.\n")
            f.write("\t\tMENU_DATA_VAR_SET var_set;          //!< Type-specific data for var_set or actions.\n")
            f.write("\t\tMENU_DATA_VAR_CHANGE var_change;    //!< Type-specific data for var_change actions.\n")
            f.write("\t\tMENU_DATA_LEAP_YEAR leap_year;      //!< Type-specific data for leap_year actions.\n")
            f.write("\t\tMENU_DATA_VAR_COPY var_copy;        //!< Type-specific data for var_copy actions.\n")
            f.write("\t\tMENU_DATA_VAR_COPY_OFFSET var_copy_offset; //!< Type-specific data for var_copy_offset actions.\n")
            f.write("\t\tMENU_DATA_DDMP_SET ddmp_set;        //!< Type-specific data for ddmp_set actions.\n")
            f.write("\t\tMENU_DATA_DELAYED_START delayed_start;  //!< Type-specific data for delayed_start actions.\n")
            f.write("\t\tMENU_DATA_DELAYED_STOP delayed_stop;    //!< Type-specific data for delayed_start actions.\n")
            f.write("\t\tMENU_DATA_TIMER_START timer_start;  //!< Type-specific data for timer_start actions.\n")
            f.write("\t\tMENU_DATA_TIMER_STOP timer_stop;    //!< Type-specific data for timer_stop actions.\n")
            f.write("\t\tMENU_DATA_DISPLAY_BRIGHTNESS_SET display_brightness_set;    //!< Type-specific data for brightness_set actions.\n")
            f.write("\t};\n")
            f.write("\tMENU_ACTION_TYPE type;              //!< Action type.\n")
            f.write("} MENU_ACTION;\n")
            f.write("\n")
            f.write("//! Struct defining a single menu state.\n")
            f.write("typedef struct MENU_STATE\n")
            f.write("{\n")
            f.write("#ifdef HMI_MENU_DEBUG_NAMES\n")
            f.write("\tconst char *name;                               //!< Name of state, for debugging\n")
            f.write("#endif\n")
            f.write("\tconst SCREEN_DEF *screen;                       //!< Screen to show in menu state.\n")
            f.write("\tconst MENU_ACTION *actions[MAX_MENU_EVENTS];    //!< Array mapping event indexes to actions.\n")
            f.write("} MENU_STATE;\n")
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            #f.write("extern const MENU_STATE *menu_data_boot_state;\n")
            #f.write("extern const MENU_STATE *menu_global_state;\n")
            f.write("\n")
            f.write("#endif /* " + guarddef + " */\n")

        with open(source_file, "w") as f:
            f.write("/* clang-format off */" + "\n")
            f.write("/*! \\file " + os.path.basename(source_file) + "\n")
            f.write(" *  \\brief Menu data definitions. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write("\n")
            f.write("#include <stdint.h>\n")
            f.write("#include <stddef.h>\n")
            f.write("\n")
            f.write('#include "screen_data.h"\n')
            f.write('#include "menu_data.h"\n')
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            f.write("/** State names ***************************************************************/\n")

            for state in self.states.values():
                f.write("static const MENU_STATE " + state.cname + ";\n")

            f.write("\n")
            f.write("/** Actions *******************************************************************/\n")

            # Actions contain pointers to other Actions which means c definitions
            # must be written to file in the correct order to not generate GCC
            # errors.

            actions = list(self._actions.values())  # This creates a copy.
            cnames = []
            while len(actions) != 0:
                for action in actions:
                    if (action.next_action is None or action.next_action.cname in cnames) and (not isinstance(action, Branch)
                        or (action.target is None or action.target.cname in cnames)):
                        f.write(action.c_repr())
                        f.write("\n")
                        actions.remove(action)
                        cnames.append(action.cname)

            f.write("/** States ********************************************************************/\n")

            for state in self.states.values():
                f.write(state.c_repr())
                f.write("\n")

            f.write("const MENU_STATE * const menu_data_boot_state = &" + self._boot_state.cname + ";\n")
            if self._global_state is not None:
                f.write("const MENU_STATE * const menu_global_state = &" + self._global_state.cname + ";\n")
            else:
                f.write("const MENU_STATE * const menu_global_state = NULL;\n")
