# Neon Kit

This component is part of Neon Kit.

## Architecture of Neon Kit

Neon Kit consists of the following components:
- Event Driven System
- Real-time kernel
- Library

Each part is contained in a separate git repository. This allows application writer to assemble a 
Neon Kit that suits his needs.

# Neon Kit - Event Driven System

This component contains code which is responsible for event handling using state machines. The code 
is split into two sections:
- generic
- portable

This approach provides high portability to various systems.

## Generic code

The generic code is a code which does not depend on used CPU architecture,peripherals and compilers. 
A particular combination of CPU architecture, peripherals and compiler is called a port. This code 
is shared across all supported ports.

## Portable code

This code is something that is developed by application writer. The application writer is 
responsible to implement all needed functionality of a port. Port templates are available in 
`template` directory.

## Where is a build system?

There is no build system for Neon Kit suite components. The Neon Kit contains only sources which are 
needed to build the system. Since Neon Kit components are like a plugin to application project it 
does not use or favor any build system. It is up to application writer to integrate Neon Kit 
components into the project. Since there are so many IDEs and build systems it is only natural to 
leave the building process to application writer. The only thing that a Neon Kit component expect is
a directory structure specified in the description following this chapter.

## Building a Neon Kit component

All Neon Kit components need to be placed into a single directory. For convenience lets refer to
this directory as a variable named `NEON_KIT_ROOT`.

The following are needed to be configured in application project:
- include path should have the following entries:
  - directory where all Neon Kit projects are located in `$NEON_KIT_ROOT`.
  - directory where a specific Neon Kit component is located. In this case the path would be 
    `$NEON_KIT_ROOT/[component]/source`.
  - directory where portable definition header `[component]_port_definition.h` is located.
- no defined macros are needed except when a custom component configuration is needed which is 
  explained below.
- compile all sources under `source` directory, in this case `$NEON_KIT_ROOT/eds/source`.

### Example

Lets say that we have a firmware application called `application_1` with the following structure:
1. The firmware application is located in `firmware/application_1` folder.
2. In this application Neon Kit EDS component is used and is located in `firmware/application_1/lib/neon-kit/eds` folder.
3. The portable definition header `eds_port_definition.h` is located in `firmware/application_1/eds_port/`. 

With this setup we would need the following additional include paths:
- `firmware/application_1/lib/neon-kit`
- `firmware/application_1/lib/neon-kit/eds/source`
- `firmware/application_1/eds_port/`

## Building Event Driven System with custom configuration

When a custom configuration is not enabled, Event Driven System uses default configuration which 
should be a good fit for most applications. It is possible to create a specific Event Driven System
implementation which is optimal for the current application. This feature remains yet to be defined 
and implemented.

## Application Programming Interface

The application programming interface is governed by 
[coding style guide](documentation/nk_devel_coding_style.md) document. The document defines:

- __Coding style__: All public objects declared in Application Programming Interface are following
  the coding style defined in [coding style document](documentation/nk_devel_coding_style.md).
- __Naming rules__: Names of functions, types and variables are defined in the coding style guide
  document, too. Naming abbreviations are defined by [abbreviations](documentation/abbreviations.md) 
  document.
- __Common exception or error reporting__: All Neon Kit API functions are handling exceptions and
  errors in the same manner. Assertions are not used in great number which allows application writer 
  to decide what happens on certain errors and exceptions (does it want to handle the error or 
  exception or it wants to stop the application execution). Error and exception checking in Nano Kit
  is divided into the following categories:
  - __precondition exceptions__: Precondition exceptions are generated when function preconditions 
    are not satisfied. Some examples include, but are not limited to, when passing invalid arguments 
    to a function, a function is called from improper context or when function input arguments are 
    in invalid/illegal state(s). 
  - __postcondition exceptions__: Postcondition exceptions are generated when function can not 
    fulfill its designated task. Some examples are, but not limited to, failing to allocate memory 
    or a resource, failed to send data and etc.
  - __safety exceptions__: Safety exceptions are generated when functions asserts certain details 
    about the input objects or when the functions is called from inappropriate application state. 
    Some examples, include, but are not limited to, when an allocation function is called outside 
    the initialization state. If the application is not developed for a safety critical mission then 
    this exception handling might never be enabled. 
  - __general errors__: General errors are errors which can not be put into none of the above 
    categories. These errors can't be disabled, they are always enabled. Note that these errors 
    don't have the component name field in them as the exceptions in other categories.

  For more details on exception and error handling refer to 
  [error handling](documentation/error_handling.md) document.
- __Function classes__: Functions are divided into classes regarding the execution context. The
  function class is part of the function name so there should be no confusion when a function may
  be called. For details see [function classes](documentation/function_classes.md) document.

## Deterministic

Majority of algorithms used in Neon Kit are belonging to **Constant Time Complexity** category. 
Constant Time `O(1)` functions needs fixed amount of time to execute an algorithm. In other words 
the execution time of Constant Time Complexity functions does not depend on number and/or value of 
inputs. For more information see [time complexity](documentation/time_complexity.md) document.

## Configurable

The Neon Kit uses configuration files which can be used to tailor the implementation to application 
needs.

In addition, the Neon Kit components implements a number of hooks which can alter or augment the 
behavior of the component, by allowing application to intercept function calls between software 
components.

## Portable

During the design stage of the component a special attention was given to achieve high portability. 
Some data structures and algorithms are tailored to exploit new hardware features.

## Static object allocation
All objects used in Neon Kit can be statically allocated. There is no need to use any memory 
management functionality which makes it suitable to verify the application using static analysis 
tools.

## Up to 32 Agent priority levels
Each Agent has a defined priority. Lowest priority level is 0, while the highest available is level 
31.

## Application development guides

Several application development approaches are defined when using Neon Kit. More details on how
to structure the project are available in 
[application development](documentation/application_development.md) document. 


