// o2.h -- public header file for o2 system
// Roger B. Dannenberg and Zhang Chi
// see license.txt for license
// June 2016

/** \file o2.h
\mainpage
\section Introduction

This documentation is divided into modules. Each module describes a
different area of functionality: Basics, Return Codes, Low-Level
Message Send, and Low-Level Message Parsing.

\section Overview

O2 is a communication protocol for interactive music and media
applications. O2 is inspired by Open Sound Control (OSC) and uses
similar means to form addresses, specify types, and encode messages.

However, in addition to providing message delivery, O2 offers a
discovery mechanism where processes automatically discover and connect
to other processes. Each process can offer zero or more named
"services," which are top-level nodes in a global, tree-structured
address space for a distributed O2 application. In O2, services
replace the notion of network addresses (e.g. 128.2.100.57) in OSC.

O2 is based on IP (Internet Protocol), but there are some mechanisms
that allow an O2 process to serve as a bridge to other networks such
as Bluetooth.

O2 addresses begin with the service name. Thus, a complete O2 address
would be written simply as "/synth/filter/cutoff," where "synth" is
the service name.

Furthermore, O2 implements a clock synchronization protocol. A single
process is designated as the "master," and other processes
automatically synchronize their local clocks to the master. All O2
messages are timestamped. Messages are delivered immediately, but
their designated operations are invoked according to the timestamp. A
timestamp of zero (0.0) means deliver the message
immediately. Messages with non-zero timestamps are only deliverable
after both the sender and receiver have synchronized clocks.

A service is created using the functions: o2_add_service(service_name)
and

    o2_add_method("address," "types," handler, user_data, coerce, parse),

where o2_add_method is called to install a handler for each node, and each
"address" includes the service name as the first node.

Some major components and concepts of O2 are the following:

- **Application** - a collection of collaborating processes are called
  an "application." Applications are named by a simple ASCII string.
  In O2, all components belong to an application, and O2 supports
  communication only *within* an application. This allows multiple
  independent applications to co-exist and share the same local area
  network.

- **Host** - (conventional definition) a host is a computer (virtual
  or real) that may host multiple processes. Essentially, a Host is
  equivalent to an IP address.

- **Process** - (conventional definition) an address space and one or
  more threads. A process can offer one or more O2 services and
  can serve as a client of O2 services in the same or other processes.
  Each process using O2 has one directory of services shared by all
  O2 activity in that process (thus, this O2 implementation can be
  thought of as a "singleton" object). The single O2 instance in a
  process belongs to one and only one application. O2 does not support
  communication between applications.

- **Service** - an O2 service is a named server that receives and acts
  upon O2 messages. A service is addressed by name. Multiple services
  can exist within one process. A service does not imply a (new) thread
  because services are "activated" by calling o2_poll(). It is up to
  the programmer ("user") to call o2_poll() frequently, and if this
  is done using multiple threads, it is up to the programmer to deal
  with all concurrency issues. O2 is not reentrant, so o2_poll() should
  be called by only one thread. Since there is at most one O2 instance
  per process, a single call to o2_poll() handles all services in the
  process.

- **Message** - an O2 message, similar to an OSC message, contains an address
  pattern representing a function, a type string and a set of values
  representing parameters.

- **Address Pattern** - O2 messages use URL-like addresses, as does OSC,
  but the top-level node in the hierarchical address space is the
  service name. Thus, to send a message to the "note" node of the
  "synth" service, the address pattern might be "/synth/note." The
  OSC pattern specification language is used unless the first character
  is "!", e.g. "!synth/note" denotes the same address as "/synth/note"
  except that O2 can assume that there are no pattern characters such
  as "*" or "[".

- **Scheduler** - O2 implements two schedulers for timed message
    delivery. Schedulers are served by the same o2_poll() call that
    runs other O2 activity. The #o2_gtsched schedules according to the
    application's master clock time, but since this depends on clock
    synchronization, nothing can be scheduled until clock
    synchronization is achieved (typically within a few seconds of
    starting the process that provides the master clock). For
    local-only services, it is possible to use the #o2_ltsched
    scheduler (not with o2_send(), but by explicitly constructing
    messages and scheduling them with o2_schedule(). In any case,
    scheduling is useful *within* a service for any kind of timed
    activity.

*/

#ifndef O2_H
#define O2_H

// get uint32_t, etc.:
#include <stdlib.h>
#include <stdint.h>

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#endif

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

/** \defgroup debugging Debugging Support
 * @{
 */

/// \brief Enable debugging output.
/// Unless O2_NO_DEBUG is defined at compile time, O2 is
/// compiled with debugging code that prints information to
/// stdout, including network addresses, services discovered,
/// and clock synchronization status. Enable the debugging
/// information by setting o2_debug to 1 for basic connection
/// data, 2 for tracing user messages sent and received, and 3
/// for tracing clock-sync and (perhaps) discovery messages.
extern int o2_debug;

/** @} */

/** \defgroup returncodes Return Codes
 * @{
 */

// Status return values used in o2 functions
#define O2_SUCCESS 0    ///< function was successful

/// \brief an error return value: a non-specific error occurred.
/// In general, any return value <0 indicates an error. Testing for
/// only O2_FAIL will not detect more specific error return values
/// such as O2_SERVICE_CONFLICT, O2_NO_MEMORY, etc.
#define O2_FAIL (-1)

// TODO: this value is never used/returned
/// an error return value: path to handler specifies a remote service
#define O2_SERVICE_CONFLICT (-2)

// TODO: this value is never used/returned
/// an error return value: path to handler specifies non-existant service
#define O2_NO_SERVICE (-3)

