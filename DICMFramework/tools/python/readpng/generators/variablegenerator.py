"""Provides VariableGenerator class used to parse variable definition file."""

import os
import re

from lib.defreader import DefReader, DefError
from deftypes.misc import Variable, VarDDMP
from constants import VARS_MAX

class DynamicTableEntry:
    def __init__(self, dyn_str, variables):
        self._var_index = None
        self._dyn_str = dyn_str
        s = re.split(r'#', dyn_str)
        if len(s) == 1:
            self._data = 0
        else:
            self._data = int(s[1])
        # Find out which var index s[0] corresponds to
        if s[0] in variables:
            # print("Adding DynamicTableEntry " + s[0] + " for var index " + str(variables[s[0]].index)) 
            self._var_index = variables[s[0]].index
        else:
            raise ValueError("Did not find " + s[0] + " in variables")
        
    def c_repr(self):
        """Returns a string representation of the variable understood by the HMI engine."""
        string = "\t\t{\n"
        string += "\t\t\t.var_index = " + str(self._var_index) + ",\n"
        string += "\t\t\t.data = " + str(self._data) + ",\n"
        string += "\t\t},\n"

        return string
        
class VariableGenerator:
    """Generates variable data for the HMI engine from definition file.

    A VariableGenerator is used to parse the HMI variable definition file.
    Definitions are read in from the directory specified during __init__ and then
    provided to other generators trough the variables attribute.

    The write_variable_data method is used to write out contained data to .c/.h files
    in a format understood by the HMI engine. This includes both data for the varstate
    module and the ddmp_uart module.

    Attributes
    ----------
    variables : Dict of (str: Variable)
        A dict of all generated variables indexed by variable name.

    Methods
    -------
    write_variable_data
        Write VariableGenerator contents to file for use by HMI engine.

    """

    def __init__(self, var_file):
        """Initializer for VariableGenerator class.
        
        New VariableGenerator will contain one Variable object for each definition
        found in var_file.
        
        Parameters
        ----------
        var_file : str
            Path to file containing variable definitions.

        Raises
        ------
        DefError
            Raised if MenuGenerator fails to parse any definition file.

        """

        self.variables = {}
        self._subscribes = []
        self._publishes = []
        self._dynamic_table = []
        self._dynamic_table_index = 0
        self._next_index = 0

        if not os.path.isfile(var_file):
            raise ValueError("Variable definition file not found.")
        if not os.path.splitext(var_file)[1].lower() == ".txt":
            raise ValueError("Variable definition file is not .txt")

        self._add_vars(var_file)

    def _add_vars(self, var_file):
        """Create a variable object for each definition in var_file and add to variable dict.

        Parameters
        ----------
        var_file : str
            Path to file containing variable definitions.

        """

        # Parse variable definition file by line.
        with DefReader(var_file) as var_def:
            for line in var_def:

                # Check that type is valid and has enough fields.
                if line[0] == "local":
                    if len(line) < 4:
                        raise DefError("Insufficient parameters for local variable definition.", var_def.name, var_def.line_no)
                elif line[0] == "subscribe" or line[0] == "publish":
                    if len(line) < 6:
                        raise DefError("Insufficient parameters for DDMP2 variable definition.", var_def.name, var_def.line_no)
                else:
                    raise DefError("Unknown variable type.", var_def.name, var_def.line_no)

                # Calculate default in number of steps.
                step = float(line[2])
                default = float(line[3])/step
                if not default.is_integer():
                    raise DefError("Variable default value must be a whole number of steps. is {} steps".format(default), var_def.name, var_def.line_no)
                default = int(default)

                # Generate new variable
                if line[0] == "local":
                    new_var = Variable(self._next_index, line[1], step, default)
                elif line[0] == "subscribe" or line[0] == "publish":
                    if line[5] == "int32":
                        data_type = "DDM2_TYPE_INT32_T"
                    elif line[5] == "uint32":
                        data_type = "DDM2_TYPE_UINT32_T"
                    elif line[5] == "struct":
                        data_type = "DDM2_TYPE_STRUCT"
                    elif line[5] == "other":
                        data_type = "DDM2_TYPE_OTHER"
                    elif line[5] == "string":
                        data_type = "DDM2_TYPE_STRING"
                    else:
                        raise DefError("Unknown DDM data type {}.".format(line[5]), var_def.name, var_def.line_no)
                    new_var = VarDDMP(self._next_index, line[1], step, default, line[4], data_type)

                # Add new variable to appropriate lists.
                if new_var.name in self.variables:
                    raise DefError("Variable with name {} already exists".format(new_var.name), var_def.name, var_def.line_no)
                self.variables[new_var.name] = new_var
                if line[0] == "subscribe":
                    self._subscribes.append(new_var)
                elif line[0] == "publish":
                    self._publishes.append(new_var)

                # Buildup/append dynamic mapping table
                if new_var.dynamic_param is not None:
                    print("Variable is dynamic: " + new_var.dynamic_param)
                    new_dyn_entry = DynamicTableEntry(new_var.dynamic_param, self.variables)
                    self._dynamic_table.append(new_dyn_entry)
                    new_var.update_param_id(self._dynamic_table_index)
                    self._dynamic_table_index += 1
                print("Added variable: " + new_var.name)

                # Increase variable index
                self._next_index += 1
                if self._next_index > VARS_MAX:
                    raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
                    
        # TODO: Define ADC configuration in definition files. Then remove this.
        # In the mean time we assume ADC readings should be stored in the
        # variable named room_temp. Therefore we need to define such a variable
        # if it didn't exist in the definition file.
        if "room_temp" not in self.variables:
            new_var = Variable(self._next_index, "room_temp", 0.01, 20.0)
            self.variables[new_var.name] = new_var
            self._next_index += 1
        # Add hmi0-class
        if "hmi0avl" not in self.variables:
            new_var = VarDDMP(self._next_index, "hmi0avl", 1.0, 1, "hmi0avl", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0ver" not in self.variables:
            # No default for string type
            new_var = VarDDMP(self._next_index, "hmi0ver", 1.0, 0, "hmi0ver", "DDM2_TYPE_STRING")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0tempunit" not in self.variables:
            # Default to 0. Will be initialized from nvs data
            new_var = VarDDMP(self._next_index, "hmi0tempunit", 1.0, 0, "hmi0tempunit", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0backlight" not in self.variables:
            # Default to 100. Will be initialized from nvs data
            new_var = VarDDMP(self._next_index, "hmi0backlight", 1.0, 100, "hmi0backlight", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0sound" not in self.variables:
            # Default to enabled. Will be initialized from nvs data
            new_var = VarDDMP(self._next_index, "hmi0sound", 1.0, 1, "hmi0sound", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0timeformat" not in self.variables:
            # Default to 24h. Will be initialized from nvs data
            new_var = VarDDMP(self._next_index, "hmi0timeformat", 1.0, 1, "hmi0timeformat", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0event" not in self.variables:
            # Event
            new_var = VarDDMP(self._next_index, "hmi0event", 1.0, 1, "hmi0event", "DDM2_TYPE_OTHER")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0vardata" not in self.variables:
            # Request var data
            new_var = VarDDMP(self._next_index, "hmi0vardata", 1.0, 0, "hmi0vardata", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0mute" not in self.variables:
            # Global Mute/unmute sound, default FALSE
            new_var = VarDDMP(self._next_index, "hmi0mute", 1.0, 0, "hmi0mute", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0childlock" not in self.variables:
            # HMI childlock function, default FALSE
            new_var = VarDDMP(self._next_index, "hmi0childlock", 1.0, 0, "hmi0childlock", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)
        if "hmi0screentimeout" not in self.variables:
            # Screen timeout in seconds, default 5 min
            new_var = VarDDMP(self._next_index, "hmi0screentimeout", 0.001, 300, "hmi0screentimeout", "DDM2_TYPE_INT32_T")
            self.variables[new_var.name] = new_var
            self._publishes.append(new_var)
            print("Added variable: " + new_var.name)
            # Increase variable index
            self._next_index += 1
            if self._next_index > VARS_MAX:
                raise DefError("Maximum of {} variables allowed.".format(VARS_MAX), var_def.name, None)

    def write_variable_data(self, header_file, source_file):
        """Exports VariableGenerator contents in a format understood by the HMI engine.

        VariableGenerator contents will be written to source_file while header_file
        will extern names needed by other HMI data files.

        Parameters
        ----------
        header_file, source_file : str
            Full paths to source and header file to be written.

        """

        with open(header_file, "w") as f:
            guarddef = re.sub('[^A-Za-z0-9]', '_', os.path.basename(header_file).upper()) + '_'

            f.write("/*! \\file " + os.path.basename(header_file) + "\n")
            f.write(" *  \\brief HMI varstate data header. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("#ifndef " + guarddef + "\n")
            f.write("#define " + guarddef + "\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write('#include <stdint.h>\n')
            f.write('#include "ddm2_parameter_list.h"\n')
            f.write("\n")
            f.write("/** Defines *******************************************************************/\n")
            f.write("\n")
            f.write("/** Typedefs ******************************************************************/\n")
            f.write("\n")
            f.write("//! Struct holding a HMI variable definition.\n")
            f.write("typedef struct VARSTATE_DEF\n")
            f.write("{\n")
            f.write("#ifdef HMI_VARSTATE_DEBUG_NAMES\n")
            f.write("\tconst char *name;                                   //!< Name of variable as defined in hmi source data\n")
            f.write("#endif\n")
            f.write("\tint32_t default_value;  //!< Variable default value.\n")
            f.write("\tfloat   step;           //!< Variable scale factor.\n")
            f.write("} VARSTATE_DEF;\n")
            f.write("\n")
            f.write("//! Struct holding adc configuration data.\n")
            f.write("typedef struct VARSTATE_ADC_CONF\n")
            f.write("{\n")
            f.write("\tuint32_t period;        //!< Time in sys ticks between measurements.\n")
            f.write("\tuint8_t var_index;      //!< HMI variable index to store measured data in.\n")
            f.write("} VARSTATE_ADC_CONF;\n")
            f.write("\n")
            f.write("//! Struct holding a dynamic mapping entry definition.\n")
            f.write("typedef struct VARSTATE_DYNENTRY_DEF\n")
            f.write("{\n")
            f.write("\tuint8_t var_index;   //!< Variable index.\n")
            f.write("\tuint8_t data;        //!< Additional data.\n")
            f.write("} VARSTATE_DYNENTRY_DEF;\n")
            f.write("\n")
            f.write("//! Struct holding a dynamic mapping table definition.\n")
            f.write("typedef struct VARSTATE_DYNMAPTABLE_DEF\n")
            f.write("{\n")
            f.write("\tuint8_t                  len;             //!< Length of table.\n")
            f.write("\tVARSTATE_DYNENTRY_DEF    table[];         //!< Dynamic mapping table.\n")
            f.write("} VARSTATE_DYNMAPTABLE_DEF;\n")
            f.write("\n")
            f.write("//! Struct holding varstate module configuration data.\n")
            f.write("typedef struct VARSTATE_CONF\n")
            f.write("{\n")
            f.write("\tconst VARSTATE_DEF  *var_def_array; //!< Pointer to HMI-variable array.\n")
            f.write("\tuint8_t             num_var_defs;   //!< Number of HMI-variables in array.\n")
            f.write("\tVARSTATE_ADC_CONF   adc1;           //!< Configuration for adc1.\n")
            f.write("\tconst VARSTATE_DYNMAPTABLE_DEF *dyn_map_table; //!< Dynamic mapping table.\n")
            
            f.write("} VARSTATE_CONF;\n")
            f.write("\n")
            f.write("//! Struct holding definition data for a DDMP2 publish.\n")
            f.write("typedef struct VARSTATE_DDMP_PARAMETER\n")
            f.write("{\n")
            f.write("#ifdef HMI_VARSTATE_DEBUG_NAMES\n")
            f.write("\tconst char *name;                                   //!< Name of variable as defined in hmi source data\n")
            f.write("#endif\n")
            f.write("\tconst struct VARSTATE_DDMP_PARAMETER *next_entry;   //!< Next entry in linked list. NULL for last entry.\n")
            f.write("\tuint32_t parameter_id;                              //!< ID of DDMP2 parameter.\n")
            f.write("\tDDM2_TYPE_ENUM type;                                //!< Type of DDMP2 parameter.\n")
            f.write("\tuint8_t var_index;                                  //!< HMI variable index used to store parameter value.\n")
            f.write("} VARSTATE_DDMP_PARAMETER;\n")
            f.write("\n")
            f.write("typedef enum _VARSTATE_INDEX\n{\n")
            for var in self.variables.values():
                f.write(var.c_index_repr())
            f.write("} VARSTATE_INDEX;\n\n")
            f.write("/** Variables *****************************************************************/\n")
            #f.write("extern const VARSTATE_CONF varstate_conf;\n")
            #f.write("extern const VARSTATE_DDMP_PARAMETER *sub_list;\n")
            #f.write("extern const VARSTATE_DDMP_PARAMETER *pub_list;\n")
            f.write("\n")
            f.write("#endif /* " + guarddef + " */\n")

        with open(source_file, "w") as f:
            f.write("/* clang-format off */" + "\n")
            f.write("/*! \\file " + os.path.basename(source_file) + "\n")
            f.write(" *  \\brief HMI varstate data definitions. This file is machine generated - do not edit!\n")
            f.write(" */\n")
            f.write("\n")
            f.write("/** Includes ******************************************************************/\n")
            f.write("\n")
            f.write("#include <stdint.h>\n")
            f.write("#include <stdbool.h>\n")
            f.write("\n")
            f.write('#include "' + os.path.basename(header_file) + '"\n')
            f.write("\n")
            f.write("/** Variables *****************************************************************/\n")
            f.write("static const VARSTATE_DEF var_defs[] =\n")
            f.write("{")    # Newline removed on purpose.

            for var in self.variables.values():
                f.write("\n")
                f.write(var.c_repr())

            f.write("};\n")
            f.write("\n")
            f.write("/** DDMP parameters ***********************************************************/\n")

            # Subscribes and publishes are linked lists so c_ddmp_repr() takes previous
            # object in list as a parameter so it can write out the appropriate cname.
            
            if self._subscribes:
                f.write(self._subscribes[0].c_ddmp_repr(None))
                f.write("\n")
                for i in range(1, len(self._subscribes)):
                    f.write(self._subscribes[i].c_ddmp_repr(self._subscribes[i-1]))
                    f.write("\n")

            if self._publishes:
                f.write(self._publishes[0].c_ddmp_repr(None))
                f.write("\n")
                for i in range(1, len(self._publishes)):
                    f.write(self._publishes[i].c_ddmp_repr(self._publishes[i-1]))
                    f.write("\n")

            f.write("static const VARSTATE_DYNMAPTABLE_DEF var_dynmap_table =\n")
            f.write("{")
            if self._dynamic_table:

                f.write("\n")
                f.write("\t.len = " + str(len(self._dynamic_table)) + ",\n")
                f.write("\t.table = \n")
                f.write("\t{\n")
                for i in range(len(self._dynamic_table)):
                    f.write(self._dynamic_table[i].c_repr())
                f.write("\t}\n")
            else:
                f.write("\n")
            f.write("};\n\n")
            
            f.write("/** Initialization data *******************************************************/\n")

            # TODO: Define ADC configuration in definition files.
            # For now this assumes ADC readings should be stored in the
            # room_temp variable. This variable will have been created in
            # _add_vars if it doesn't exist.
            f.write("const VARSTATE_CONF varstate_conf =\n")
            f.write("{\n")
            f.write("\t.var_def_array = var_defs,\n")
            f.write("\t.num_var_defs = " + str(len(self.variables)) + ",\n")
            f.write("\t.adc1 =\n")
            f.write("\t{\n")
            f.write("\t\t.period = 1000,\n")
            f.write("\t\t.var_index = " + str(self.variables["room_temp"].index) + ",\n")
            f.write("\t},\n")
            f.write("\t.dyn_map_table = &var_dynmap_table,\n")
            f.write("};\n")
            f.write("\n")
            
            if self._subscribes:
                f.write("const VARSTATE_DDMP_PARAMETER * const sub_list = &" +  self._subscribes[-1].c_name + ";\n")
            else:
                f.write("const VARSTATE_DDMP_PARAMETER * const sub_list = NULL;\n")
            
            if self._publishes:
                f.write("const VARSTATE_DDMP_PARAMETER * const pub_list = &" +  self._publishes[-1].c_name + ";\n")
            else:
                f.write("const VARSTATE_DDMP_PARAMETER * const pub_list = NULL;\n")

            f.write("\n")
