/**
 * @file
 * @brief       Nano-Kit Event Driven System (EDS) interface header
 *
 * Main API header file. This is usually the only needed header file for application to use the
 * Event Driven System. It does not include additional header files besides standard C files. All types
 * defined in this header are opaque types to objects. The application uses
 * `eds__<object>_create` functions to allocate objects.
 *
 * This header defines:
 * - Public API macros
 * - Public API integral types
 * - Public API opaque types
 * - Public API functions
 *
 * @author      Nenad Radulovic (nenad.b.radulovic@gmail.com)
 * @authors     Nenad Radulovic (nenad.b.radulovic@gmail.com)
 *
 * @defgroup    eds_intf Event Driven System (EDS)
 * @brief       Event Driven System (EDS) interface
 *
 * @{
 */

#ifndef NEON_KIT_GENERIC_SOURCE_NK_EDS_H_
#define NEON_KIT_GENERIC_SOURCE_NK_EDS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Forward declarations of types */
typedef struct eds_object__mem eds__mem;
typedef struct eds_object__evt eds__event;
typedef struct eds_object__smp eds__sm;
typedef struct eds_object__epa eds__agent;
typedef struct eds_object__etm_node eds__etimer;
typedef struct eds_object__epc eds__channel;
typedef struct eds_object__epn eds__network;

/**
 * @defgroup    eds_defaults Defaults
 * @brief       EDS Defaults interface.
 * @{
 */

/**
 * @brief       Default maximum memory entries that can be handled by EDS.
 *
 * This setting cannot be changed by global configuration (@ref eds_config).
 *
 * @see         eds_mem
 */
#define EDS__DEFAULT_MEM_ENTRIES            16u

/**
 * @brief       Default minimum bytes of a memory pool allocator.
 *
 * No memory pool allocator can be added if its allocation sizes are smaller than this macro.
 *
 * This setting cannot be changed by global configuration (@ref eds_config).
 *
 * @see         eds_mem
 */
#define EDS__DEFAULT_MEM_MIN_BYTES          16u

/**
 * @brief       Default number of events in event queue of event processing agent.
 *
 * This setting cannot be changed by global configuration (@ref eds_config), but for each created
 * event processing agent you can tweak this setting by supplying the attribute structure
 * (@ref eds__agent_attr) to @ref eds__agent_create function.
 *
 * @see         eds_agent
 */
#define EDS__DEFAULT_EPA_QUEUE_ENTRIES      16u

/**
 * @brief       Default event processing agent priority.
 *
 * This setting cannot be changed by global configuration (@ref eds_config), but for each created
 * event processing agent you can tweak this setting by supplying the attribute structure
 * (@ref eds__agent_attr) to @ref eds__agent_create function.
 *
 * @see         eds_agent
 */
#define EDS__DEFAULT_EPA_PRIO               16u

/**
 * @brief       Default event processing agent name.
 *
 * This setting cannot be changed by global configuration (@ref eds_config), but for each created
 * event processing agent you can tweak this setting by supplying the attribute structure
 * (@ref eds__agent_attr) to @ref eds__agent_create function.
 *
 * @see         eds_agent
 */
#define EDS__DEFAULT_EPA_NAME               "nameless"

/**
 * @brief       Default event processing network name.
 *
 * This setting cannot be changed by global configuration (@ref eds_config), but for each created
 * event processing agent you can tweak this setting by supplying the attribute structure
 * (@ref eds__agent_attr) to @ref eds__agent_create function.
 *
 * @see         eds_network
 */
#define EDS__DEFAULT_EPN_NAME               "nameless"

/** @} *//**
 * @defgroup    eds_version Version information
 * @brief       Version information interface.
 * @{
 */

/**
 * @brief       Version identification macro
 */
#define EDS__VERSION                        0x0300

/** @} *//**
 * @defgroup    eds_errors Error handling
 * @brief       Error handling interface.
 * @{
 */

typedef uint_fast8_t eds__error;