/// an error return value: process is out of free memory
#define O2_NO_MEMORY (-4)

/// an error return value for o2_initialize(): O2 is already running.
#define O2_RUNNING (-5)

/// an error return value for o2_initialize(): invalid name parameter.
#define O2_BAD_NAME (-6)

/// an error return value for o2_initialize(): the socket is closed.
#define O2_TCP_HUP (-7)


// Status return codes for o2_status function:

/// \brief return value for o2_status() function: this is a local service
/// but clock sync has not yet been established
#define O2_LOCAL_NOTIME 0

/// \brief return value for o2_status() function: this is a remote service
/// but clock sync has not yet been established. The remote service
/// may represent a bridge to a non-IP destination or to an OSC
/// server.
#define O2_REMOTE_NOTIME 1

/// \brief return value for o2_status() function: this service is connected.
/// The service is attached to this process by non-IP link. Clock sync
/// has not yet been established between the master clock and this
/// process, so timestamped messages to this service will be
/// dropped. Note that within other processes,
/// the status for this service will be #O2_REMOTE_NOTIME rather than
/// #O2_BRIDGE_NOTIME. Note also that O2 does not require the
/// remote bridged process to have a synchronized clock, so "NOTIME" only
/// means that *this* process is not synchronized and therefore cannot
/// (and will not) schedule a timestamped message for timed delivery.
#define O2_BRIDGE_NOTIME 2

/// \brief return value for o2_status() function: this service is connected.
/// The service forwards messages to an OSC server. The status of the
/// OSC server is not reported by O2 (and in the typical UDP case,
/// there is no way to determine if the OSC server is operational, so
/// "connected" may just mean that the service has been defined).
/// Clock sync has not yet been established between the master clock
/// and this process, so timestamped messages to this service will be
/// dropped. Note that within other processes,
/// the status for this service will be #O2_REMOTE_NOTIME rather than
/// #O2_TO_OSC_NOTIME. Note also that O2 does not require the
/// OSC server to have a synchronized clock, so "NOTIME" only
/// means that *this* process is not synchronized and therefore cannot
/// (and will not) schedule a timestamped message for timed delivery.
/// (There is no standard for clock synchronization in OSC; therefore,
/// one should not expect the status to ever change to O2_TO_OSC.)
#define O2_TO_OSC_NOTIME 3

/// \brief return value for o2_status() function: this is a local service
/// and clock sync has been established. Note that even though the
/// service is local to the process and therefore shares a local
/// clock, clocks are not considered to be synchronized until the
/// local clock is synchronized to the master clock. If this process
/// provides the master clock, it is considered to be synchronized
/// immediately.)
#define O2_LOCAL 4

/// \brief return value for o2_status() function: this is a remote service
/// and clock sync has been established.
#define O2_REMOTE 5

/// \brief return value for o2_status() function: this service is connected.
/// The service is attached by non-IP link, and this process is synchronized.
/// If the bridged process is also synchronized, timed messages are
/// sent immediately and dispatched according to the synchronized
/// clock; if the bridged process is *not* synchronized, timed
/// messages are scheduled locally and sent according to the
/// timestamp, resulting in some added network latency.
#define O2_BRIDGE 6

/// \brief return value for o2_status() function: this service is connected.
/// The service forwards messages to an OSC server, and this process
/// is synchronized. The status of the OSC server is not reported by
/// O2 (and in the typical UDP case, there is no way to determine if
/// the OSC server is operational). Timed messages will be scheduled
/// locally and sent according to the timestamp, resulting in some
/// added network latency. If O2 timestamps can be converted to valid
/// OSC timestamps, messages should be forwarded immediately and
/// scheduled in the OSC server, but OSC clock synchronization is not
/// standardized or commonly used, so do not expect this behavior.
#define O2_TO_OSC 7

/** @} */


// Macros for o2 protocol
//#ifdef WIN32
//#define o2_send o2_send_marker
//#define o2_send_cmd o2_send_cmd_marker
//#define o2_send_osc_message o2_send_osc_message_marker
//#else
/* an internal value, ignored in transmission but check against O2_MARKER in the
 * argument list. Used to do primitive bounds checking */
#define O2_MARKER_A (void *) 0xdeadbeefdeadbeefL
#define O2_MARKER_B (void *) 0xf00baa23f00baa23L
//#endif

extern void *((*o2_malloc)(size_t size));
extern void ((*o2_free)(void *));
void *o2_calloc(size_t n, size_t s);

/** \defgroup basics Basics
 * @{
 */

/** \brief allocate memory
 *
 * O2 allows you to provide custom heap implementations to avoid
 * priority inversion or other real-time problems. Normally, you
 * should not need to explicitly allocate memory since O2 functions
 * are provided to allocate, construct, and deallocate messages, but
 * if you need to allocate memory, especially in an O2 message
 * handler callback, i.e. within the sphere of O2 execution, you
 * should use #O2_MALLOC, #O2_FREE, and #O2_CALLOC.
 */
#define O2_MALLOC(x) (*o2_malloc)(x)

/** \brief free memory allocated by #O2_MALLOC */
#define O2_FREE(x) (*o2_free)(x)

/** \brief allocate and zero memory (see #O2_MALLOC) */
#define O2_CALLOC(n, s) o2_calloc((n), (s))

/** \brief O2 timestamps are doubles representing seconds since the
 * approximate start time of the application.
 */
typedef double o2_time;


/** \brief an O2 message
 *
 */
