
from genericpath import exists
import os
import re

from lib.defreader import DefReader, DefError
from deftypes.misc import Event, EventError


class EventGenerator:

    def __init__(self, event_file, variables):

        self.events = {}

        if not os.path.isfile(event_file):
            raise ValueError("Event definition file not found.")
        if not os.path.splitext(event_file)[1].lower() == ".txt":
            raise ValueError("Event definition file is not .txt")

        self._add_events(event_file, variables)

    def _add_events(self, event_file, variables):
        with DefReader(event_file) as event_def:
            state_index = {}
            menu_ev_index = 1
            # Add builtin events state_entry and state_exit
            # state_entry
            state_index['state_entry'] = 0
            try:
                ev = Event(menu_ev_index, 0, 'state_entry', 'state_entry', [], variables)
            except EventError as err:
                raise DefError(err.message, event_def.name, event_def.line_no)
            menu_ev_index += 1
            self.events[ev.name] = ev
            print("Added event: " + ev.name)
            # state_exit
            state_index['state_exit'] = 0
            try:
                ev = Event(menu_ev_index, 0, 'state_exit', 'state_exit', [], variables)
            except EventError as err:
                raise DefError(err.message, event_def.name, event_def.line_no)
            menu_ev_index += 1
            self.events[ev.name] = ev
            print("Added event: " + ev.name)

            for line in event_def:
                # Extract parameters from line.
                if len(line) < 2:
                    raise DefError("Insufficient parameters for event definition.", event_def.name, event_def.line_no)

                ev_type = line[0]
                ev_name = line[1]
                ev_args = line[2:]

                # Keep track of state index, number of times each type of event is defined.
                if ev_type in state_index:
                    state_index[ev_type] += 1
                else:
                    state_index[ev_type] = 0

                st_index = state_index[ev_type]

                # Create and add event.
                try:
                    ev = Event(menu_ev_index, st_index, ev_type, ev_name, ev_args, variables)
                except EventError as err:
                    raise DefError(err.message, event_def.name, event_def.line_no)

                if ev.name in self.events:
                    raise DefError("Event " + ev.name + " already defined.", event_def.name, event_def.line_no)

                menu_ev_index += 1
                self.events[ev.name] = ev
                print("Added event: " + ev.name)




    def write_event_data(self, header_file, source_file):

        with open(header_file, "w") as f:
            guarddef = re.sub('[^A-Za-z0-9]', '_', os.path.basename(header_file).upper()) + '_'

            f.write("/*! \\file " + os.path.basename(header_file) + "\n")
            f.write(" *  \\brief Full Climate event definition headers. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("#ifndef " + guarddef + "\n")
            f.write("#define " + guarddef + "\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write('#include <stdint.h>\n')
            f.write('#include <stdbool.h>\n')
            f.write("\n")
            f.write("/** Defines *******************************************************************/\n")
            f.write("\n")
            f.write("#define EVENT_MAX_BUTTONS               6   //!< Maximum number of buttons allowed for an event.\n")
            f.write("#define EVENT_MAX_VARS                  10  //!< Maximum number of variables in a varstate event.\n")
            f.write("\n")
            f.write("/** Typedefs ******************************************************************/\n")
            f.write("\n")
            f.write("//! Human readable names for connected buttons.\n")
            f.write("typedef enum EVENT_BUTTON_INDEX\n")
            f.write("{\n")
            f.write("\tEVENT_BUTTON_LEFT,\n")
            f.write("\tEVENT_BUTTON_MIDDLE,\n")
            f.write("\tEVENT_BUTTON_RIGHT,\n")
            f.write("\tEVENT_BUTTON_OK,\n")
            f.write("#ifdef CONNECTOR_HMI_BUTTON_ONOFF_PIN\n")
            f.write("\tEVENT_BUTTON_ONOFF,\n")
            f.write("#endif\n")
            f.write("\tEVENT_NUM_BUTTONS\n")
            f.write("} EVENT_BUTTON_INDEX;\n")
            f.write("\n")
            f.write("//! Human readable names for connected rotary encoders.\n")
            f.write("typedef enum EVENT_ROTARY_INDEX\n")
            f.write("{\n")
            f.write("\tEVENT_ROTARY_ONE,\n")
            f.write("\tEVENT_NUM_ROTARIES\n")
            f.write("} EVENT_ROTARY_INDEX;\n")
            f.write("\n")
            f.write("//! Enumeration of possible input device event types.\n")
            f.write("typedef enum EVENT_TYPE\n")
            f.write("{\n")
            f.write("\tEVENT_TYPE_KEY_PRESS,\n")
            f.write("\tEVENT_TYPE_KEY_RELEASE,\n")
            f.write("\tEVENT_TYPE_KEY_HOLD,\n")
            f.write("\tEVENT_TYPE_ROTATION,\n")
            f.write("\tEVENT_TYPE_VAR_CHANGE,\n")
            f.write("\tEVENT_TYPE_STATE_ENTRY,\n")
            f.write("\tEVENT_TYPE_STATE_EXIT,\n")
            f.write("\tEVENT_TYPE_IDLE,\n")
            f.write("\tEVENT_TYPE_TRIGGERED,\n")
            f.write("\tEVENT_TYPE_DELAY\n")
            f.write("} EVENT_TYPE;\n")
            f.write("\n")
            f.write("//! Holds static data for a keypress event.\n")
            f.write("typedef struct EVENT_KEY_PRESS\n")
            f.write("{\n")
            f.write("\tuint8_t num_buttons;                            //!< Number of buttons used by event.\n")
            f.write("\tEVENT_BUTTON_INDEX  buttons[EVENT_MAX_BUTTONS]; //!< Array of indexes for buttons used by event.\n")
            f.write("} EVENT_KEY_PRESS;\n")
            f.write("\n")
            f.write("//! Holds static data for a key release event.\n")
            f.write("typedef EVENT_KEY_PRESS EVENT_KEY_RELEASE;\n")
            f.write("\n")
            f.write("//! Holds static data for a key hold event.\n")
            f.write("typedef struct EVENT_KEY_HOLD\n")
            f.write("{\n")
            f.write("\tbool    repeat;                                 //!< True if event should repeat until buttons are released.\n")
            f.write("\tuint8_t num_buttons;                            //!< Number of buttons used by event.\n")
            f.write("\tEVENT_BUTTON_INDEX  buttons[EVENT_MAX_BUTTONS]; //!< Array of indexes for buttons used by event.\n")
            f.write("\tuint32_t delay;                                 //!< Time buttons must be held down. Also delay between repeats.\n")
            f.write("} EVENT_KEY_HOLD;\n")
            f.write("\n")
            f.write("//! Holds static data for a rotation event.\n")
            f.write("typedef struct EVENT_ROTATION\n")
            f.write("{\n")
            f.write("\tEVENT_ROTARY_INDEX  encoder;                    //!< Rotary encoder used by event.\n")
            f.write("\tint8_t  steps;                                  //!< Number of encoder steps to trigger event. Negative for CCW rotation.\n")
            f.write("\tuint8_t num_buttons;                            //!< Number of buttons used by event.\n")
            f.write("\tEVENT_BUTTON_INDEX  buttons[EVENT_MAX_BUTTONS]; //!< Array of indexes for buttons used by event.\n")
            f.write("} EVENT_ROTATION;\n")
            f.write("\n")
            f.write("//! Holds static data for a varstate change event.\n")
            f.write("typedef struct EVENT_VAR_CHANGE\n")
            f.write("{\n")
            f.write("\tuint8_t num_vars;                               //!< Number of varstate variables.\n")
            f.write("\tuint8_t vars[EVENT_MAX_VARS];                   //!< Array of indexes for varstate variables.\n")
            f.write("} EVENT_VAR_CHANGE;\n")
            f.write("\n")
            f.write("//! Holds static data for an idle event.\n")
            f.write("typedef struct EVENT_IDLE\n")
            f.write("{\n")
            f.write("\tunion\n\t{\n")
            f.write("\t\tuint32_t delay;                             //!< Time idle before event triggers.\n")
            f.write("\t\tstruct\n\t\t{\n")
            f.write("\t\t\tuint32_t index : 31;                    //!< Variable index\n")
            f.write("\t\t\tuint32_t is_var : 1;                    //!< Last bit indicates a variable reference\n")
            f.write("\t\t\t/* data */\n")
            f.write("\t\t} var;\n")
            f.write("\t};\n")
            f.write("} EVENT_IDLE;\n")
            f.write("\n")
            f.write("typedef struct EVENT_DELAY\n")
            f.write("{\n")
            f.write("\tunion\n\t{\n")
            f.write("\t\tuint32_t delay;                             //!< Time idle before event triggers.\n")
            f.write("\t\tstruct\n\t\t{\n")
            f.write("\t\t\tuint32_t index : 31;                    //!< Variable index\n")
            f.write("\t\t\tuint32_t is_var : 1;                    //!< Last bit indicates a variable reference\n")
            f.write("\t\t\t/* data */\n")
            f.write("\t\t} var;\n")
            f.write("\t};\n")
            f.write("\tuint8_t repeat;\n")
            f.write("} EVENT_DELAY;\n")
            f.write("\n")
            f.write("//! Struct defining an input device triggered event.\n")
            f.write("typedef struct EVENT_DEF\n")
            f.write("{\n")
            f.write("#ifdef HMI_EVENT_DEBUG_NAMES\n")
            f.write("\tconst char *name;               //!< Name of state, for debugging\n")
            f.write("#endif\n")
            f.write("\tEVENT_TYPE type;                //!< Type of the defined event.\n")
            f.write("\tuint8_t state_index;            //!< Index for accessing event runtime state.\n")
            f.write("\tuint8_t menu_event;             //!< Menu event to trigger.\n")
            f.write("\tunion                           //!< Type-specific event data.\n")
            f.write("\t{\n")
            f.write("\t\tEVENT_KEY_PRESS key_press;\n")
            f.write("\t\tEVENT_KEY_HOLD  key_hold;\n")
            f.write("\t\tEVENT_ROTATION  rotation;\n")
            f.write("\t\tEVENT_VAR_CHANGE var_change;\n")
            f.write("\t\tEVENT_IDLE      idle_event;\n")
            f.write("\t\tEVENT_DELAY     delay_event;\n")
            f.write("\t\tint32_t         key_none;\n")
            f.write("\t};\n")
            f.write("} EVENT_DEF;\n")
            f.write("\n")
            f.write("//! Human readable names for menu events.\n")
            f.write("typedef enum MENU_EVENT\n")
            f.write("{\n")
            f.write("\tMENU_EVENT_NONE = 0,\t//!< No event. Must always be 0.\n")

            for evt in [evt for evt in self.events.values() if evt.internal == False]:
                f.write("\t" + evt.event_name + " = " + str(evt.index) + ",\n")

            f.write("} MENU_EVENT;\n")
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            f.write("extern const EVENT_DEF event_data_events[];\n")
            f.write("\n")
            f.write("#endif /* " + guarddef + " */\n")

        with open(source_file, "w") as f:
            f.write("/* clang-format off */" + "\n")
            f.write("/*! \\file " + os.path.basename(source_file) + "\n")
            f.write(" *  \\brief Full Climate event module event definitions. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write('#include "' + os.path.basename(header_file) + '"\n')
            f.write("\n")
            f.write("#include <stdint.h>\n")
            f.write("#include <stdbool.h>\n")
            f.write("\n")
            f.write('#include "event_data.h"\n')
            f.write("\n")
            f.write("/** Defines *******************************************************************/\n")
            f.write("\n")
            f.write("//! Human readable names for events.\n")
            f.write("typedef enum EVENT_INDEX\n")
            f.write("{\n")

            for evt in [evt for evt in self.events.values() if evt.internal == False]:
                f.write("\t" + evt.def_name + ",\n")

            f.write("\tNUM_EVENTS,\n")
            f.write("} EVENT_INDEX;\n")
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            f.write("\n")
            f.write("//! Input event definitions. Note that array order also decides event priority.\n")
            f.write("const EVENT_DEF event_data_events[NUM_EVENTS] =\n")
            f.write("{")

            for evt in [evt for evt in self.events.values() if evt.internal == False]:
                f.write("\n")
                f.write(evt.c_repr())

            f.write("};\n")