/**
 * @brief       No error has occurred.
 */
#define EDS__ERROR_NONE                     0

/**
 * @brief       Invalid argument is supplied to function.
 *
 * This usually means an argument pointer has a NULL value, but it might mean that pointed object
 * has failed some internal cheks.
 */
#define EDS__ERROR_INVALID_ARGUMENT         0x01

/**
 * @brief       No available memory.
 *
 * This error happens during memory allocation of an object and when memory allocator returns NULL
 * pointer.
 */
#define EDS__ERROR_NO_MEMORY                0x02
#define EDS__ERROR_NO_RESOURCE              0x03
#define EDS__ERROR_NO_PERMISSION            0x04
#define EDS__ERROR_ALREADY_EXISTS           0x05
#define EDS__ERROR_INVALID_CONFIGURATION    0x06
#define EDS__ERROR_NOT_EXISTS               0x07
#define EDS__ERROR_MALFORMED_SM             0x08
#define EDS__ERROR_NO_SPACE                 0x09
#define EDS__ERROR_OUT_OF_RANGE             0x0a
#define EDS__ERROR_SM_BAD_ENTER             0x0b
#define EDS__ERROR_SM_BAD_EXIT              0x0c
#define EDS__ERROR_SM_BAD_INIT              0x0d
#define EDS__ERROR_SM_BAD_SUPER             0x0e

const char * eds__error_to_str(uint32_t error);

/** @} *//**
 * @defgroup    eds_mem Memory allocator
 * @brief       Memory allocator interface.
 *
 * Neon-Kit EDS does not support standard C use of heap memory allocation (using malloc and free)
 * since many embedded projects and system impose a lot of restrictions on memory allocation
 * methods. In order to support dynamic nature of events, Neon-Kit EDS still needs to have some
 * support for dynamic memory allocation. Since the memory allocation restrictions change from
 * application to application (or project to project) it was decided that memory allocation problem
 * is pushed to the application writer.
 *
 * The application writer best knows what are circumstances where the application is going to be
 * executed and he has the best view in order to decide on allocation algorithm. Whatever the
 * application writer decides to use (memory pools or heap) all it needs to do is to tell to
 * Neon-Kit which allocator to use, using @ref eds__mem_add_allocator function.
 *
 * @{
 */

/**
 * @brief       Allocator function type.
 *
 * This type defines pointer to function which receives the following arguments:
 * - pointer to a context (which was supplied to @ref eds__mem_add_allocator function)
 * - size of memory block which needs to be allocated.
 *
 * The method returns pointer to allocated memory section. If NULL pointer is returned then the
 * allocation memory has been depleted.
 */
typedef void*
(eds__mem_alloc_fn)(void*, size_t);

/**
 * @brief       Deallocator function type.
 *
 * Deallocator method receives two arguments:
 * - pointer to a context (which was supplied to @ref eds__mem_add_allocator function)
 * - pointer to a previously allocated memory by @ref eds__mem_alloc_fn function.
 */
typedef void
(eds__mem_dealloc_fn)(void*, void*);