typedef struct o2_message {
  struct o2_message *next; ///< links used for free list and scheduler
  int allocated;           ///< how many bytes allocated in data part
  int length;              ///< the length of the message in data part
  struct {
    o2_time timestamp;   ///< the message delivery time (0 for immediate)
    /** \brief the message address string
     *
     * Although this field is declared as 4 bytes, actual messages
     * have variable length, and the address is followed by a
     * string of type codes and the actual parameters. The length
     * of the entire message including the timestamp is given by
     * the `length` field.
     */
    char address[4];
  } data; ///< the message (type string and payload follow address)
} o2_message, *o2_message_ptr;


/**
 *  \brief The structure for binary large object.
 *
 *  A blob can be passed in an O2 message using the 'b' type. Created
 *  by calls to o2_blob_new().
 */
typedef struct o2_blob {
  uint32_t size;  ///< size of data
  char data[4];   ///< the data, actually of variable length
} o2_blob, *o2_blob_ptr;


/**
 *  \brief An enumeration of the O2 message types.
 */
typedef enum {
  // basic O2 types
  O2_INT32 =     'i',     ///< 32 bit signed integer.
  O2_FLOAT =     'f',     ///< 32 bit IEEE-754 float.
  O2_STRING =    's',     ///< NULL terminated string (Standard C).
  O2_BLOB =      'b',     ///< Binary Large OBject (BLOB) type.

  // extended O2 types
  O2_INT64 =     'h',     ///< 64 bit signed integer.
  O2_TIME  =     't',     ///< OSC time type.
  O2_DOUBLE =    'd',     ///< 64 bit IEEE-754 double.
  O2_SYMBOL =    'S',     ///< Used in systems distinguish strings and symbols.
  O2_CHAR =      'c',     ///< 8bit char variable (Standard C).
  O2_MIDI =      'm',     ///< 4 byte MIDI packet.
  O2_TRUE =      'T',     ///< Sybol representing the value True.
  O2_FALSE =     'F',     ///< Sybol representing the value False.
  O2_NIL =       'N',     ///< Sybol representing the value Nil.
  O2_INFINITUM = 'I',     ///< Sybol representing the value Infinitum.

  // O2 types
  O2_BOOL =      'B'      ///< Boolean value returned as either 0 or 1
} o2_type, *o2_type_ptr;


/**
 * \brief union of all O2 parameter types
 *
 * An o2_arg_ptr is a pointer to an O2 message argument. If argument
 * parsing is requested (by setting the parse parameter in o2_add_method),
 * then the handler receives an array of o2_arg_ptrs. If argument parsing
 * is not requested, you have the option of parsing the message one
 * parameter at a time by calling o2_get_next(), which returns an
 * o2_arg_ptr.
 *
 * The o2_arg_ptr can then be dereferenced to obtain a value of the
 * expected type. For example, you could write
 * \code{.c}
 *     double d = o2_get_next()->d;
 * \endcode
 * to extract a parameter of type double. (This assumes that the message
 * is properly formed and the type string indicates that this parameter is
 * a double, or that type coercion was enabled by the coerce flag in
 * o2_add_method().)
 */
typedef union {
  int32_t    i32;  ///< 32 bit signed integer.
  int32_t    i;    ///< an alias for i32
  int64_t    i64;  ///< 64 bit signed integer.
  int64_t    h;    ///< an alias for i64
  float      f;    ///< 32 bit IEEE-754 float.
  float      f32;  ///< an alias for f
  double     d;    ///< 64 bit IEEE-754 double.
  double     f64;  ///< an alias for d
  char       s[4]; ///< Standard C, NULL terminated string.
  /** \brief Standard C, NULL terminated, string.
             Used in systems which distinguish strings and symbols. */
  char       S[4];
  int        c;    ///< Standard C, 8 bit, char, stored as int.
  uint8_t    m[4]; ///< A 4 byte MIDI packet.
  o2_time    t;    ///< TimeTag value.
  o2_blob    b;    ///< a blob (unstructured bytes)
  int32_t    B;    ///< a boolean value, either 0 or 1
} o2_arg, *o2_arg_ptr;


/** \brief name of the application
 *
 * A collection of cooperating O2 processes forms an
 * *application*. Applications must have unique names. This allows
 * more than one application to exist within a single network without
 * conflict. For example, there could be two applications, "joe" and
 * "sue", each with services named "synth." Since the application
 * names are different, joe's messages to the synth service go to
 * joe's synth and not to sue's synth.
 *
 * Do not set, modify or free this variable! Consider it to be
 * read-only. It is managed by O2 using o2_initialize() and o2_finish().
 */
extern char *o2_application_name;

/** \brief set this flag to stop o2_run()
 *
 * Some O2 applications will initialize and call o2_run(), which is a
 * simple loop that calls o2_poll(). To exit the loop, set
 * #o2_stop_flag to #TRUE
 */
extern int o2_stop_flag;


/**
 *  \brief callback function to receive an O2 message
 *
 * @param msg The full message in host byte order.
 * @param types If you set a type string in your method creation call,
 *              then this type string is provided here. If you did not
 *              specify a string, types will be the type string from the
 *              message (without the initial ','). If parse_args and
 *              coerce_flag were set in the method creation call,
 *              types will match the types in argv, but not necessarily
 *              the type string or types in msg.
 * @param argv An array of #o2_arg types containing the values, e.g. if the
 *             first argument of the incoming message is of type 'f' then
 *             the value will be found in argv[0]->f. (If parse_args was
 *             not set in the method creation call, argv will be NULL.)
 * @param argc The number of arguments received. (This is valid even if
 *             parse_args was not set in the method creation call.)
 * @param user_data This contains the user_data value passed in the call
 *             to the method creation call.
 * @return O2_SUCCESS (0) indicates that the message was succesfully
 *         handled. This value is currently ignored.
 *
 */
