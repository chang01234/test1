"""Provides DefReader and DefError used by generators when parsing HMI definition files.

DefReader
    An iterator that yields only the actual object definitions while skipping
    over any blank lines and comments.

DefError
    An error type raised when failing to parse a HMI definition file.

Examples
--------
Use a DefReader to parse def_file and pass definition objects named foo or bar
to their respective handlers. raise a DefError for any unfamiliar object types.

    with DefReader(def_file) as reader:
        for line in reader:
            if line[0] == "foo":
                parse_foo(line, reader)
            elif line[0] == "bar":
                parse_bar(line, reader)
            else:
                raise DefError("Unknown definition type", reader.name, reader.line_no)

"""

import os

class DefReader:
    """Iterator for looping over a HMI definition file.

    This class provides an iterator for looping over a HMI definition file
    while skipping any comments or blank lines. Each iteration yields the
    contents of the next non-blank/non-comment line in the file split into a
    tuple of parameters. Each parameter is lowercase and stripped of any leading
    or trailing whitespace.

    Implements necessary interfaces for use in python with statements.

    Attributes
    ----------
    name : str
        Name of the file being parsed.
    line_no : int
        Most recently parsed line.

    Methods
    -------
    parse_condition
        Parse a string containing a screen or branch condition.

    """

    def __init__(self, file):
        """Initializer for DefReader.

        Parameters
        ----------
        file : str
            Path to definition file to parse,

        """
        self._file = file
        self.name = os.path.basename(file)
        self._reader = None
        self.line_no = 0

    def __enter__(self):
        self.line_no = 0
        self._reader = open(self._file, "r")
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self._reader.close()

    def __iter__(self):
        return self

    def __next__(self):
        while True:
            line = self._reader.readline()
            self.line_no += 1

            # This is last line of file.
            if len(line) == 0:
                raise StopIteration

            # If this line is a comment or empty line read next line.
            line = line.strip()
            if len(line) == 0 or line[0] == "#":
                continue

            # Split line into fields and return these.
            return tuple(field.strip().lower() for field in line.split(","))

    def parse_condition(self, definition, variables):
        """Parse a screen or branch condition into a type, target, and value.

        Parameters
        ----------
        definition : str
            Condition definition field.
        variables : dict,
            Dict of available variables. Indexed by variable name.

        Returns
        -------
        cnd_type : {"==", "!=", "<", "<=", ">", "=>", "<_time", ">_time" }
            Condition type.
        var : Variable
            Tested variable. None if one of the time types.
        value : int
            Target value.

        """

        # Parse condition.
        if "==" in definition:
            cnd_type = "=="
        elif "!=" in definition:
            cnd_type = "!="
        elif "<=" in definition:
            cnd_type = "<="
        elif ">=" in definition:
            cnd_type = ">="
        elif "<" in definition:
            cnd_type = "<"
        elif ">" in definition:
            cnd_type = ">"
        else:
            raise DefError("Unknown condition type.", self.name, self.line_no)

        offset = None
        # Unpack condition av identify variable and value.
        data = [str.strip() for str in definition.split(cnd_type)]
        if data[0] == "time":
            var = None
            value = data[1]
            cnd_type += "_time"
        else:
            # format is <variable>[:offset:len]
            basevar = data[0]
            offset = basevar.split(":")
            if len(offset) > 1:
                data[0] = offset[0]
                if len(offset) != 3:
                    #print(offset)
                    raise DefError("offset:length not specified", self.name, self.line_no)
                offset = offset[1:]
            else:
                offset = None    
        
            if data[0] in variables:
                (var, value) = data
                var = variables[var]
                value = var.scale_step(value, self)
            else:
                raise DefError("Could not find variable.", self.name, self.line_no)

        return (cnd_type, var, value, offset)

class DefError(Exception):
    """Raised on failure to parse a HMI definition file."""

    def __init__(self, message, filename, line_no):
        """Initializer for DefError.

        Filename and line should be None only when error is not local to any
        specific line or file.

        Parameters
        ----------
        message : str
            Error message to show user.
        filename : str, None
            Name of file in which error occurred.
        line_no : int, None
            Line of file on which error occurred.

        """
        error_message = ""
        if filename is not None:
            if line_no is not None:
                error_message = "Failed to parse {} on line {}. ".format(filename, line_no)
            else:
                error_message = "Failed to parse {}. ".format(filename)
        error_message += message
        print(error_message)
        super().__init__(error_message)
