"""Classes representing menu actions.

Readpng represents every menu action understood by the HMI engine as a subclass
of the Action class. Each subclass provides a c_repr() method which when called
will return a string representation of that action understood by the HMI engine.

Actions form linked lists when multiple actions need to be bound to a single
event. For this each object created from an Action subclass has a next_action
attribute which is initially None, meaning there is no next action, but can
later be set to point at whichever action should come next.

Branch objects additionally have a target action which is handled the same way.

Note that Action and some of its subclasses exist only as parents for other
actions and have no equivalent inside the HMI engine. Trying to call c_repr()
on one of these classes will raise a TypeError.

"""

class Action:
    """Parent class for all menu actions.

    Objects should be created from the correct subclass. Never from Action. Creating
    an action object directly from Action will cause errors.

    Attributes
    ----------
    cname : str
        A c-compatible name for the Action.
    ctype : str
        A MENU_ACTION_TYPE understood by the HMI engine.
    next_action : Action, None
        Next action to be taken by HMI engine.

    Methods
    -------
    c_repr
        Returns a string representation of this action understood by the HMI engine.

    """

    def __init__(self, cname):
        """Common initialization for all actions. Called from subclass __Init__.

        Parameters
        ----------
        cname : str
            A c-compatible name.

        """
        self.next_action = None
        self.ctype = None
        self.cname = cname

    def c_repr(self):
        """Returns a string representation of the action understood by the HMI engine.

        c_repr() is general for all action types (and thus all subclasses of
        Action). Each subclass is expected to override _c_subrepr() so that it
        returns a string representation of the type-specific parts of the action.

        Each subclass is also expected to set the class instance ctype to the correct
        string. Thus a ctype of None is an error likely caused by creating an object
        from the wrong class.

        Returns
        -------
        str
            A representation of the action understood by the HMI engine.

        """
        if self.ctype is None:
            raise TypeError("ctype must be set by Action subclass and must not be None")

        string = "static const MENU_ACTION " + self.cname + " =\n"
        string += "{\n"
        string += "\t.type = " + self.ctype + ",\n"
        string += "#ifdef HMI_MENU_DEBUG_NAMES\n"
        string += "\t.name = \"" + self.cname + "\",\n"
        string += "#endif // HMI_MENU_DEBUG_NAMES\n"
        string += self._c_subrepr()

        if self.next_action is None:
            string += "\t.next_action = NULL,\n"
        else:
            string += "\t.next_action = &" + self.next_action.cname + ",\n"

        string += "};\n"
        return string

    def _c_subrepr(self):
        raise NotImplementedError("Type-specific c representation must be provided by subclass.")

class StateChange(Action):
    """Change menu state menu action.

    This action changes current menu state to new state.

    """

    def __init__(self, new_state):
        """Initializer for StateChange action.

        Parameters
        ----------
        new_state : State
            New menu state object.

        """
        super().__init__("ac_set_state_" + new_state.name + "_A0")
        self.ctype = "MENU_ACTION_STATE_SET"
        self.new_state = new_state

    def _c_subrepr(self):
        return "\t.new_state = &" + self.new_state.cname + ",\n"

class VarSet(Action):
    """Variable set menu action.

    This actions sets variable to value.

    """

    def __init__(self, variable, value):
        """Initializer for VarSet action.

        Parameters
        ----------
        variable : Variable
            Variable object to change.
        value : int
            New variable value.

        """
        value = int(value)
        if value >= 0:
            super().__init__("ac_set_" + variable.name + "_to_" + str(value) + "_A0")
        else:
            super().__init__("ac_set_" + variable.name + "_to_minus_" + str(-value) + "_A0")
        self.value = value
        self.variable = variable
        self.ctype = "MENU_ACTION_VAR_SET"

    def _c_subrepr(self):
        string = "\t.var_set =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t\t.value = " + str(self.value) + ",\n"
        string += "\t},\n"

        return string

class LeapYear(Action):
    """Leap Year check menu action.

    This actions sets variable if provided value is leap year.

    """

    def __init__(self, variable, current_year):
        """Initializer for LeapYear action.

        Parameters
        ----------
        variable : Variable
            Variable object to change.
        current_year : int
            Current year set.

        """
        super().__init__("ac_set_" + variable.name + "_A0")
        self.current_year = current_year
        self.variable = variable
        self.ctype = "MENU_ACTION_LEAP_YEAR"

    def _c_subrepr(self):
        string = "\t.leap_year =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t\t.current_year_index = " + str(self.current_year.index) + ",\n"
        string += "\t},\n"

        return string