typedef int (*o2_method_handler)(const o2_message_ptr msg, const char *types,
                                 o2_arg_ptr *argv, int argc, void *user_data);


/**
 *  \brief Start O2.
 *
 *  If O2 has not been initialized, it is created and intialized.
 *  O2 will begin to establish connections to other instances
 *  with a matching application name.
 *
 *  @param application_name the name of the application. O2 will attempt to
 *  discover other processes with a matching application name,
 *  ignoring all processes with non-matching names.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if an error occurs,
 *  #O2_RUNNING if already running, #O2_BAD_NAME if `application_name`
 *  is NULL.
 */
int o2_initialize(char *application_name);


/**
 * \brief Tell O2 how to allocate/free memory.
 *
 * In many C library implementations, the standard implementation of
 * free() must lock a data structure. This can lead to priority
 * inversion if O2 runs at an elevated priority. Furthermore, the
 * standard `malloc()` and `free()` do not run in constant (real) time. To
 * avoid these problems, you can provide an alternate heap
 * implementation for O2 by calling this function before calling
 * o2_initialize(). For example, the provided functions can implement
 * a private heap for the thread running O2.
 *
 * @param malloc a function pointer that behaves like standard
 *     `malloc()`
 * @param free a function pointer that behaves like standard `free()`
 *
 * @return O2_SUCCESS if succeed, O2_FAIL if not.
 */
int o2_memory(void *((*malloc)(size_t size)), void ((*free)(void *)));


/**
 *  \brief Add a service to the current application.
 *
 * Once created, services are "advertised" to other processes with
 * matching application names, and messages are delivered
 * accordingly. E.g. to handle messages addressed to "/synth/volume"
 * you call
 * \code{.c}
 * o2_add_service("synth");
 * o2_add_method("/synth/volume", "f", synth_volume_handler, NULL, NULL, TRUE);
 * \endcode
 * and define `synth_volume_handler` (see the type declaration for
 * #o2_method_handler and o2_add_method())
 *
 *  @param service_name the name of the service
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
int o2_add_service(char *service_name);


/**
 * \brief Add a handler for an address.
 *
 * @param path      the address including the service name
 * @param typespec  the types of parameters, use "" for no parameters and
 *                      NULL for no type checking
 * @param h         the handler
 * @param user_data pointer saved and passed to handler
 * @param coerce    is true if you want to allow automatic coercion of types.
 *                      Coercion is only enabled if both coerce and parse are
 *                      true.
 * @param parse     is true if you want O2 to construct an argv argument
 *                      vector to pass to the handle
 *
 * @return O2_SUCCESS if succeed, O2_FAIL if not.
 */
int o2_add_method(const char *path, const char *typespec,
                  o2_method_handler h, void *user_data, int coerce, int parse);


/**
 *  \brief Process current O2 messages.
 *
 *  Since O2 does not create a thread and O2 requires active processing
 *  to establish and maintain connections, the O2 programmer (user)
 *  should call o2_poll() periodically, even if not offering a service.
 *  o2_poll() runs a discovery protocol to find and connect to other
 *  processes, runs a clock synchronization protocol to establish valid
 *  time stamps, and handles incoming messages to all services. O2_poll()
 *  should be called at least 10 times per second. Messages can only be
 *  delivered during a call to o2_poll() so more frequent calls will
 *  generally lower the message latency as well as the accuracy of the
 *  clock synchronization (at the cost of greater CPU utilization).
 *  Human perception of timing jitter is on the order of 10ms, so
 *  polling rates of 200 to 1000 are advised in situations where
 *  rhythmic accuracy is expected.
 *
 *  @return 0 (O2_SUCCESS) if succeed, -1 (O2_FAIL) if not.
 */
int o2_poll();

/**
 * \brief Run O2.
 *
 * Call o2_poll() at the rate (in Hz) indicated.
 * Returns if a handler sets #o2_stop_flag to non-zero.
 */
int o2_run(int rate);