/**
 * @brief       Add memory allocator for EDS objects creation.
 *
 * This functions needs to be called before any other function from EDS package. The function will
 * store the information about available allocators. Allocators are used by create functions of EDS
 * when creating new objects. All objects used by EDS may be allocated dynamically or statically.
 * In order to create a static object use attribute structure `instance` pointer.
 *
 * If no allocators are provided EDS will not be able to create new objects and errors will be
 * returned.
 *
 * Specify for each allocator malloc/free pair how big memory partitions it can handle. This adds
 * ability to use multiple memory pools of different sizes.
 *
 * EDS can accept up to @ref EDS__DEFAULT_MEM_ENTRIES different allocators. It was shown in
 * practice that around 7 pool allocators of different sizes are needed for a typical application.
 *
 * @param       alloc is a pointer to function which allocates memory space. Allocator function
 *              receives the following arguments: first is the pointer to context structure, which
 *              was provided by @a context argument of this function; the second second argument
 *              tells to allocator function how big the memory block should be allocated. The size
 *              of this argument can be anything between zero and @a max_size argument of this
 *              function. The allocator function returns `void` pointer to allocated memory. If it
 *              can not allocate memory it returns NULL pointer. Allocator function does not need
 *              handle zero allocations (size of allocation is equal to zero).
 * @param       dealloc is a pointer to function which receives the context structure and the
 *              previously allocated block and it should recycle its space. This function does not
 *              need to handle NULL deallocations.
 * @param       context is pointer to allocator context structure if it needed by allocator. The
 *              structure needs to exist the whole time while allocator functions are being used.
 * @param       max_size is maximum size of block that can be allocated and freed by allocator. In
 *              case when a pool memory is used this argument will be equal to pool  memory block
 *              size. When this argument is set to zero or SIZE_MAX value then this allocator would
 *              be used in case no other allocator satisfies required size.
 *
 * @return      Operation status.
 * @retval      EDS__ERROR_NONE (@ref EDS__ERROR_NONE) Operation completed successfully.
 * @retval      EDS__ERROR_INVALID_ARGUMENT (@ref EDS__ERROR_INVALID_ARGUMENT) Is returned when
 *              either @a alloc or @a dealloc arguments are NULL pointer or when @a max_size is less
 *              than @ref EDS__DEFAULT_MEM_MIN_BYTES bytes.
 * @retval      EDS__ERROR_NO_RESOURCE (@ref EDS__ERROR_NO_RESOURCE) Is returned when application
 *              tried to add more than @ref EDS__DEFAULT_MEM_ENTRIES instances of memory allocator.
 * @retval      EDS__ERROR_ALREADY_EXISTS (@ref EDS__ERROR_ALREADY_EXISTS) The allocator with
 *              @a max_size memory block was already added.
 */
eds__error
eds__mem_add_allocator(eds__mem_alloc_fn *alloc,
    eds__mem_dealloc_fn *dealloc,
    void *context,
    size_t max_size);

/** @} *//**
 * @defgroup    event Events
 * @brief       Events interface.
 * @{
 */

/**
 * @name       Predefined event identifiers
 */
#define EDS__EVENT__NULL                    0
#define EDS__EVENT__USER                    64

/** @} *//**
 * @defgroup    eds_event Event handling
 *
 * @{
 */

struct eds__event_attr
{
    eds__event *static_instance;
};

/**
 * @brief       Create an event and initialize it.
 *
 * Before an event can be generated it needs to be created. This function creates a dynamic event.
 * Dynamic event are events that are allocated using a memory allocator.
 *
 * @param       event_id Event identification number. This is a unique number which identifies an
 *              event. Event identifiers are usually handled by enumerations. It is application
 *              responsibility to allocate unique numbers for each event.
 * @param       event_data_size Some events might have some attached data. This argument specifies
 *              the size of attached data in bytes. When event does not have any data put this
 *              argument to zero.
 * @param [out] event Pointer to `eds_event *` variable. This variable will hold the pointer to
 *              newly created event. The application might want to attach data to this event (see
 *              @ref eds_event__data function).
 * @return      Operation status.
 * @retval      EDS__ERROR_NONE Operation completed successfully.
 * @retval      EDS__ERROR_INVALID_ARGUMENT Is returned when either @a event_id is equal to zero or
 *              when @a event pointer is NULL pointer.
 * @retval      EDS__ERROR_NO_RESOURCE When no suitable allocator was found and EDS is then unable
 *              to allocate memory space for the event. To add allocators use
 *              @ref eds__add_allocator function.
 * @retval      EDS__ERROR_NO_MEMORY When a suitable allocator was found but its memory reserves
 *              have been depleted.
 */
eds__error
eds__event_create(uint32_t event_id, size_t event_data_size, eds__event **event);

eds__error
eds__event_cancel(eds__event *event);