class VarChange(Action):
    """Variable change menu action.

    This action increases variable by value while ensuring that new value is less
    than or equal to limit. Can also be used to decrease variable by setting value
    negative in which case value will be more than or equal to limit.

    """

    def __init__(self, variable, value, limit):
        """Initializer for VarChange action.

        Parameters
        ----------
        variable : Variable
            Variable object to change.
        value : int
            Value to increase/decrease variable by.
        limit : int
            Upper/lower bound for new variable value.

        """
        value = int(value)
        if value >= 0:
            super().__init__("ac_inc_" + variable.name + "_by_" + str(value) + "_A0")
        else:
            super().__init__("ac_dec_" + variable.name + "_by_" + str(-value) + "_A0")
        self.variable = variable
        self.value = value
        self.limit = int(limit)
        self.ctype = "MENU_ACTION_VAR_CHANGE"

    def _c_subrepr(self):
        string = "\t.var_change =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t\t.value = " + str(self.value) + ",\n"
        string += "\t\t.limit = " + str(self.limit) + ",\n"
        string += "\t},\n"

        return string

class VarCopy(Action):
    """Variable copy menu action.

    This action copies the value of source variable to  target variable.

    """

    def __init__(self, variable, source, offset=None):
        """Initializer for VarCopy action.

        Parameters
        ----------
        variable : Variable
            Target variable object.
        source : Variable
            Source variable object.
        offset : string array 
            offset string array object.

        """
        super().__init__("ac_copy_" + source.name + "_to_" + variable.name + "_A0")
        self.ctype = "MENU_ACTION_VAR_COPY"
        self.offset = offset
        if offset is not None:
            self.ctype = "MENU_ACTION_VAR_COPY_OFFSET"
        self.variable = variable
        self.source = source

    def _c_subrepr(self):
        if self.offset is not None:
            string = "\t.var_copy_offset =\n"
        else:
            string = "\t.var_copy =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t\t.source_index = " + str(self.source.index) + ",\n"
        string += "\t\t.scale_factor = " + str(self.source.step/self.variable.step) + ",\n"
        if self.offset is not None:
            string += "\t\t.offset = " + self.offset[0] + ",\n"
            string += "\t\t.len = " + self.offset[1] + ",\n"

        string += "\t},\n"
        
        return string

class DDMPSet(Action):
    """DDMP2 set menu action.

    This action sends out a DDMP2 set request for the DDMP2 parameter associated
    with variable. Requested new value is current value of source variable.

    """

    def __init__(self, variable, source):
        """Initializer for DDMPSet action.

        Parameters
        ----------
        variable : Variable
            Variable object for DDMP2 set request.
        source : Variable
            Variable object containing new value to set.

        """
        super().__init__("ac_ddmp_set_" + variable.name + "_to_" + source.name + "_A0")
        self.ctype = "MENU_ACTION_DDMP_SET"
        self.variable = variable
        self.source = source

        if variable.data_type == "DDM2_TYPE_INT32_T":
            self.ddmp_length = 4
        else:
            raise ValueError

    def _c_subrepr(self):
        string = "\t.ddmp_set =\n"
        string += "\t{\n"
        string += "\t\t.parameter_id = " + hex(self.variable.parameter_id) + ",\n"
        string += "\t\t.parameter_length = " + str(self.ddmp_length) + ",\n"
        string += "\t\t.var_index = " + str(self.source.index) + ",\n"
        string += "\t},\n"

        return string

class DelayedStart(Action):
    """Delayed start menu action.

    This action triggers a delayed event after a specified time.

    """

    def __init__(self, event, delay):
        """Initializer for DelayedStart action.

        Parameters
        ----------
        event : Event
            Event to trigger.
        delay : int
            Time before triggering event.

        """
        super().__init__("ac_delayed_start_" + event.name + "_at_" + str(delay) + "_A0")
        self.ctype = "MENU_ACTION_DELAYED_START"
        self.event = event
        self.delay = delay

    def _c_subrepr(self):
        string = "\t.delayed_start =\n"
        string += "\t{\n"
        string += "\t\t.event_index = " + str(self.event.index) + ",\n"
        string += "\t\t.delay = " + str(self.delay) + ",\n"
        string += "\t},\n"

        return string