/**
 * \brief Check the status of the service.
 *
 *  @param service the name of the service
 * @return
 * - #O2_FAIL if no service is found,
 * - #O2_LOCAL_NOTIME if the service is local but we have no clock sync yet,
 * - #O2_REMOTE_NOTIME if the service is remote but we have no clock sync yet,
 * - #O2_BRIDGE_NOTIME if service is attached by a non-IP link, but we have
 *        no clock sync yet (if the non-IP connection is not handled
 *        by this process, the service status will be #O2_REMOTE_NOTIME),
 * - #O2_TO_OSC_NOTIME if service forwards to an OSC server but we
 *        have no clock sync yet (if the OSC connection is not handled
 *        by this process, the service status will be #O2_REMOTE_NOTIME),
 * - #O2_LOCAL if service is local and we have clock sync,
 * - #O2_REMOTE if service is remote and we have clock sync,
 * - #O2_BRIDGE if service is handled locally by forwarding to an
 *        attached non-IP link, and we have clock sync. (If the non-IP
 *        connection is not local, the service status will be #O2_REMOTE).
 * - #O2_TO_OSC if service is handled locally by forwarding to an OSC
 *       server and this process has clock sync. (If the OSC
 *       connection is not handled locally, the service status will be
 *       #O2_REMOTE).
 *
 *  @return Note that codes are carefully
 * ordered to allow testing for categories:
 * - to test if delivery is possible with a zero (immediate) timestamp,
 * use `o2_status(service) > O2_FAIL`, `o2_status(service) >= 0`, or
 * `o2_status(service) >= O2_LOCAL_NOTIME`.
 * - to test if delivery is possible with a non-zero timestamp, use
 * `o2_status(service) >= O2_LOCAL`. Note that status can change over
 * time, e.g. the
 * status of a remote service will be #O2_FAIL until the service is
 * discovered. It will then change to #O2_REMOTE_NOTIME until both the
 * sender and receiver achieve clock synchronization and share their
 * synchronized status, and finally the status will become #O2_REMOTE.
 *
 * In the cases with no clock sync, it is safe to send an immediate message
 * with timestamp = 0, but non-zero timestamps are meaningless because
 * either the sending process has no way to obtain a valid timestamp
 * or the receiver has no way to schedule delivery according to a
 * timestamp.
 *
 * Messages to services are *dropped* if the service has not been
 * discovered. Timestamped messages (timestamp != 0) are *dropped* if
 * the sender and receiver are not
 * clock-synchronized. (`o2_status(service) >= O2_LOCAL`).
 *
 * A special case is with `BRIDGE` and `OSC` services. In these cases,
 * the O2 process offering the service can either schedule the
 * messages locally, sending them according to the timestamp (and
 * suffering some network latency), or if the destination process is
 * synchronized, messages can be forwarded immediately for more
 * precise scheduling at their final destination. O2 does not provide
 * any way for clients/users to determine which of these methods is in
 * effect, and in the case of messages being forwarded by an
 * intermediary O2 process, the originator of the message cannot
 * determine whether the service is offered by an O2 server on the
 * local network, by an OSC server, or through a bridge to another
 * network such as Bluetooth. The status at the originator will be
 * simply #O2_REMOTE or #O2_REMOTE_NOTIME.
 */
int o2_status(const char *service);


/**
 *  \brief Get network round-trip information.
 *
 * @return If clock is synchronized, return O2_SUCCESS and set
 *   `*mean` to the mean round-trip time and `*min` to the minimum
 *   round-trip time of the last 5 (where 5 is the value of
 *   CLOCK_SYNC_HISTORY_LEN) clock sync requests. Otherwise,
 *   O2_FAIL is returned and `*mean` and `*min` are unaltered.
 */
int o2_roundtrip(double *mean, double *min);


/** \brief signature for callback that defines the master clock
 *
 * See o2_set_clock() for details.
 */
typedef o2_time (*o2_time_callback)(void *rock);


/**
 *  \brief Provide a time reference to O2.
 *
 *  Exactly one process per O2 application should provide a master
 *  clock. All other processes synchronize to the master. To become
 *  the master, call o2_set_clock(). 
 *
 *  The time reported by the gettime function will be offset to 
 *  match the current local time so that local time continues to 
 *  increase smoothly. You cannot force O2 time to match an external 
 *  absolute time, but once o2_set_clock() is called, the difference
 *  between the time reference and O2's local time (as reported by 
 *  o2_local_time()) will be fixed.
 *
 *  @param gettime function to get the time in units of seconds. The
 *  reference may be operating system time, audio system time, MIDI
 *  system time, or any other time source. The times returned by this
 *  function must be non-decreasing and must increase by one second
 *  per second of real time to close approximation. The value may be
 *  NULL, in which case a default time reference will be used.
 *
 *  @parm rock an arbitrary value that is passed to the gettime
 *  function. This may be need to provide context. Use NULL if no
 *  context is required.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
int o2_set_clock(o2_time_callback gettime, void *rock);


/**
 * \brief Construct and send O2 message with best effort protocol
 *
 *  Normally, this constructs and sends an O2 message via UDP. If the
 *  destination service is reached via some other network protocol
 *  (e.g. Bluetooth), the message is delivered in the lowest latency
 *  protocol available, with no guaranteed delivery.
 *
 *  @param path an address pattern
 *  @param time when to dispatch the message, 0 means right now. In any
 *  case, the message is sent to the receiving service as soon as
 *  possible. If the message arrives early, it will be held at the
 *  service and dispatched as soon as possible after the indicated time.
 *  @param typestring the type string for the message. Each character
 *  indicates one data item. Type codes are as in OSC.
 *  @param ...  the data of the message. There is one parameter for each
 *  character in the typestring.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 *
 */
/** \hideinitializer */ // turn off Doxygen report on o2_send_marker()
#define o2_send(path, time, typestring, ...) \
    o2_send_marker(path, time, FALSE, typestring, __VA_ARGS__, O2_MARKER_A, O2_MARKER_B)

/** \cond INTERNAL */ \
int o2_send_marker(char *path, double time, int tcp_flag, char *typestring, ...);
/** \endcond */

/**
 * \brief Construct and send an O2 message reliably.
 *
 *  Normally, this constructs and sends an O2 message via TCP. If the
 *  destination service is reached via some other network protocol
 *  (e.g. Bluetooth), the message is delivered using the most reliable
 *  protocol available. (Thus, this call is considered a "hint" rather
 *  than an absolute requirement.)
 *
 *  @param path an address pattern
 *  @param time when to dispatch the message, 0 means right now. In any
 *  case, the message is sent to the receiving service as soon as
 *  possible. If the message arrives early, it will be held at the
 *  service and dispatched as soon as possible after the indicated time.
 *  @param typestring the type string for the message. Each character
 *  indicates one data item. Type codes are defined by #o2_type.
 *  @param ...  the data of the message. There is one parameter for each
 *  character in the typestring.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
/** \hideinitializer */ // turn off Doxygen report on o2_send_marker()
#define o2_send_cmd(path, time, typestring, ...) \
    o2_send_marker(path, time, TRUE, typestring, __VA_ARGS__, O2_MARKER_A, O2_MARKER_B)


