
# Software simulator

## Contents of this directory

This directory contains a number of simulator implementations simulating various DDM parameter
classes. A DDM parameter class might be simulated in several variants.

## Simulation variants

    +-----------------------+-----------+-------------------+-------------------------------------------------------+
    | Name                  | Class     | Variant           | Comments                                              |
    +-----------------------+-----------+-------------------+-------------------------------------------------------+
    | ac_none               | AC        | none              | zero initial values, no actions/generators            |
    | ac_full_climate       | AC        | full climate      | some initial values, actions/generators, no errors    |
    | cc_none               | CC        | none              | zero initial values, no actions/generators            |
    | cc_full_climate       | CC        | full climate      | some initial values, actions/generators, no errors    |
    | ccs_none              | CCS       | none              | zero initial values, no actions/generators            |
    | ccs_full_climate      | CCS       | full climate      | some initial values, actions/generators, no errors    |
    | htr_none              | HTR       | none              | zero initial values, no actions/generators            |
    | htr_full_climate      | HTR       | full climate      | some initial values, actions/generators, no errors    |
    | iv_none               | IV        | none              | zero initial values, no actions/generators            |
    | iv_full_climate       | IV        | full climate      | some initial values, actions/generators, no errors    |
    +-----------------------+-----------+-------------------+-------------------------------------------------------+

To see available simulation configurations refer to `software_simulator_configuration.h` header
located in `dicm/esp32/main/software_simulator` directory.

##  Simulator files rules

1. Files are named in the following format:

       simulator_[class]_[variant]

   For example, the simulator for AC class with `Full Climate` variant whould be named:

       simulator_ac_full_climate.c
       simulator_ac_full_climate.h

2. These files are located under `dicm/esp32/main/software_simulator` directory.
3. Each class-variant file may contain additional pre-processor directives to make a sub-variant. If
   the number of pre-processor directives becomes too high (code is full of pre-processor
   directives) you can always create a totally new variant with needed changes and less number of
   pre-processor directives.
4. Each of the simulation header file exposes a constant global variable of type
   `const struct software_simimulator__descriptor` or `SOFTWARE_SIMULATOR`. For example, the file
   `simulator_ac_none.c` would have `g__simulator_ac_none_description` (actual naming might deviate
   a bit, depending on the rules and conventions for global variable naming).
5. Each of the simulation header files defines a class macro. Class macros are named in the
   following format:

       SIMULATOR_[CLASS]

   For example, the simulator class macro for AC class of any variant would be:

       SIMULATOR_AC

6. You don’t need to protect the compilation of these files with pre-processor, since they will not
   be linked into main executable if the global variable is not referenced in the connector.
7. Appropriate simulator headers are included by `software_simulator_configuration.h` header located
   in `dicm/esp32/main/software_simulator` directory.

Connectors would then include the description tables in the following way:

    static int initialize_connector(void)
    {
        return soft_sim__init(&s__shape_sim, &SIMULATOR_AC);
    }

## Using software simulator

In your project configuration header file:

1. Add defines to enable desired simulated connectors

       #define CONNECTOR_SIMULATOR_SHARC
       #define CONNECTOR_SIMULATOR_SHAPE
       #define CONNECTOR_SIMULATOR_INVENTILATE
2. Select which simulation variant is to be simulated

       #define SOFTWARE_SIMULATOR_FULL_CLIMATE