class DelayedStop(Action):
    """Delayed stop menu action.

    This action cancels a delayed event.

    """

    def __init__(self, event):
        """Initializer for DelayedStop action.

        Parameters
        ----------
        event : Event
            Event to cancel.

        """
        super().__init__("ac_delayed_stop_" + event.name + "_A0")
        self.ctype = "MENU_ACTION_DELAYED_STOP"
        self.event = event

    def _c_subrepr(self):
        string = "\t.delayed_stop =\n"
        string += "\t{\n"
        string += "\t\t.event_index = " + str(self.event.index) + ",\n"
        string += "\t},\n"

        return string

class TimerStart(Action):
    """Starts/restarts a timer (delay) menu action.

    This action starts a timer to generate delay events.

    """

    def __init__(self, event):
        """Initializer for TimerStart action.

        Parameters
        ----------
        event : Event
            Event to start/restart.

        """
        super().__init__("ac_timer_start_" + event.name + "_A0")
        self.ctype = "MENU_ACTION_TIMER_START"
        self.event = event

    def _c_subrepr(self):
        string = "\t.timer_start =\n"
        string += "\t{\n"
        string += "\t\t.event_index = " + str(self.event.index) + ",\n"
        string += "\t},\n"

        return string

class TimerStop(Action):
    """Stops timer (delay) menu action.

    This action stops a timer from generating delay events.

    """

    def __init__(self, event):
        """Initializer for TimerStop action.

        Parameters
        ----------
        event : Event
            Event to trigger.

        """
        super().__init__("ac_timer_stop_" + event.name + "_A0")
        self.ctype = "MENU_ACTION_TIMER_STOP"
        self.event = event

    def _c_subrepr(self):
        string = "\t.timer_stop =\n"
        string += "\t{\n"
        string += "\t\t.event_index = " + str(self.event.index) + ",\n"
        string += "\t},\n"

        return string

class DisplayOn(Action):
    """Display on menu action.

    This action makes the display controller leave sleep mode and turns on the
    backlight.

    """

    def __init__(self):
        """Initializer for DisplayOn action.

        """
        super().__init__("ac_display_on_A0")
        self.ctype = "MENU_ACTION_DISPLAY_ON"

    def _c_subrepr(self):
        string = ""

        return string

class DisplayOff(Action):
    """Display off menu action.

    This action turns off the backlight and makes the display controller enter
    sleep mode.

    """

    def __init__(self):
        """Initializer for DisplayOff action.

        """
        super().__init__("ac_display_off_A0")
        self.ctype = "MENU_ACTION_DISPLAY_OFF"

    def _c_subrepr(self):
        string = ""

        return string

class DisplayBrightnessSet(Action):
    """Display brightness set menu action.

    This action sets the screen brightness. The value is set from a specified
    variable.

    """

    def __init__(self, variable):
        """Initializer for DisplayBrightnessSet action.

        Parameters
        ----------
        variable : Variable
            Variable object containing new value to set.

        """
        super().__init__("ac_display_brightness_set_to_" + variable.name + "_A0")
        self.ctype = "MENU_ACTION_DISPLAY_BRIGHTNESS_SET"
        self.variable = variable

    def _c_subrepr(self):
        string = "\t.display_brightness_set =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t},\n"

        return string

class SaveHMISettings(Action):
    """SaveHMISettings menu action.

    This action save HMI settings (brightness and other product specific vars) to non-volatile memory

    """

    def __init__(self):
        """Initializer for SaveHMISettings action.

        """
        super().__init__("ac_save_hmi_settings_A0")
        self.ctype = "MENU_ACTION_SAVE_HMI_SETTINGS"

    def _c_subrepr(self):
        string = ""
        return string

class IdleReset(Action):
    """DDMP2 idle reset action.

    This action clears the idle timer for HMIs without buttons.

    """

    def __init__(self):
        """Initializer for DDMPSet action.

        """
        super().__init__("ac_idle_reset_A0")
        self.ctype = "MENU_ACTION_IDLE_RESET"

    def _c_subrepr(self):
        string = ""

        return string