/**
 * \brief Send an O2 message. (See also macros #o2_send and #o2_send_cmd).
 *
 * @param msg points to an O2 message.
 * @param tcp_flag is true for reliable send, false for best effort (UDP) send.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 *
 * After the call, the `msg` parameter is "owned" by O2, which will
 * free it.
 */
int o2_send_message(o2_message_ptr msg, int tcp_flag);

/**
 * \brief Get the estimated synchronized global O2 time.
 *
 *  This function returns a valid value either after you call
 *  o2_set_clock(), making the local clock the master clock for the O2
 *  application, or after O2 has finished discovering and
 *  synchronizing with the master clock. Until then, -1 is returned.
 *
 *  The clock accuracy depends upon network latency, how often
 *  o2_poll() is called, and other factors, but
 *
 *  @return the time in seconds, or -1 if global (master) time is unknown.
 */
o2_time o2_get_time();


/**
 * \brief Get the real time using the local O2 clock
 *
 * @return the local time in seconds
 */
o2_time o2_local_time();

/**
 *  \brief Return text representation of an O2 error
 *
 *  @param i error number returned from some O2 function
 *
 *  @return return the error message as a string
 */
const char *o2_get_error(int i);

/**
 *  \brief release the memory and shut down O2.
 *
 *  Close all sockets, free all memory, and restore critical
 *  variables so that O2 behaves as if it was never initialized.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
int o2_finish();


// Interoperate with OSC
/**
 *  \brief Create a port to receive OSC messages.
 *
 *  OSC messages are converted to O2 messages and directed to the service.
 *  E.g. if the service is "maxmsp" and the message address is
 *  `/foo/x`, then the message is directed to and handled by
 *  `/maxmsp/foo/x`.
 *
 *  @param service_name The name of the service to which messages are delivered
 *  @param port_num     Port number.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
int o2_create_osc_port(const char *service_name, int port_num, int udp_flag);


/**
 *  \brief Send an OSC message.
 *
 * This function (mostly) bypasses O2 and just constructs a message
 *  and sends it directly via UDP to an OSC server.
 *
 *  Note: Before calling o2_send_osc_message(), you should first use
 *  o2_add_osc_service() to add in the osc service and give it a
 *  service name. Then you can use the service name to send the message.
 *
 *  @param service_name The o2 name for the remote osc server, named by calling
 *                      o2_add_osc_service().
 *  @param path         The osc path.
 *  @param typestring   The type string for the message
 *  @param ...          The data values to be transmitted.
 *
 *  @return O2_SUCCESS if success, O2_FAIL if not.
 */
/** \hideinitializer */
#define o2_send_osc_message(service_name, path, typestring, ...) \
    o2_send_osc_message_marker(service_name, path, typestring, \
                               O2_MARKER_A, O2_MARKER_B)

/** \cond INTERNAL */ \
int o2_send_osc_message_marker(char *service_name, const char *path,
                               const char *typestring, ...);
/** \endcond */

/**
 *  \brief Create a service that forwards O2 messages to an OSC server.
 *
 *  @param service_name The o2 service name.
 *  @param ip           The ip address of the osc server.
 *  @param port_num     The port number of the osc server.
 *  @param tcp_flag     Send OSC message via TCP protocol, in which case
 *                      port_num is the TCP server port, not a connection.
 *
 *  @return #O2_SUCCESS if success, #O2_FAIL if not.
 *
 *  If `tcp_flag` is set, a TCP connection will be established with
 *  the OSC server.
 *  When the created service receives any O2 messages, it will
 *  send the message to the OSC server. If the incoming message has
 *  a timestamp for some future time, the message will be held until
 *  that time, then sent to the OSC server. (Ideally, O2 will convert
 *  the message to an OSC timestamped bundle and send it immediately
 *  to achieve precise forward-synchronous timing, but this requires
 *  clock synchronization with the OSC server, which is normally
 *  unimplemented.)
 */
int o2_delegate_to_osc(char *service_name, char *ip, int port_num, int tcp_flag);

/** @} */ // end of Basics

/**
 * \defgroup lowlevelsend Low-Level Message Send
 *
 * Rather than passing all parameters in one call or letting O2
 * extract parameters from a message before calling its handler,
 * these functions allow building messages one parameter at a time
 * and extracting message parameters one at a time.
 * The functions operate on "hidden" messages, so these functions are
 * not reentrant.
 *
 * To build a message, begin by calling o2_start_send() to allocate a
 * message. Then call one of the `o2_add_()` functions to add each
 * parameter. Finally, call either o2_finish_send() or
 * o2_finish_send_cmd() to send the message. You should not explicitly
 * allocate or deallocate a message using this procedure.
 *
 * To extract parameters from a message, begin by calling
 * o2_start_extract() to prepare to get parameters from the
 * message. Then call o2_get_next() to get each parameter. If the
 * result is non-null, a parameter of the requested type was obtained
 * and you can read the parameter from the result. Scalar results are only
 * valid until the next call to o2_get_next(), so you should use or
 * copy the value before reading the next one. String values are
 * always in the message and have the same lifetime as the
 * message. o2_get_next() will perform type conversion if possible
 * when the requested type does not match the actual type. You can
 * determine the original type by reading the type string in the
 * message. The number of parameters is determined by the length of
 * the type string. Normally, you should not free the message because
 * normally you are accessing the message in a handler and the message
 * will be freed by the O2 message dispatch code that called the
 * handler.
 */

/** \addtogroup lowlevelsend
 * @{
 */