eds__error
eds__event_keep(const eds__event *event);

eds__error
eds__event_toss(const eds__event *event);

/**
 * @brief       Initialize a static event.
 *
 * Static event is an event which always exists. Once processed it can not be deleted. Using static
 * events is an alternative when embedded system does not allow any memory allocation.
 *
 * @param       event Is a pointer to statically allocated event structure. The definition of this
 *              structure is available in `eds_object.h` header file.
 * @param       event_id Event identification number. This is a unique number which identifies an
 *              event. Event identifiers are usually handled by enumerations. It is application
 *              responsibility to allocate unique numbers for each event.
 * @param       event_data_size Some events might have some attached data. This argument specifies
 *              the size of attached data in bytes. When event does not have any data put this
 *              argument to zero.
 * @return      Operation status.
 * @retval      EDS__ERRROR_NONE Operation completed successfully.
 * @retval      EDS__ERROR_INVLD_ARGUMENT Is returned when either @a event_id is equal to zero or
 *              when @a event pointer is NULL pointer.
 */
eds__error
eds__event_init(eds__event *event, uint32_t event_id, size_t event_data_size);


void *
eds__event_put_data(eds__event * event);

/**
 * @brief       Get event identification number from the event.
 *
 * @pre         Argument @a event must be a non-NULL pointer and pointing to a previously initialized
 *              or created event.
 * @param       event Pointer to event.
 * @return      Event identification number (uint32_t).
 */
uint32_t
eds__event_id(const eds__event *event);

/**
 * @brief       Get attached data from the event.
 *
 * @pre         Argument @a event must be a non-NULL pointer and pointing to a previously initialized
 *              or created event.
 * @param       event Pointer to event.
 * @return      Pointer to event data.
 * @retval      NULL The event has no attached data.
 */
const void*
eds__event_data(const eds__event *event);

/**
 * @brief       Get event attached data size from the event.
 *
 * @pre         Argument @a event must be a non-NULL pointer and pointing to a previously initialized
 *              or created event.
 * @param       event Pointer to event.
 * @return      Size of event attached data in bytes.
 * @retval      0 The event has no attached data.
 */
size_t
eds__event_size(const eds__event *event);

/** @} *//**
 * @defgroup    sm State machine
 *
 * @{
 */

#define EDS__SM_EVENT__INIT                 1
#define EDS__SM_EVENT__ENTER                2
#define EDS__SM_EVENT__EXIT                 3
#define EDS__SM_EVENT__SUPER                4

typedef uint_fast8_t eds__sm_action;

typedef eds__sm_action
(eds__sm_state)(eds__sm*, void *workspace, const eds__event *event);

eds__sm_action
eds__sm_event_handled(eds__sm *sm);

eds__sm_action
eds__sm_event_ignored(eds__sm *sm);

eds__sm_action
eds__sm_transit_to(eds__sm *sm, eds__sm_state *new_state);

eds__sm_action
eds__sm_super_state(eds__sm *sm, eds__sm_state *super_state);

eds__sm_action
eds__sm_top_state(eds__sm *sm, void *workspace, const eds__event *event);

/** @} *//**
 * @defgroup    eds_agent Event Processing Agent (EPA)
 *
 * @{
 */

