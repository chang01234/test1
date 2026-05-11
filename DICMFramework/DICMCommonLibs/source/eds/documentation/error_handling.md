# Error handling

## Intro

- Error indicates a serious problems that a reasonable application should not try to catch.
- Exception indicates conditions that a reasonable application might want to catch.

Errors are to be handled by assertion macros like `assert` from standard C library `assert.h`. These
macros block the execution (some implementations also restart the whole device). 

Exceptions are to be handled by conditions in code. Since we are using C, the most simplest way is 
to use plain conditions and handle them by marking that an error has happened, but the execution 
continues.

__When not to use errors:__ It is be strictly forbidden to assert external signal value(s) or 
any signal aspect (this include values provided by MCU peripheral, too). This is to avoid and 
minimise sensitivity of the firmware to external influences which could lead to blocking the 
firmware execution or restarting the device.

__When to use errors:__ Only internal values are allowed to be asserted. Examples are, but not 
restricted to, memory or a resource allocation, NULL pointer checks. 

__When to use exceptions:__ When a value which is to be checked is coming from a peripheral (like 
ADC value is out of range) or from external component on a data bus (timeout on data bus).

All Neon Kernel API functions are reporting exceptions as return values. The following classes of 
errors are defined:
- precondition errors (P)
- postcondition errors (O)
- safety errors (S)
- general errors (G)

It is up to application to decide if an erroneous state is to be treated as an error or an exception.

Nano Kernel internally uses errors which are handled by `assert.h` standard C library header. The
application must define a function which implements the assert macro. This macro should never 
return.

__NOTE__:
- All erroneous states in Nano Kernel API functions are called errors. It is up to application to
  promote errors to exceptions as it sees fit.
- In the code and in the following text, both error and exception, are refered to as an error.
  
## Error reporting

Nano Kernel documentation defines the following terms:

- __originator error reporting__ - is a piece of code which is used by a function to report its 
  erronous state to someone. There are 3 possible methods for a function to report an erronous 
  state: 
  1. _return value_ - a function can return an error code to its caller. This is most common method 
     for reporting the error. The function itself is responsible for any cleanup if an error comes 
     up during the execution steps. This method is used by many open source projects including the 
     most prominent one, the Linux kernel.
  2. _global state_ - a function sets a global variable directly or by a call to another function 
     which stores the error information in some global state (variable). This method is used by some 
     embedded systems and by standard C library (for example, see errno). This method has drawback 
     when the system switched to multiple threads, then global state needs to be protected or 
     allocated on thread local storage (TLS).
  3. _bundle status flags_ - a function returns data values and data status flags bundled together. 
     All data types in a system are structures which contain data payload and data status flags. 
     When such a structure is returned by a callee function, the caller can see if data is valid or 
     not and what is the reason of failure. This kind of method is typically used in proprietary 
     SCADA systems, where all data is tagged with status flags and is passed between components as 
     such.
- __device exception reporting__ - defines how errors reported to external world some communication 
link or, very commonly, with LEDs. This piece of code handles the error values it got from various 
internal components and how to present that information to external world.

__NOTE__:
- Nano Kernel uses _return value_ method for originator error reporting.
 
## Error code bitfields

All error code use the bitfields to distinguish error classes.

Error code bitfields:

| Component  | P field | O field | S field | G field |
| ---------- | ------- | ------- | ------- | ------- |
| 4 bits     | 8 bits  | 6 bits  | 6 bits  | 8 bits  |
| 31 - 28    | 27 - 20 | 19 - 14 | 13 - 8  | 7 - 0   |

Where:
- __Component__ - specifies the origin of error. The origin of the error is specifined in terms of
  components.
- __P field__ - specifies if the error is a precondition error (P). If this bitfield is zero then
  the error is not a __P__ class of error.
- __O field__ - specifies if the error is a postcondifion error (O). If this bitfield is zero then
  the error is not a __O__ class of error.
- __S field__ - specifies if the error is a safety error (S). If this bitfield is zero then the
  error is not a __S__ class of error.
  
No error can not have all POSG fields set to zero. One exception to this is NK_ERROR__NONE where all
fields are set to zero.
    
## Error mapping

### General error - specific error mapping
    