/**
 * \brief Allocate a blob.
 *
 * Allocate a blob and initialize the size field. If the return address
 * is not NULL, copy data (up to length size) to `blob->data`. You can
 * change `blob->size`, but of course you should not set `blob->size`
 * greater than the `size` parameter originally passed to o2_blob_new().
 *
 * Caller is responsible for freeing the returned blob using O2_FREE().
 *
 * A constructed blob can be added to a message. If you add parameters to
 * a message one-at-a-time, you can use o2_add_blob_data() to copy data
 * directly to a message without first allocating a blob and copying
 * data into it.
 *
 * @param size The size of the data to be added to the blob
 *
 * @return the address of the new blob or NULL if memory cannot be allocated.
 */
o2_blob_ptr o2_blob_new(uint32_t size);


/**
 * \brief Prepare to build a message
 *
 * @return #O2_SUCCESS if success, #O2_FAIL if not.
 *
 * Allocates a "hidden" message in preparation for adding
 * parameters. After calling this, you should call `o2_add_` functions
 * such as o2_add_int32() to add parameters. Then call
 * o2_finish_send() or o2_finish_send_cmd() to send the message.
 */
int o2_start_send();

/// \brief add an `int32` to the message (see o2_start_send())
int o2_add_int32(int32_t i);

/// \brief add a `float` to the message (see o2_start_send())
int o2_add_float(float f);

/// \brief add a symbol to the message (see o2_start_send())
int o2_add_symbol(char *s);

/// \brief add a string to the message (see o2_start_send())
int o2_add_string(char *s);

/// \brief add an `o2_blob` to the message (see o2_start_send()), where
///        the blob is given as a pointer to an #o2_blob object.
int o2_add_blob(o2_blob *b);

/// \brief add an `o2_blob` to the message (see o2_start_send()), where
///        the blob is specified by a size and a data address.
int o2_add_blob_data(uint32_t size, void *data);

/// \brief add an `int64` to the message (see o2_start_send())
int o2_add_int64(int64_t i);

/// \brief add a time (`double`) to the message (see o2_start_send())
int o2_add_time(o2_time t);

/// \brief add a `double` to the message (see o2_start_send())
int o2_add_double(double d);

/// \brief add a `char` to the message (see o2_start_send())
int o2_add_char(char c);

/// \brief add a short midi message to the message (see o2_start_send())
int o2_add_midi(uint8_t *m);

/// \brief add "true" to the message (see o2_start_send())
int o2_add_true();

/// \brief add a "false" to the message (see o2_start_send())
int o2_add_false();

/// \brief add 0 (false) or 1 (true) to the message (see o2_start_send())
int o2_add_bool(int i);

/// \brief add "nil" to the message (see o2_start_send())
int o2_add_nil();

/// \brief add "infinitum" to the message (see o2_start_send())
int o2_add_infinitum();

/**
 * \brief finish and return the message.
 *
 * @param time the timestamp for the message (0 for immediate)
 * @param address the O2 address pattern for the message
 *
 * @return the address of the completed message, or NULL on error
 *
 * The message must be freed using o2_free_message() or by calling
 * o2_send_message().
 */
o2_message_ptr o2_finish_message(o2_time time, char *address);

/**
 * \brief free a message allocated by o2_start_send().
 *
 * This function is not normally used because O2 functions that send
 * messages take "ownership" of messages and (eventually) free them.
 */
void o2_free_message(o2_message_ptr msg);

/**
 * \brief send a message allocated by o2_start_send().
 *
 * This is similar to calling o2_send(), except you use a three-step
 * process of (1) allocate the message with o2_start_send(), (2) add
 * parameters to it using `o2_add_` functions, and (3) call
 * o2_finish_send() to send it.
 *
 * @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
int o2_finish_send(o2_time time, char *address);


/**
 * \brief send a message allocated by o2_start_send().
 *
 * This is similar to calling o2_send_cmd(), except you use a three-step
 * process of (1) allocate the message with o2_start_send(), (2) add
 * parameters to it using `o2_add_` functions, and (3) call
 * o2_finish_send_cmd() to send it.
 *
 * @return #O2_SUCCESS if success, #O2_FAIL if not.
 */
int o2_finish_send_cmd(o2_time time, char *address);

/** @} */

/**
 * \defgroup lowlevelparse Low-Level Message Parsing
 *
 * These functions can retrieve message arguments one-at-a-time.
 * There are some hidden state variables to keep track of the state
 * of unpacking, so these functions are not reentrant.
 * Arguments are returned using a pointer to a union type: #o2_arg_ptr.
 *
 */

/** \addtogroup lowlevelparse
 * @{
 */

/**
 * \brief initialize internal state to parse, extract, and coerce
 * message arguments.
 *
 * @return #O2_SUCCESS if success, #O2_FAIL if not.
 *
 * To get arguments from a message, call o2_start_extract(), then for
 * each parameter, call o2_get_next().
 */
int o2_start_extract(o2_message_ptr msg);