typedef enum eds_object__epa_prio
{
    EDS__EPA_PRIO__IDLE = 0,            ///< Reserved for Idle EPA.
    EDS__EPA_PRIO__LOW = 1,            ///< Priority: low
    EDS__EPA_PRIO__LOW_1 = 1 + 1,        ///< Priority: low + 1
    EDS__EPA_PRIO__LOW_2 = 1 + 2,        ///< Priority: low + 2
    EDS__EPA_PRIO__LOW_3 = 1 + 3,        ///< Priority: low + 3
    EDS__EPA_PRIO__LOW_4 = 1 + 4,        ///< Priority: low + 4
    EDS__EPA_PRIO__BELOW_NORMAL = 6,            ///< Priority: below normal
    EDS__EPA_PRIO__BELOW_NORMAL_1 = 6 + 1,        ///< Priority: below normal + 1
    EDS__EPA_PRIO__BELOW_NORMAL_2 = 6 + 2,        ///< Priority: below normal + 2
    EDS__EPA_PRIO__BELOW_NORMAL_3 = 6 + 3,        ///< Priority: below normal + 3
    EDS__EPA_PRIO__BELOW_NORMAL_4 = 6 + 4,        ///< Priority: below normal + 4
    EDS__EPA_PRIO__NORMAL = 11,           ///< Priority: normal
    EDS__EPA_PRIO__NORMAL_1 = 11 + 1,       ///< Priority: normal + 1
    EDS__EPA_PRIO__NORMAL_2 = 11 + 2,       ///< Priority: normal + 2
    EDS__EPA_PRIO__NORMAL_3 = 11 + 3,       ///< Priority: normal + 3
    EDS__EPA_PRIO__NORMAL_4 = 11 + 4,       ///< Priority: normal + 4
    EDS__EPA_PRIO__NORMAL_5 = 11 + 5,       ///< Priority: normal + 5
    EDS__EPA_PRIO__ABOVE_NORMAL = 17,           ///< Priority: above normal
    EDS__EPA_PRIO__ABOVE_NORMAL_1 = 17 + 1,       ///< Priority: above normal + 1
    EDS__EPA_PRIO__ABOVE_NORMAL_2 = 17 + 2,       ///< Priority: above normal + 2
    EDS__EPA_PRIO__ABOVE_NORMAL_3 = 17 + 3,       ///< Priority: above normal + 3
    EDS__EPA_PRIO__ABOVE_NORMAL_4 = 17 + 4,       ///< Priority: above normal + 4
    EDS__EPA_PRIO__HIGH = 22,           ///< Priority: high
    EDS__EPA_PRIO__HIGH_1 = 22 + 1,       ///< Priority: high + 1
    EDS__EPA_PRIO__HIGH_2 = 22 + 2,       ///< Priority: high + 2
    EDS__EPA_PRIO__HIGH_3 = 22 + 3,       ///< Priority: high + 3
    EDS__EPA_PRIO__HIGH_4 = 22 + 4,       ///< Priority: high + 4
    EDS__EPA_PRIO__REALTIME = 27,           ///< Priority: realtime
    EDS__EPA_PRIO__REALTIME_1 = 27 + 1,       ///< Priority: realtime + 1
    EDS__EPA_PRIO__REALTIME_2 = 27 + 2,       ///< Priority: realtime + 2
    EDS__EPA_PRIO__REALTIME_3 = 27 + 3,       ///< Priority: realtime + 3
    EDS__EPA_PRIO__REALTIME_4 = 27 + 4,       ///< Priority: realtime + 4
} eds__epa_prio;

/**
 * @brief       Agent attribute structure
 *
 * Attribute structure describes in greater details an agent. When no attribute is being used,
 * agents are created using default values specified in @ref defaults. Use this structure to
 * override and customize an agent instance.
 */
struct eds__agent_attr
{
    const char *name;
    eds__epa_prio prio;
    uint32_t equeue_entries;
    eds__agent *static_instance;
    eds__event **static_equeue_storage;
};

