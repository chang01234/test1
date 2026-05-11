# NK Development coding style

## Intro

__Intended audience__ of this document: 

* Developers who will work on Nano Kernel sources (required).
* Testers who test Nano Kernel sources (informative).
* Users of the Nano Kernel (informative).

This document describes coding style guidelines and rules used in Nano Kernel sources.


## About This Guide

Purpose of this style guide is to encourage use of common coding practices within the Nano Kernel.

Style guide is a set of rules which are aimed to help create readable and maintainable code. 
By writing code which looks the same way across the code base will help others read and understand 
the code. By using the same conventions for spaces, newlines and indentation chances are reduced 
that future changes will produce huge unreadable diffs. By following the common patterns for code
structure and by using language features consistently will help others understand the code behavior.

# Naming rules

## General naming rules
- __R0100__: All functions, variables and types are using lower case names, words are delimited by 
  underscore `_` character. 
  Examples: 
    
      void do_something(void);
      uint32_t some_variable;
      
- __R0101__: All macro names are in `UPPERCASE` style, words are delimited by underscore `_` 
  character. 
- __R0102__: All functions, variables and types are prefixed with: `nk_`.
- __R0103__: All typedefs are named in the same manner as struct or union types. No suffixes (like 
  the usual `_t` suffix) are allowed.
- __R0104__: All macro names are prefixed with: `NK_`.
- __R0105__: In addition to __R0102__, the global public variables are additionally prefixed with 
  `__g__`, resulting in `nk__g__` prefix.
- __R0106__: In addition to __R0102__, the global local (file scope) variables are additionally 
  prefixed with `__l__`, resulting in `nk__l__` prefix.
- __R0107__: In addition to __R0102__, the global private variables or functions are additionally 
  prefixed with `__p__`, resulting in `nk__p__` prefix.
- __R0108__: The name of API functions is constructed in the following way:

      nk_[component]__[function_name]
      
  Where:
  - `nk_` is standard prefix as defined by __R0102__.
  - `[component]` is the name of the component which function is part of.
  - `__` is component - function name separator.
  - `[function_name]` is the name of the function.
  
  Examples: 
  - In the following example, the function name is  `do_something` and it is part of API for `list` 
    component:
    
        void nk_list__do_something(void);
        
  - The function name is `cancel` and it is part of API for `timer` component:
  
        void nk_timer__cancel(nk_timer * timer);
        
  - The function name is `long_name_operation` and it is part of API for `long_name_component` 
    component:
    
        void nk_long_name_component__long_name_operation(void);

## Function classes naming rules

For details on what are function classes, refer to [function classes document](function_classes.md).

All API functions may be suffixed if they fall are into either I or S class categories:
- __R0200__: - I class - Regular Interrupts are locked. Function suffix is `_i`.
- __R0201__: - S class - Scheduler switchings are locked. Function suffix is `_s`.

## Portable objects naming rules

All Port Interface objects are named using the rules stated above with certain differences:
- __R0300__: In addition to __R0102__, all functions, variables and types are additionally prefixed 
  with: `port_`, resulting in `nk_port_` prefix.
- __R0301__: In addition to __R0104__, all macro names are additionally prefixed with: `PORT_`, 
  resulting in `NK_PORT_` prefix.
- __R0302__: In addition to __R0102__, all port specific functions, variables and types (which are 
  not part of public NK RTOS API) are additionally prefixed with `cpu_`, resulting in `nk_cpu_` 
  prefix.
- __R0303__: In addition to __R0104__: all port specific macros are additionally prefixed with 
  `CPU_`, resulting in `NK_CPU_` prefix.

## Structure and union member naming rules

Members which are private and not to be accessed and modified by application code must follow the 
following naming rules:
- __R0400__: Private members must use `p__` prefix in order to tell to programmer that the member is 
  private to the structure.
  
# Right margin

- __R0500__: The hard wrap is set to column 100.

# Indentation

- __R0600__: Macro body starts either on column 45 if the body fits in between the column and the 
  right ruler defined by __R0500__. If the macro body does not fit or the macro name is too long, 
  the macro body starts on next line with  `\` continuation character located one tab before the
  right margin defined by __R0500__ rule. The body on next line start at column 9 (two tabs).
  
      #define NK_SAFETY__HAS_STARTED()            (nk__p__safety_critical_has_started == true)
      
      #define NK_SAFETY__START()                                                                      \
              do {                                                                                    \
                  nk__p__safety_critical_has_started = true;                                          \
              } while (0)

              
# Fixed or predefined values

Never code a fixed value directly in code. Always use a macro for this job:

    #define SOME_LIMIT_CONDITION 23

    uint32_t configurable_size_array[SOME_LIMIT_CONDITION];


# Comments

The Rob Pike article "Notes on programming in C" section about comments is spot on:

    A delicate matter, requiring taste and judgement. I tend to err on the side of
    eliminating comments, for several reasons. First, if the code is clear, and
    uses good type names and variable names, it should explain itself.  Second, 
    comments aren't checked by the compiler, so there is no guarantee they're 
    right, especially after the code is modified.  A misleading comment can be very
    confusing. Third, the issue of typography: comments clutter code.

    But I do comment sometimes. Almost exclusively, I use them as an introduction
    to what follows. Examples: explaining the use of global variables and types 
    (the one thing I always comment in large programs); as an introduction to an
    unusual or critical procedure; or to mark off sections of a large computation.

# Error handling

Error handling in Nano Kernel is using _return value_ method. 
- __G0100__: The overall look of functions needs to comform to the following style:

      int function(void)
      {
          int error;
          error = do_something1();
      
          if (error) 
              goto CLEAN_1;
              
          error = do_something2();
      
          if (error)
              goto CLEAN_2;
            
          error = do_something3();
      
          if (error)
              goto CLEAN_3;
            
          return ERR_NONE;
      CLEAN_3:
          undo_something2();
      CLEAN_2:
          undo_something1();
      CLEAN_1:
          return error;
      }
    
- __G0101__: If a function does not need to undo actions, the following style is also acceptable:

      int function(void)
      {
          int error;
          error = do_something1();
      
          if (error) {
              return error;
          }
          error = do_something2();
      
          if (error) {
              return error;
          }
          error = do_something3();
      
          if (error) {
              return error;
          }
          return ERR_NONE;
      }
    
- __G0102__: If a function needs only to ensure that arguments are valid the following style should 
  be used:

      int function(arg_type arg1, arg_type arg2, arg_type arg3)
      {
          if (IS_NOT_VALID(arg1)) {
              return NK_ERROR__X__X1;
          } 
          if (IS_NOT_VALID(arg2)) {
              return NK_ERROR__X__X2;
          }
          if (IS_NOT_VALID(arg3)) {
              return NK_ERROR__X__X3;
          }

          /* ... */
                    
          return NK_ERROR__NONE;
      }