/**
 * \brief get the next message parameter
 *
 * This function is called repeatedly to obtain parameters in order
 * from the message passed to o2_start_extract().
 *
 * If the message parameter type matches the `type_code`, a pointer to
 * the parameter is returned. If the types do not match, but coercion
 * is possible, the parameter is coerced, copied to a new location,
 * and a pointer is returned. Otherwise, NULL is returned.
 *
 * The type of any non-NULL return value always matches the type
 * specified by the parameter `type_code`. To determine the
 * original type of the parameter as specified by the message, use the
 * `types` string which is passed to message handlers. (Or course,
 * this assumes that message type strings are correct. Badly formed
 * messages are detected when the type string and data imply that the
 * message is longer than the actual length, but otherwise there is no
 * way to detect errors in type strings.)
 *
 * The result points into the message or to a statically allocated
 * buffer if type coercion is required. This storage is only valid
 * until the next call to o2_get_next, so normally, you copy
 * the return value immediately. If the value is a pointer
 * (string, symbol, midi data, blob), then the value was
 * not copied and remains in place within the message, so there should
 * never be the need to immediately copy the data pointed to.
 * However, since the storage for the value is the message, and
 * the message will be freed when the handler returns,
 * pointers to strings, symbols, midi data, and blobs
 * *must not* be used after the handler returns.
 *
### Example 1: Simple but not completely robust

Note: call o2_add_method() with type_spec = "id", h = my_handler,
coerce = false, parse = false. In this case, since there is
no type coercion, type_spec must match the message exactly,
so o2_get_next() should always return a non-null o2_arg_ptr.
However, this code can fail if a badly formed message is sent
because there is no test for the NULL value that will be
returned by o2_get_next().
\code{.c}
    int my_handler(o2_message_ptr msg, char *types,
                   o2_arg_ptr *argv, int argc, void *user_data)
    {
        o2_start_extract(msg);
        // we expect an int32 and a double argument
        int32_t my_int = o2_get_next('i')->i32;
        double my_double = o2_get_next('d')->d;
        ...
    }
\endcode

### Example 2: Type coercion and type checking.

Note: call o2_add_method() with type_spec = NULL, h = my_handler,
coerce = false, parse = false. In this case, even though
coerce is false, there is no type_spec, so the handler will
be called without type checking. We could check the
actual message types (given by types), but here, we will
coerce into our desired types (int32 and double) if possible.
Since type coercion can fail (e.g. string will not be converted
to number, not even "123"), we need to check the return value
from o2_get_next(), where NULL indicates incompatible types.
\code{.c}
    int my_handler(o2_message_ptr msg, char *types,
                   o2_arg_ptr *argv, int argc, void *user_data)
    {
        o2_start_extract(msg);
        // we want to get an int32 and a double argument
        o2_arg_ptr ap = o2_get_next('i');
        if (!ap) return O2_FAIL; // parameter cannot be coerced
        int32_t my_int = ap->i32;
        o2_arg_ptr ap = o2_get_next('d');
        if (!ap) return O2_FAIL; // parameter cannot be coerced
        double my_double = ap->d;
        ...
    }
\endcode
 *
 * @param type_code the desired parameter type
 *
 * @return the next message parameter or NULL if no more parameters
*/
o2_arg_ptr o2_get_next(char type_code);

/** @} */


/* Scheduling */
/** \addtogroup basics
 * @{
 */

// Messages are stored in the table modulo their timestamp, so the
// table acts sort of like a hash table (this is also called the
// timing wheel structure). Messages are stored as linked lists sorted
// by increasing timestamps when there are collisions.

/** \cond INTERNAL */ \
// Size of scheduler table.
#define O2_SCHED_TABLE_LEN 128

// Scheduler data structure.
typedef struct o2_sched {
  int64_t last_bin;
  double last_time;
  o2_message_ptr table[O2_SCHED_TABLE_LEN];
} o2_sched, *o2_sched_ptr;
/** \endcond */

/**
 * \brief Scheduler that schedules according to global (master) clock
 * time
 *
 * Scheduling on this scheduler (including sending timed messages)
 * will only work after clock synchronization is obtained. Until then,
 * timed message sends will fail and attempts to o2_schedule() will
 * fail.
 */
extern o2_sched o2_gtsched;

/**
 * \brief Scheduler that schedules according to local clock time
 *
 * It may be necessary to schedule events before clock synchronization
 * with the master clock, or you may want to schedule local processing
 * that ignores any changes in clock time or clock speed needed to
 * stay synchronized with the master clock (even though these should
 * be small). For example, O2 uses the local time scheduler to
 * schedule the clock synchronization protocol, which of course must
 * run before clock synchronization is obtained.
 *
 * In these cases, you should schedule messages using #o2_ltsched.
 */
extern o2_sched o2_ltsched;

/**
 * \brief Current scheduler.
 *
 * When a timed message is delivered by a scheduler, #o2_active_sched
 * is set to pount to the scheduler. A handler that constructs and
 * schedules a message can use this pointer to continue using the same
 * scheduler.
 */
extern o2_sched_ptr o2_active_sched; // the scheduler that should be used


/**
 * /brief Schedule a message.
 *
 * Rather than sending a message, messages can be directly
 * scheduled. This is particulary useful if you want to schedule
 * activity before clock synchronization is achieved. For example, you
 * might want to poll every second waiting for clock
 * synchronization. In that case, you need to use the local scheduler
 * (#o2_ltsched). o2_send() will use the global time scheduler
 * (#o2_gtsched), so your only option is to construct a message and
 * call o2_schedule().
 *
 * @param scheduler a pointer to a scheduler (`&o2_ltsched` or
 *        `&o2_gtsched`)
 * @param msg a pointer to the message to schedule
 *
 * The message is scheduled for delivery according to its timestamp
 * (which is interpreted as local or global time depending on the
 * scheduler).
 *
 * The message is delivered immediately if the time is zero or less
 * than the current time; however, to avoid unbounded recursion,
 * messages scheduled within handlers are appended to a "pending
 * messages" queue and delivered after the handler returns.
 */
void o2_schedule(o2_sched_ptr scheduler, o2_message_ptr msg);

/** @} */ // end of a basics group


#endif /* O2_H */