/**
 * @brief       Create and initialize an agent (Event Processing Agent).
 *
 * @param       sm_initial_state Pointer to initial state function of state machine.
 * @param       sm_workspace Pointer to an allocated memory space reserved for state machine
 *              operation. Use this argument to pass pointer to structure instances which then can
 *              be used by state machine code.
 * @param       attr Use @a attr pointer to pass attribute structure. Use attribute structure to
 *              customize agent instance. Pass NULL pointer to use defaults specified in
 *              @ref defaults.
 * @param [out] agent Pointer to pointer to agent. The pointer to agent will be filled after
 *              successful creation of the agent.
 * @return      Operation status.
 * @retval      EDS__ERROR_NONE Operation completed successfully.
 * @retval      EDS__ERROR_INVALID_ARGUMENT Is returned when @a sm_initial_state or @a agent pointer
 *              is NULL pointer.
 * @retval      EDS__ERROR_INVALID_CONFIGURATION Is returned when:
 *              1. The member `equeue_entries` in @a attr attribute structure is zero.
 *              2. Not both members `static_instance` and `static_equeue_storage` are set to
 *              non-NULL value. See the valid case examples in @ref eds__agent_attr description.
 * @retval      EDS__ERROR_NO_RESOURCE When no suitable allocator was found and EDS is then unable
 *              to allocate memory space for the agent. To add allocators use
 *              @ref eds__add_allocator function.
 * @retval      EDS__ERROR_NO_MEMORY When a suitable allocator was found but its memory reserves
 *              have been depleted.
 */
eds__error
eds__agent_create(eds__sm_state *sm_initial_state,
    void *sm_workspace,
    const struct eds__agent_attr *attr,
    eds__agent **agent);

eds__error
eds__agent_delete(eds__agent *agent);

eds__error
eds__agent_send(eds__agent *agent, const eds__event *event);

eds__agent*
eds__agent_from_sm(eds__sm *sm);

/**
 * @brief       Get the network this agent belongs to
 *
 * @param       agent Pointer to agent
 * @return      Pointer to network this agent belongs to.
 * @retval      NULL this agent does not belong to any network (undesignated agent) or @a agent
 *              pointer is NULL pointer.
 */
eds__network*
eds__agent_network(const eds__agent *agent);

/**
 * @brief       Get the workspace of this agent
 *
 * @param       agent Pointer to agent
 * @return      void* Pointer to agent workspace which was passed to @ref eds__agent_create function
 *              during the creation of the agent.
 */
void*
eds__agent_workspace(const eds__agent *agent);

/** @} */
#define EDS__ETIMER_FLAG__UP_TO             0x1
#define EDS__ETIMER_FLAG__AT_LEAST          0x2

struct eds__etimer_attr
{
    eds__etimer *static_instance;
    uint32_t flags;
};

eds__error
eds__etimer_create(const struct eds__etimer_attr *attr, eds__etimer **etimer);

eds__error
eds__etimer_delete(eds__etimer *etimer);

eds__error
eds__etimer_send_after(eds__etimer *etimer,
    eds__agent *agent,
    const eds__event *event,
    uint32_t after_ms);

eds__error
eds__etimer_send_every(eds__etimer *etimer,
    eds__agent *agent,
    const eds__event *event,
    uint32_t every_ms);

eds__error
eds__etimer_cancel(eds__etimer *etimer);

struct eds__epn_attr
{
    const char *name;
    eds__network *instance;
};

eds__error
eds__network_create(const struct eds__epn_attr *atrr, eds__network **epn);

eds__error
eds__epn_delete(eds__network *epn);

eds__error
eds__network_add_agent(eds__network *epn, eds__agent *sm);

/**
 * @brief       Remove an agent from the network
 *
 * @param       network Pointer to event processing network.
 * @param       agent Pointer to agent instance which is to be removed from the network.
 * @return      Operation status.
 * @retval      EDS__ERROR_NONE Operation completed successfully.
 * @retval      EDS__ERROR_INVLD_ARGUMENT Is returned when @a network or @a agent pointer is NULL
 *              pointer.
 * @retval      EDS__ERROR_NOT_EXISTS when the agent pointed by @a agent is not belonging to any
 *              network.
 */
eds__error
eds__network_remove_agent(eds__network *network, eds__agent *agent);

eds__error
eds__network_start(eds__network *epn);

eds__error
eds__epn_stop(eds__network *epn);

eds__error
eds__tick_process_all(void);

#endif /* NEON_KIT_GENERIC_SOURCE_NK_EDS_H_ */
/** @} */
