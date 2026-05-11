"""Classes representing screen conditions.

Readpng represents every screen condition understood by the HMI engine as a
subclass of the Condition class. Each subclass provides a c_repr() method which
when called will return a string representation of that condition understood by
the HMI engine.

Conditions form linked lists when multiple conditions need to be tested for a
screen object. For this each object created from a Condition subclass has a 
next_condition attribute which is initially None, meaning this is the last
condition, but can later be set to point at whichever condition should come
next.

Note that Condition and some of its subclasses exist only as parents for other
conditions and have no equivalent inside the HMI engine. Trying to call
c_repr() on one of these classes will raise a TypeError.

"""

class Condition:
    """Parent class for all screen conditions.

    Objects should be created from the correct subclass. Never from Condition.
    Creating a condition object directly from Condition will cause errors.

    Attributes
    ----------
    cname : str
        A c-compatible name for the Condition.
    ctype : str
        A SCREEN_CONDITION_TYPE understood by the HMI engine.
    next_condition : Condition, None
        Next condition to be tested by HMI engine.

    Methods
    -------
    c_repr
        Returns a string representation of this condition understood by the HMI
        engine.

    """

    def __init__(self, cname):
        """Common initialization for all conditions. Called from subclass __Init__.

        Parameters
        ----------
        cname : str
            A c-compatible name.

        """
        self.ctype = None
        self.cname = cname
        self.next_condition = None

    def c_repr(self):
        """Returns a string of the condition understood by the HMI engine.

        c_repr() is general for all condition types (and thus all subclasses of
        Condition). Each subclass is expected to override _c_subrepr() so that it
        returns a string representation of the type-specific parts of the condition.

        Each subclass is also expected to set the class instance ctype to the correct
        string. Thus a ctype of None is an error likely caused by creating an object
        from the wrong class.

        Returns
        -------
        str
            A representation of the screen condition understood by the HMI engine.

        """
        if self.ctype is None:
            raise TypeError("ctype must be set by Condition subclass and must not be None")

        string = "static const SCREEN_CONDITION " + self.cname + " =\n"
        string += "{\n"
        string += "\t.type = " + self.ctype + ",\n"
        string += self._c_subrepr()

        if self.next_condition is None:
            string += "\t.next_condition = NULL, \n"
        else:
            string += "\t.next_condition = &" + self.next_condition.cname + ",\n"

        string += "};\n"
        return string

    def _c_subrepr(self):
        raise NotImplementedError("Type-specific c representation must be provided by subclass.")

class Compare(Condition):
    """Parent class for compare type screen conditions.

    Objects should be created from the correct subclass. Never from Compare. Creating
    a condition object directly from Compare will cause errors.

    Attributes
    ----------
    variable : Variable
        Variable to test.
    value : int
        Value to test variable against.
    offset : string array 
        offset and len information
    """

    def __init__(self, variable, value, typestr, ctype, offset = None):
        """Common initialization for all compares. Called from subclass __Init__.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        typestr : str
            A string description of the compare condition type.
        offset : string array 
            offset and len information

        """
        value = int(value)
        if value >= 0:
            super().__init__("{}_{}_{}_C0".format(variable.name, typestr, value))
        else:
            super().__init__("{}_{}_minus_{}_C0".format(variable.name, typestr, -value))
        self.variable = variable
        self.value = value
        self.ctype = ctype
        self.offset = offset
        if offset is not None:
            self.ctype = self.ctype + "_OFFSET"

    def _c_subrepr(self):
        if self.offset is None:
            string = "\t.compare =\n"
        else:
            string = "\t.compare_offset =\n"
        string += "\t{\n"
        string += "\t\t.value = " + str(self.value) + ",\n"
        string += "\t\t.var_index = " + str(self.variable.index) + ",\n"
        if self.offset is not None:
            string += "\t\t.offset = " + str(self.offset[0]) + ",\n"
            string += "\t\t.len = " + str(self.offset[1]) + ",\n"
        
        string += "\t},\n"

        return string

class CompEqual(Compare):
    """Screen condition. True if variable equals value."""

    def __init__(self, variable, value, offset = None):
        """Initializer for CompEqual condition.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        self.ctype = "SCREEN_COMPARE_EQUAL"
        super().__init__(variable, value, "equal", "SCREEN_COMPARE_EQUAL", offset)

class CompNotEqual(Compare):
    """Screen condition. True if variable does not equal value."""

    def __init__(self, variable, value, offset = None):
        """Initializer for CompNotEqual condition.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        self.ctype = "SCREEN_COMPARE_NOT_EQUAL"
        super().__init__(variable, value, "not_equal", "SCREEN_COMPARE_NOT_EQUAL", offset)

