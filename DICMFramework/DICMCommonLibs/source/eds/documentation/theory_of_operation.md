# Theory of operation

## Neon-Kit Event Driven System (EDS) project structure

The project contains the following folders:
- __build__: Contains scripts and configuration files for Doxygen and for unit-tests.
- __configuration__: Contains templates which are to be used by application to define EDS
  configuration options.
- __documentation__: Contains text documents, images and source files of images diagrams.
- __source__: Contains all the source code of EDS which is common to all supported ports.
- __tests__: Contains unit-tests.

# Sources

- `eds.h` - Main API header file. This is usually the only needed header file for application to
  use the EDS. It does not include additional header files besides standard C files. All types
  defined in this header are opaque types to objects. The application uses
  `eds__<object>__create_<variant>` functions to allocate objects.

  __Declares:__
  - Public API macros.
  - Public API integral types.
  - Public API opaque types.
  - Public API functions.
  - Object naming prefix: `eds__[object]__` and `EDS__[OBJECT]__`.

  __Restrictions:__
  - Must not include any other header file except standard C library header files.

  __Rationale:__
  - Since this file is main API header file, we don't want to include a bunch of header files to be
    included by it. This enhances understandability and encapsulation of the implementation. The
    goal is to have one header file and several implementation files for complete EDS
    implementation. Other header files in this directory are mainly used to extend the existing
    implementation of EDS.

- `eds_object.h` - Secondary API file. This file should not be included by application code, except,
  when the application wants to extend the objects or allocate the objects statically.

  __Declares:__
  - All object types used by main `eds.h` API header.
  - All object types used internally by EDS.
  - Object naming prefix: `eds_object__`.

  __Restrictions:__
  - Must not include any other header file except `eds.h`, `eds_config.h`, `eds_port.h` and
    standard C library header files.

  __Rationale:__
  - The cleanest approach would be that this header includes only standard C library header files.
    Unfortunately, this is impossible due to configurable nature of EDS. The application
    may configure the EDS as it sees fit and this requirement impacts how the objects are
    constructed, too.

- `eds_[component]_.h` - Private API file. This file should not be included by application code,
  except, when the application wants to reuse (or extend) some functinality of EDS implementation.

  __Declares:__
  - All internal component functions of EDS. All internal component functions have protected
    access level.
  - All global variables (all variables have protected access level to EDS).
  - Object naming prefix: `eds_[component]__` and `EDS_[COMPONENT]__`.

  __Restrictions:__
  - Must not include any other header file except `eds_object.h`, other `eds_[component].h` and
    standard C library header files.

  __Rationale:__
  - All internal component functions and variables have protected access level. This means that
    they are not part of public API, but might be accessed by including additional headers. This
    allows that an external component or application might use the existing functionality offered
    by EDS.

- `eds_[component].c` - Private implementation file.

  __Defines:__
  - All functions which are internal to EDS.
  - All global variables.

- `core` component (`eds_core.h` and `eds_core.c`) is the only component that implements more than
  one sub-components.