| General error                       | Specific error                      |
| ----------------------------------- | ----------------------------------- |
| NK_ERROR__INVLD_ARGUMENT            | NK_ERROR__POOL_P_INVLD_POOL         |
| ^                                   | NK_ERROR__POOL_P_INVLD_BLOCK        |
| ^                                   | NK_ERROR__POOL_P_INVLD_N_BLOCKS     |
| ^                                   | NK_ERROR__POOL_P_INVLD_BLOCK_SIZE   |
| ^                                   | NK_ERROR__POOL_P_INVLD_STORAGE      |
| ^                                   | NK_ERROR__POOL_P_INVLD_NAME         |
| ^                                   | NK_ERROR__POOL_P_INVLD_FREE         |
| ^                                   | NK_ERROR__POOL_P_INVLD_TOTAL        |
| ^                                   | NK_ERROR__POOL_P_INVLD_OCCUPIED     |
| ^                                   | NK_ERROR__TIMER_P_INVLD_TIME_MS     |
| ^                                   | NK_ERROR__TIMER_P_INVLD_CALLBACK    |
| ^                                   | NK_ERROR__TIMER_P_INVLD_TIMER       |
| NK_ERROR__CALLED_FROM_ISR           | NK_ERROR__POOL_P_CALLED_FROM_ISR    |
| NK_ERROR__RESOURCE_BUSY             | NK_ERROR__POOL_P_IN_USE_DELETE      |
| NK_ERROR__NO_MEMORY                 | NK_ERROR__POOL_O_NO_FREE_POOL       |
| ^                                   | NK_ERROR__POOL_O_NO_FREE_BLOCK      |
| NK_ERROR__ILGL_CREATE               | NK_ERROR__POOL_S_ILGL_CREATE        |
| ^                                   | NK_ERROR__TIMER_S_ILGL_CREATE       |
| ^                                   | NK_ERROR__POOL_S_ILGL_DELETE        |

### Specific error generation

Component base values:

| Component name | Component base |
| -------------- | -------------: |
| pool           |      1000 0000 |
| timer          |      2000 0000 |
| flag           |      3000 0000 |
| queue          |      4000 0000 |
| mutex          |      5000 0000 |
| sem            |      6000 0000 |
| mbox           |      7000 0000 |
| task           |      8000 0000 |

Class base and class shift values:

| Class name               | Class base | Class shift |
| ------------------------ | ---------: | ----------: |
| precondition errors (P)  |  0010 0000 |          20 |
| postcondition errors (O) |  0000 4000 |          14 |
| safety errors (S)        |  0000 0100 |           8 |

The specific error code is generated by following formula:

    error_code = [component base] + [class base] + [index << class shift] + [general error]
    
Specific error values:

| Specific error                      | Component base | Class base | Class shift | Index | General error | Code      | 
| ----------------------------------- | -------------: | ---------: | ----------: | ----: | ------------: | --------: |
| NK_ERROR__POOL_P_INVLD_POOL         |      1000 0000 |  0010 0000 |          20 |     0 |            01 | 1010 0001 |
| NK_ERROR__POOL_P_INVLD_BLOCK        |              ^ |          ^ |           ^ |     1 |            01 | 1020 0001 |
| NK_ERROR__POOL_P_INVLD_N_BLOCKS     |              ^ |          ^ |           ^ |     2 |            01 | 1030 0001 |
| NK_ERROR__POOL_P_INVLD_BLOCK_SIZE   |              ^ |          ^ |           ^ |     3 |            01 | 1040 0001 |
| NK_ERROR__POOL_P_INVLD_STORAGE      |              ^ |          ^ |           ^ |     4 |            01 | 1050 0001 |
| NK_ERROR__POOL_P_INVLD_NAME         |              ^ |          ^ |           ^ |     5 |            01 | 1060 0001 |
| NK_ERROR__POOL_P_INVLD_FREE         |              ^ |          ^ |           ^ |     6 |            01 | 1070 0001 |
| NK_ERROR__POOL_P_INVLD_TOTAL        |              ^ |          ^ |           ^ |     7 |            01 | 1080 0001 |
| NK_ERROR__POOL_P_INVLD_OCCUPIED     |              ^ |          ^ |           ^ |     8 |            01 | 1090 0001 |
| NK_ERROR__POOL_P_CALLED_FROM_ISR    |              ^ |          ^ |           ^ |     9 |            06 | 10a0 0006 |
| NK_ERROR__POOL_P_IN_USE_DELETE      |              ^ |          ^ |           ^ |     a |            05 | 10b0 0005 |
| NK_ERROR__POOL_O_NO_FREE_POOL       |              ^ |  0000 4000 |          14 |     0 |            04 | 1000 4004 |
| NK_ERROR__POOL_O_NO_FREE_BLOCK      |              ^ |          ^ |           ^ |     1 |            04 | 1000 8004 |
| NK_ERROR__POOL_S_ILGL_CREATE        |              ^ |  0000 0100 |           8 |     0 |            03 | 1000 0103 |
| NK_ERROR__POOL_S_ILGL_DELETE        |              ^ |          ^ |           ^ |     1 |            03 | 1000 0203 |
| NK_ERROR__TIMER_P_INVLD_TIME_MS     |      2000 0000 |  0010 0000 |          20 |     0 |            01 | 2010 0001 |
| NK_ERROR__TIMER_P_INVLD_CALLBACK    |              ^ |          ^ |           ^ |     1 |            01 | 2020 0001 |
| NK_ERROR__TIMER_P_INVLD_TIMER       |              ^ |          ^ |           ^ |     2 |            01 | 2030 0001 |
| NK_ERROR__TIMER_O_NO_FREE_TIMER     |              ^ |  0000 4000 |          14 |     0 |            04 | 2000 4004 |
| NK_ERROR__TIMER_S_ILGL_CREATE       |              ^ |  0000 0100 |           8 |     0 |            03 | 2000 0103 |