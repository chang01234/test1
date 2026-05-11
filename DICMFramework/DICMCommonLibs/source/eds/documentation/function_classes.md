# Function classes

## Execution context

A function may be called from different execution contextes. We differentiate the following 
execution contextes:
- __thread execution__ - This execution context is started automatically in main function. Even when
  no RTOS is being used this context is still called `thread` context.
- __interrupt service execution__ - This context is present in interrupt service routines.

## Execution class

The thread context can be interrupted by interrupt context. In every application scenario there is
a need to share data between thread and interrupt context. In order to provide data consistency, a
some kind of data protection mechanism is being used (an interrupt lock or an RTOS construct, like 
mutex or semaphore) to protect and serialize the access to the data. 

In order to provide data protection mechanism we can have different levels of locking:
- __system lock__ - thread-to-thread context switching is disabled. Interrupts are still being
  serviced, but when they return, it is guranteed that the execution will return to the interrupted
  thread.
- __interrupt lock__ - all context switching is disabled. No Nano Kernel interrupts are serviced.
  This is the most restrictive type of lock and its usage should be minimised.
  
A function implementation usually doesn't always implement a data protection mechanism for data
access. It is expected from caller that it implements it in the most efficient way. In order to 
minimise wrong usage of functions, the function classes have been defined.

## Function classes

Depending on the data access aproach we distinguish the following function classes:
* __general function class__ - This class of functions provides all necessary data protection 
  mechanism when accessing shared data. These functions can be called from thread and interrupt 
  context without the need to implement specific data protection mechanism. Note that some functions
  do have a restriction to be called from interrupt context, however.
* __system lock function class__ or __S class__ - These functions are assuming that data is shared 
  exclusively between threads. When these kind of functions are accessing shared data, they assume 
  that thread context switching will not happen. This is usually ensured by `locking` the system 
  scheduler, hence the name `system lock` function class.
* __interrupt lock function class__ or __I class__ - These functions are assuming that the data is 
  shared between multiple threads and at least one interrupt service routine. When these kind of 
  functions are accessing shared data, they assume that thread context switching is disabled and 
  interrupt service routines are not allowed. In other words, these functions do not implement any 
  kind of data protection mechanism.
  
Some Nano Kernel components are providing the above versions of the functions. As a caller, the
application must ensure that:
- when a __S class__ function is called, that the Nano Kernel scheduler is locked.
- when a __I class__ function is called, that the Nano Kernel interrupts are disabled.
