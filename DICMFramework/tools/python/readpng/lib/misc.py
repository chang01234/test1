"""Functions and classes not yet moved to their own modules."""

import os

from hmiversion import HMI_VERSION

def write_data_interface(outdir, source_file, num_events, draw_direction, inc_dir):
    """Writes definition data interface in c format to specified file.

    Only source file is written since data_interface.h does not depend on data.

    Parameters
    ----------
    source_file : str
        Destination path to write data interface source file to.
    num_events : int
        Number of events used by HMI definitions.
    draw_direction : str
        A valid draw direction enum name for draw module.

    TODO: Document.
    """
    c_source_file = os.path.join(outdir, source_file + ".c")
    h_source_file = os.path.join(outdir, inc_dir, source_file + ".h")
    extflash_file = os.path.join(outdir, inc_dir, 'extflash' + ".h")
    
    with open(c_source_file, "w") as f:
        f.write("/* clang-format off */" + "\n")
        f.write("/*! \\file " + os.path.basename(c_source_file) + "\n")
        f.write(" *  \\brief HMI data interface definition. This file is machine generated - do not edit!\n")
        f.write(" */\n")
        f.write("\n")
        f.write("/** Includes ******************************************************************/\n")
        f.write('#include <stdint.h>\n')
        f.write('#include "extflash.h"\n')
        f.write('#include "' + os.path.basename(h_source_file) + '"\n')
        f.write("\n")
        f.write("#if defined(BUILD_WITH_HMI_DATA)\n")
        f.write('#include "menu_data.c"\n')
        f.write('#include "varstate_data.c"\n')
        f.write("\n")
        f.write("/** Defines ******************************************************************/\n")
        f.write('#define HMI_VERSION "' + HMI_VERSION + '"\n')
        f.write("\n")

        f.write("/** Variables *****************************************************************/\n")
        f.write("const EXTFLASH_HEADER DEF_DATA def_data_header =\n")
        f.write("{\n")
        f.write("\t.varstate_conf = &varstate_conf,\n")
        f.write("\t.sub_list = sub_list,\n")
        f.write("\t.pub_list = pub_list,\n")
        f.write("\t.menu_boot_state = menu_data_boot_state,\n")
        f.write("\t.menu_global_state = menu_global_state,\n")
        f.write("\t.events = event_data_events,\n")
        f.write("\t.num_events = " + str(num_events) + ",\n")
        f.write("\t.draw_direction = " + draw_direction + ",\n")
        f.write("\t.version = HMI_VERSION,\n")
        f.write("};\n")
        f.write("#endif\n")

    with open(h_source_file, "w") as f:
        f.write("/*! \\file " + os.path.basename(h_source_file) + "\n")
        f.write(" *  \\brief HMI data interface definition. This file is machine generated - do not edit!\n")
        f.write(" *\n")
        f.write(" *  This file is used to make HMI definition data visible to engine code. It is\n")
        f.write(" *  the only definition data file that can safely be included outside of the\n")
        f.write(" *  definition data.\n")
        f.write(" */\n\n")
        f.write("#ifndef " + source_file.upper() + "_H_\n")
        f.write("#define " + source_file.upper() + "_H_\n\n")
        f.write("/** Includes ******************************************************************/\n")
        f.write('#include <stdint.h>\n')
        f.write('#include <stdbool.h>\n')
        f.write("\n")
        f.write('#include "bitmap_data.h"\n')
        f.write('#include "screen_data.h"\n')
        f.write('#include "event_data.h"\n')
        f.write('#include "menu_data.h"\n')
        f.write('#include "varstate_data.h"\n')
        f.write("\n")
        f.write("/** Defines *******************************************************************/\n")
        f.write("\n")
        f.write("/** Typedefs ******************************************************************/\n")
        f.write("\n")
        f.write("//! Struct for holding definition data pointers\n")
        f.write("typedef struct DEF_DATA\n")
        f.write("{\n")
        f.write("\tconst VARSTATE_CONF *varstate_conf;\t\t//!< Varstate module configuration data.\n")
        f.write("\tconst VARSTATE_DDMP_PARAMETER * const sub_list;\t//!< DDMP module subscribe list.\n")
        f.write("\tconst VARSTATE_DDMP_PARAMETER * const pub_list;\t//!< DDMP module publish list.\n")
        f.write("\tconst MENU_STATE * const menu_boot_state;\t\t//!< Menu module initial state.\n")
        f.write("\tconst MENU_STATE * const menu_global_state;\t//!< Global menu state.\n")
        f.write("\tconst EVENT_DEF *events;\t\t\t//!< Event module event array.\n")
        f.write("\tconst uint8_t num_events;\t\t\t\t//!< Event module event array length.\n")
        f.write("\tDRAW_DIRECTION_ENUM draw_direction;\t\t//!< Draw module draw direction.\n")
        f.write("\tconst char version[20];\t\t\t//!< Version of HMI data. Set in constants.py.\n")
        f.write("} DEF_DATA;\n")
        f.write("\n")
        f.write("/** Variables ******************************************************************/\n")
        f.write("#if defined(BUILD_WITH_HMI_DATA)\n")
        f.write('// When building hmi_data partition\n')
        f.write("extern const DEF_DATA def_data_header;\n")
        f.write("#endif\n")
        f.write("#endif /* " + source_file.upper() + "_H_ */\n")
"""        
    with open(extflash_file, "w") as f:
        f.write("/*! \\file " + os.path.basename(extflash_file) + "\n")
        f.write(" * \\brief Macros for placing data in external flash. This file is machine generated - do not edit!\n")
        f.write(" */\n\n")
        f.write("#ifndef " + os.path.splitext(os.path.basename(extflash_file))[0].upper() + "_H_\n")
        f.write("#define " + os.path.splitext(os.path.basename(extflash_file))[0].upper() + "_H_\n\n")
        f.write("/** Defines *******************************************************************/\n")
        f.write("\n")
        f.write('#define EXTFLASH_HEADER\t__attribute__((section(".extflashhdr")))\t//!< Object will be placed at the beginning of external flash memory. Only to be used with one object.\n')
        f.write('#define EXTFLASH\t\t__attribute__((section(".extflash")))\t\t//!< Object will be placed somewhere in the external flash memory. Use for all other flash objects.\n')
        f.write("\n")
        f.write("\n")
        f.write("#endif /* " + os.path.splitext(os.path.basename(extflash_file))[0].upper() + "_H_ */\n")
"""
        