class NoAction(Action):
    """DDMP2 No action action.

    Performs no action. Used to override global actions in current state

    """

    def __init__(self):
        """Initializer for DDMPSet action.

        """
        super().__init__("ac_no_action_A0")
        self.ctype = "MENU_ACTION_NO_ACTION"

    def _c_subrepr(self):
        string = ""

        return string

class Branch(Action):
    """Parent class for branch type menu actions.

    Objects should be created from the correct subclass. Never from Branch. Creating
    an action object directly from Branch will cause errors.

    Attributes
    ----------
    variable : Variable
        Variable to test.
    value : int
        Value to test variable against.
    target : Action
        Target action when branch condition is true.
    offset : string array 
        offset and len information

    """

    def __init__(self, variable, value, typestr, offset = None):
        """Common initialization for all branches. Called from subclass __Init__.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        typestr : str
            A string description of the branch type.
        offset : string array 
            offset and len information

        """
        value = int(value)
        if value >= 0:
            super().__init__("ac_branch_{}_{}_{}_A0".format(variable.name, typestr, value))
        else:
            super().__init__("ac_branch_{}_{}_minus_{}_A0".format(variable.name, typestr, -value))
        self.variable = variable
        self.value = value
        self.target = None
        self.offset = offset
        if offset is not None:
            if int(offset[0]) == 0 and int(offset[1]) == 0:
                raise ValueError
        

    def _c_subrepr(self):
        string = "\t.branch =\n"
        string += "\t{\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        string += "\t\t.value = " + str(self.value) + ",\n"

        if self.target is None:
            string += "\t\t.target = NULL,\n"
        else:
            string += "\t\t.target = &" + self.target.cname + ",\n"
        if self.offset is not None:
            string += "\t\t.offset = " + self.offset[0] +",\n"
            string += "\t\t.len = " + self.offset[1] + ",\n"
        string += "\t},\n"
        return string

class BranchEqual(Branch):
    """Branch if equal menu branch.

    Makes next action be target instead of next_action if variable value is value.

    """

    def __init__(self, variable, value, offset = None):
        """Initializer for BranchEqual action.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information
        """
        super().__init__(variable, value, "equal", offset)
        self.ctype = "MENU_ACTION_BRANCH_EQUAL"

class BranchNotEqual(Branch):
    """Branch if not equal menu branch.

    Makes next action be target instead of next_action if variable value is not
    value.

    """

    def __init__(self, variable, value, offset = None):
        """Initializer for BranchNotEqual action.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        super().__init__(variable, value, "not_equal", offset)
        self.ctype = "MENU_ACTION_BRANCH_NOT_EQUAL"

class BranchLess(Branch):
    """Branch if less than menu branch.

    Makes next action be target instead of next_action if variable value is less
    than value.

    """

    def __init__(self, variable, value, offset = None):
        """Initializer for BranchLess action.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        super().__init__(variable, value, "less", offset)
        self.ctype = "MENU_ACTION_BRANCH_LESS_THAN"

class BranchLessEqual(Branch):
    """Branch if less than or equal menu branch.

    Makes next action be target instead of next_action if variable value is less
    than or equal to value.

    """

    def __init__(self, variable, value, offset = None):
        """Initializer for BranchLessEqual action.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        super().__init__(variable, value, "less_or_equal", offset)
        self.ctype = "MENU_ACTION_BRANCH_LESS_THAN_OR_EQUAL"

class BranchMore(Branch):
    """Branch if greater than menu branch.

    Makes next action be target instead of next_action if variable value is more
    than value.

    """

    def __init__(self, variable, value, offset = None):
        """Initializer for BranchMore action.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        super().__init__(variable, value, "more", offset)
        self.ctype = "MENU_ACTION_BRANCH_GREATER_THAN"

class BranchMoreEqual(Branch):
    """Branch if greater than or equal menu branch.

    Makes next action be target instead of next_action if variable value is more
    than or equal to value.

    """

    def __init__(self, variable, value, offset = None):
        """Initializer for BranchMoreEqual action.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        super().__init__(variable, value, "more_equal", offset)
        self.ctype = "MENU_ACTION_BRANCH_GREATER_THAN_OR_EQUAL"