class CompLess(Compare):
    """Screen condition. True if variable less than value."""

    def __init__(self, variable, value, offset = None):
        """Initializer for CompLess condition.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        self.ctype = "SCREEN_COMPARE_LESS_THAN"
        super().__init__(variable, value, "less", "SCREEN_COMPARE_LESS_THAN", offset)


class CompLessEqual(Compare):
    """Screen condition. True if variable less than or equal to value."""

    def __init__(self, variable, value, offset = None):
        """Initializer for CompLessEqual condition.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        self.ctype = "SCREEN_COMPARE_LESS_THAN_OR_EQUAL"
        super().__init__(variable, value, "less_equal", "SCREEN_COMPARE_LESS_THAN_OR_EQUAL", offset)

class CompMore(Compare):
    """Screen condition. True if variable mote than value."""

    def __init__(self, variable, value, offset = None):
        """Initializer for CompMore condition.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        self.ctype = "SCREEN_COMPARE_GREATER_THAN"
        super().__init__(variable, value, "more", "SCREEN_COMPARE_GREATER_THAN", offset)

class CompMoreEqual(Compare):
    """Screen condition. True if variable more than or equal to value."""

    def __init__(self, variable, value, offset = None):
        """Initializer for CompMoreEqual condition.

        Parameters
        ----------
        variable : Variable
            Variable to test.
        value : int
            Value to test variable against.
        offset : string array 
            offset and len information

        """
        self.ctype = "SCREEN_COMPARE_GREATER_THAN_OR_EQUAL"
        super().__init__(variable, value, "more_equal", "SCREEN_COMPARE_GREATER_THAN_OR_EQUAL", offset)

class Delay(Condition):
    """Parent class for delay type screen conditions.

    Objects should be created from the correct subclass. Never from Delay. Creating
    a condition object directly from Delay will cause errors.

    Attributes
    ----------
    length : int
        Delay length in milliseconds.

    """

    def __init__(self, length, typestr):
        """Common initialization for all delays. Called from subclass __Init__.

        Parameters
        ----------
        length : int
            Delay length in milliseconds.
        typestr : str
            A string description of the delay type.

        """
        super().__init__("delay_{}_{}ms_C0".format(typestr, length))
        self.length = int(length)

    def _c_subrepr(self):
        string = "\t.delay =\n"
        string += "\t{\n"
        string += "\t\t.length = " + str(self.length) + ",\n"
        string += "\t},\n"

        return string

class DelayOn(Delay):
    """Screen condition. True if last screen change more than length milliseconds ago."""

    def __init__(self, length):
        """Initializer for DelayOn condition.

        Parameters
        ----------
        length : int
            Delay length in milliseconds.

        """
        super().__init__(length, "on")
        self.ctype = "SCREEN_DELAY_ON"

class DelayOff(Delay):
    """Screen condition. True if last screen change less than length milliseconds ago."""

    def __init__(self, length):
        """Initializer for DelayOff condition.

        Parameters
        ----------
        length : int
            Delay length in milliseconds.

        """
        super().__init__(length, "off")
        self.ctype = "SCREEN_DELAY_OFF"

class DelayPeriodic(Condition):
    """Screen condition. Toggles true/false based on time since last screen change.

    DelayPeriodic provides a condition that is periodically true and false based on
    the time since last screen change.

    Condition is true if (n-1)*period < time < n*on_time.
    Condition is false if n*on_time < time < n*period.
    For some integer n.

    """

    def __init__(self, on_time, period):
        """Initializer for DelayPeriodic condition.

        Parameters
        ----------
        on_time : int
            milliseconds of each period for which condition is true.
        period : int
            Length of each period in milliseconds.

        """
        super().__init__("period_" + str(on_time) + "_of_" + str(period) + "_C0")
        self.ctype = "SCREEN_DELAY_PERIODIC"
        self.period = int(period)
        self.on_time = int(on_time)

    def _c_subrepr(self):
        string = "\t.period = \n"
        string += "\t{\n"
        string += "\t\t.on_time = " + str(self.on_time) + ",\n"
        string += "\t\t.period = " + str(self.period) + ",\n"
        string += "\t},\n"

        return string
