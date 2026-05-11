/*! \file ddmp.h
	\brief Master DDMP library header

	Includes the rest of the headers and contains the manual.
*/

#ifndef DDMP_H_
#define DDMP_H_

#include "ddm.h"
#include "encapsulation.h"
#include "parameters.h"

#endif /* DDMP_H_ */

/*! \defgroup Manual DDMP Library manual
@{ 
Description
========================

The DDMP library aims to be portable and run regardless of architecture and operating system, to do as much of the protocol heavy-lifting as possible and to be easy to integrate to.

To be able to use hardware and operating system functions while being portable, the library employs a few callback functions where system-specific code is put.

- Automatic link state handling. Acknowledgements and the like are sent behind the scenes.
- Platform-specific functions are connected to the library by a number of callback functions.
- The library functions work in nonblocking mode; queuing up work where applicable or fail when a resource isn't readily available.
- Only little-endian architechtures supported as of now.
- Requires C99 support.
- Send functions are reentrant if locks are used.

Basic function
========================

Interface
------------------------

![Caption text](../../ddmp_library_interface.png)

The protocol stack needs to be initialized with ddmp_initialize() before it is used. To this function you need to supply your callback functions and interface definitions.

The basic protocol data unit is called a frame, as described in the Dometic Data Model Protocol specification (8.1). To be able to handle this variable-size data unit,
the size of the frame needs to be known. For this purpose another data structure is defined, called a parameter, which combines a size with a frame.
\sa DDMP_FRAME
\sa DDMP_FRAME_BUFFER

To send a parameter out of a interface, call ddmp_send() and supply which interface and a pointer to the parameter to send. This is a synchronous operation as it is the application 
program that decides when to send somehting. There are also helper functions to send specific action frames and to create parameter data structures.
\sa ddmp_send_multiple()
\sa create_frame()
\sa ddmp_publish()
\sa ddmp_subscribe()
\sa ddmp_publish_reply()
\sa ddmp_subscribe_reply()

To input data into the library you first need to first aquire the data from the interface platform-specifically and then input it into the library with ddmp_receive. This is
an asynchronous operation as the application program does not decide when to be sent data from the neightbour.
\sa Interface

Integration
========================

To be able to use the library, the global callbacks (DDMP_ERROR_CB is optional), an interface callback for each used interface (only on in an end device) and initialize the library with this information. ddmp_initialize() takes a struct collecting pointers to these functions so that the library can call them as needed.

To send a parameter, simply call ddmp_send().

To receive, call for example ddmp_receive_uart() with data acquired from the interface by platform-specific means (possibly an ISR).

Callback functions
------------------------

Callback functions connect the library to the specific platform it is running on.

\ref DDMP_FRAME_CB is called when ddmp_receive() has received a complete frame. Here you can filter and process incoming frames and reply to subscriptions.

\ref DDMP_ERROR_CB is called whenever an error occurs. Log/handle errors here.

\ref DDMP_TIMER_CB is called whenever the library has sent something and needs to tell if the reception of an acknowledgement times out. The same function is used to stop the timer when acknowledgement has been received. When this timer runs out, it is to call ddmp_retransmit() to cause a retransmission of the previous frame.

\ref DDMP_SEND_CB is a per-interface callback that is called when the library wants to send data out of its respective interface.

\ref DDMP_CONNECT_CB is called when a logical connection is (re)established.

\ref DDMP_GLOBAL_LOCK is called when the stack needs to enter a critical section.

\ref DDMP_GLOBAL_UNLOCK is called when the stack needs to leave a critical section.

\ref DDMP_LOCK is called when the stack needs to enter a critical section on a single interface (optional).

\ref DDMP_UNLOCK is called when the stack needs to leave a critical section on a single interface (optional).

\sa Callbacks

Examples
------------------------

Eight example implementation tools for Windows are supplied:

- pubsub_cc - A simple simulation of a Compressor controller. Commented.
- pubsub_hmi - A simple simulation of a HMI device. Commented.
- ddmp_peer - A simple passive ddmp peer that keeps link state but does not speak by itself.
- ddmp_loopback - A peer that echoes back anything. Does not follow link protocols.
- ddmp_sniffer - A passive peer that only listens to the incoming serial link.
- ddmp_sendframe - A peer that can send frames input in hex by keyboard to an UART interface.
- ddmp_json - A peer that can send frames input in hex by keyboard over WiFi/JSON.
- ddmp_gui - A graphical DDMP control panel; can send and monitor DDMP traffic.

Known Issues:
========================

- inttypes.h may not be available on all platforms

Release history
========================
2.2 - 2019-09-05
	- Made queue handling slightly safer by putting full check into lock

2.1 - 2019-07-29
	- Added access to actual value in assert macro

2.0 - 2019-06-11
	- Small fix in ddm_update_and_publish() for when new size is different from current

1.9 - 2019-06-11
	- Added ARGUMENT_ERROR to DDMP library to indicate illegal arguments to function calls
	- Added sanity checking to source frame size in ddmp_update_and_publish()
	- Fixed pubsub_* simulators and updated and updated to generate data for history

1.8 - 2019-06-04
	- Removed duplicate DDMP_ERROR_CB declaration in ddm.h

1.7 - 2019-05-27
	- Added DDMP_ACTION_NOP
	- Added SUB_INIT() macro
	- Fixed problem in GUI preventing startup if parallel port was present in system

1.6 - 2019-04-16
	- Added ddmp_send() error strings and a function to retreive them, ddmp_send_error_string()

1.5 - 2019-04-14
	- Fixed critical memory corruption bug in ddmp_receive_uart() for multi-interface systems (broker)
	- As a result of this, changed the signature of ddmp_receive_uart(); added offset of first UART in interface list. Add ,0 for end devices
	- Fixed callback initialization bug in ddmp_initialize()

1.4 - 2019-04-04
	- Added SUB_ARRAY macro

1.3 - 2019-04-04
	- Fix bug in ddmp_subscribe_reply() sending out values with the subscriptions

1.2 - 2019-03-14
	- Frame queues of wireless interfaces are cleared when physical connection goes down
	- If one of the two outgoing queues is full, try the other one before failing

1.1 - 2019-03-14
	- Added listen mode argument to ddmp_initialize. 0=normal mode, 1=listen only mode (act as sniffer)
	- Removed possible resending of ACKs

1.0 - 2019-02-24
	- Fixed unit name display bug
	- Added recommended temperature range parameter (-18, 3)
	- Readded connection error parameter

0.9 - 2019-02-12
	- Added the function ddm_update_and_publish() to easily generate new parameter value frames
	- Extended HMI and CC examples to use this function and to declare a more complete parameter store
	- Implemented queue logic to not throw away the frame when raising connection error

0.81 - 2019-02-03
	- Renamed ddmp_receive() to ddmp_receive_uart() to prepare for other protocol inputs
	- A broken connection only generates one connection error

0.8 - 2019-01-30
	- Reintroduced locks, global and interface local
	- Added a function ddm_accept_value() to accept values and copy them into local store
	- Added a function ddm_missing_value() to check a value of a frame to the MISSING_VALUE
	- Added an example Windows implementation that can send manually input frames

0.6 - 2019-01-18
	- Added timeout error

0.5 - 2019-01-17
	- Queue architechture revised
	- Implementation of protocol according to DDMP specification c

0.2 - 2018-12-19
	- Rewrite to a non-blocking queue architechture
	- Removed lock and event callbacks

0.1 - 2018-11-27
	- Initial release

@} */
