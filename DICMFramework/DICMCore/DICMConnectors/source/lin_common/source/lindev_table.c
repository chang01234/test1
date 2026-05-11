/*! \file lindev_table.h
    \brief LINDEV Table is a component of LINDEV stack for mapping of data and describing LIN frames
    \author Nenad Radulovic
 */
#include "lindev_table.h"

#include <string.h>

#if (LINDEV_TABLE_VERBOSE_LOG == 1)
#define LINDEV_TABLE_LOG(level, format, ...)		LOG(level, format, ## __VA_ARGS__)
#else
#define LINDEV_TABLE_LOG(level, format, ...)
#endif

/**
 * @brief       Synhronization state machine events
 *
 * For details about synchronization state machine refer to documentation folder
 */
typedef enum sync_sm_event
{
    SYNC_SM_EVENT_CTRL,                 //!< CTRL frame was received
    SYNC_SM_EVENT_ERR,                  //!< Error while sending INFO frame
    SYNC_SM_EVENT_LC,                   //!< Local change has occured
    SYNC_SM_EVENT_SLEEP,                //!< A LIN sleep event (either a master command or bus inactivity) has occurred
} sync_sm_event_t;

/**
 * @brief       Reset LocalChange bit in INFO frame bundle
 *
 * @param       info_bundle Pointer to a bundle which describes an INFO frame
 *
 * If INFO frame has no synchronization bits defined then this function is NOP function.
 */
static void info_bundle_rst_local_change(const lindev_table_info_bundle_t * info_bundle)
{
    if (info_bundle->protocol->has_local_change)
    {
        *((uint8_t *)info_bundle->buffer + info_bundle->protocol->local_change_offset) = 0;
    }
}

/**
 * @brief       Set LocalChange bit in INFO frame bundle
 *
 * @param       info_bundle Pointer to a bundle which describes an INFO frame
 *
 * If INFO frame has no synchronization bits defined then this function is NOP function.
 */
static void info_bundle_set_local_change(const lindev_table_info_bundle_t * info_bundle)
{
    if (info_bundle->protocol->has_local_change)
    {
        *((uint8_t *)info_bundle->buffer + info_bundle->protocol->local_change_offset) = 1;
    }
}

/**
 * @brief       Set new page id in INFO frame bundle
 *
 * @param       info_bundle Pointer to a bundle which describes an INFO frame
 *
 * If INFO frame has no paging bits defined then this function is NOP function.
 */
static void info_bundle_set_page_id(const lindev_table_info_bundle_t * info_bundle, uint32_t page)
{
    if ((info_bundle->protocol != NULL) && (info_bundle->protocol->page_num != 0))
    {
        *((uint8_t *)info_bundle->buffer + info_bundle->protocol->page_id_offset) = (uint8_t)page;
    }
}

/**
 * @brief       Get current page id from INFO frame bundle
 *
 * @param       info_bundle Pointer to a bundle which describes an INFO frame
 *
 * If INFO frame has no paging bits defined then this function always returns zero, corresponding to
 * simple synchronzation method.
 */
static uint32_t info_bundle_get_page_id(const lindev_table_info_bundle_t * info_bundle)
{
    if ((info_bundle->protocol != NULL) && (info_bundle->protocol->page_num != 0))
    {
        return *((uint8_t *)info_bundle->buffer + info_bundle->protocol->page_id_offset);
    }
    else
    {
        return 0;
    }
}

static const lindev_table_info_bundle_t * info_bundle_get_by_initial_frame_id(
    const lindev_table_config_t * lindev_table,
    uint32_t initial_frame_id)
{
    const lindev_table_info_bundle_t * info_bundle;

    info_bundle = NULL;
    /* Find info bundle associated with initial_frame_id */
    for (size_t bundle_index = 0u; bundle_index < lindev_table->bundle_map_length; bundle_index++)
    {
        if (lindev_table->bundle_map[bundle_index].info == NULL)
        {
            continue;
        }
        if (lindev_table->bundle_map[bundle_index].info->frame_id == initial_frame_id)
        {
            info_bundle = lindev_table->bundle_map[bundle_index].info;
            break;
        }
    }
    return info_bundle;
}


/**
 * @brief       Get current CTRL page id from CTRL frame bundle
 *
 * @param       info_bundle Pointer to a bundle which describes an INFO frame
 *
 * If CTRL frame has no paging bits defined then this function always returns zero, corresponding to
 * simple synchronzation method.
 */
static size_t ctrl_bundle_get_ctrl_page_id(const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    if ((ctrl_bundle->protocol != NULL) && (ctrl_bundle->protocol->has_ctrl_and_info_page_id))
    {
        return *(uint8_t *)((uint8_t *)ctrl_bundle->buffer + ctrl_bundle->protocol->ctrl_page_id_offset);
    }
    else
    {
        return 0;
    }
}

/**
 * @brief       Get next INFO page id from CTRL frame bundle
 *
 * @param       info_bundle Pointer to a bundle which describes an INFO frame
 *
 * If CTRL frame has no paging bits defined then this function always returns zero, corresponding to
 * simple synchronzation method.
 */
static size_t ctrl_bundle_get_info_page_id(const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    if (ctrl_bundle->protocol->has_ctrl_and_info_page_id)
    {
        return *(uint8_t *)((uint8_t *)ctrl_bundle->buffer + ctrl_bundle->protocol->info_page_id_offset);
    }
    else
    {
        return 0;
    }
}

/**
 * @brief       Get the index in bundle_map from given INFO frame bundle
 *
 * @param       lindev_table Pointer to lindev_table description structure
 * @param       info_bundle Pointer of a INFO frame bundle
 * @return      size_t Index of INFO frame bundle in bundle_map table.
 * @retval      SIZE_MAX when INFO frame bundle can not be find in the bundle_map table. This is a
 *              mapping error.
 */
static size_t map_info_bundle_to_bundle_index(
    const lindev_table_config_t * lindev_table,
    const lindev_table_info_bundle_t * info_bundle)
{
    for (size_t bundle_index = 0u; bundle_index < lindev_table->bundle_map_length; bundle_index++)
    {
        if (info_bundle == lindev_table->bundle_map[bundle_index].info)
        {
            return bundle_index;
        }
    }
    return SIZE_MAX;
}

/**
 * @brief       See if `remote_data` in LIN INFO and LIN CTRL frames are equal
 *
 * @param       info_bundle Pointer to LINDEV Table INFO bundle
 * @param       ctrl_bundle Pointer to LINDEV Table CTRL bundle
 * @param       page_index Index of page in frame
 * @return      A boolean stating if remote_data members are equal or not
 * @retval      true The `remote_data` members are equal
 * @retval      false The `remote_data` members are not equal
 */
static bool sync_sm_bundle_remote_data_is_equal(
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle,
    size_t page_index)
{
    const void * lhs_remote_data;
    const void * rhs_remote_data;

    if (info_bundle->remote_data_size[page_index] == 0)
    {
        /* This happens when we have pages that do not share common remote data fields. In these
         * cases we can only assume that data was correctly received by Master and we return true;
         */
        LINDEV_TABLE_LOG(I, "No remote data segments are defined for page %u, returning TRUE", page_index);
        return true;
    }
    if (info_bundle->remote_data_size[page_index] != ctrl_bundle->remote_data_size[page_index])
    {
        LINDEV_TABLE_LOG(E,
            "INFO and CTRL bundle remote data sizes are different for page %u, INFO: %u bytes, CTRL: %u bytes",
            page_index,
            info_bundle->remote_data_size[page_index],
            ctrl_bundle->remote_data_size[page_index]);
        return false;
    }
    lhs_remote_data = (uint8_t *)info_bundle->buffer + info_bundle->remote_data_offset[page_index];
    rhs_remote_data = (uint8_t *)ctrl_bundle->buffer + ctrl_bundle->remote_data_offset[page_index];
    LINDEV_TABLE_LOG(I, "Evaluating remote data segments for page %u", page_index);
    if (memcmp(lhs_remote_data, rhs_remote_data, info_bundle->remote_data_size[page_index]) != 0)
    {
#if (LINDEV_TABLE_VERBOSE_LOG == 1)
        char buffer[256];
        int index = 0;

        for (size_t i = 0u; i < info_bundle->remote_data_size[page_index]; i++)
        {
            index += snprintf(&buffer[index], sizeof(buffer) - index, "%02x ", ((const uint8_t *)lhs_remote_data)[i]);
        }
        buffer[index] = '\0';
        LINDEV_TABLE_LOG(W, "INFO (page %u) data: %s", page_index, buffer);

        index = 0;
        for (size_t i = 0u; i < info_bundle->remote_data_size[page_index]; i++)
        {
            index += snprintf(&buffer[index], sizeof(buffer) - index, "%02x ", ((const uint8_t *)rhs_remote_data)[i]);
        }
        buffer[index] = '\0';
        LINDEV_TABLE_LOG(W, "CTRL (page %u) data: %s", page_index, buffer);
#endif /* (LINDEV_TABLE_VERBOSE_LOG == 1) */
        return false;
    }
    else
    {
        LINDEV_TABLE_LOG(I, "remote data segments are equal");
        return true;
    }
}

/**
 * @brief       Synchronization SM helper function, returns if CTRL frame has a paired INFO frame
 *              and are correctly mapped.
 *
 * @param       info_bundle Pointer to INFO frame bundle
 * @param       ctrl_bundle Pointer to CTRL frame bundle
 * @return      true Frames are matched and properly mapped
 * @return      false Frames are not matched or NOT properly mapped
 */
static bool sync_sm_is_matched(
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    if ((info_bundle == NULL) || (ctrl_bundle == NULL))
    {
        LINDEV_TABLE_LOG(I, "INFO or CTRL frame is missing, no synchronization available.");
        return false;
    }
    if ((info_bundle->protocol == NULL) || (ctrl_bundle->protocol == NULL))
    {
        return false;
    }
    if ((info_bundle->protocol->has_local_change == false) ||
        (ctrl_bundle->protocol->has_sync_frame == false))
    {
        LINDEV_TABLE_LOG(E, "INFO or CTRL frames are matched, but protocol is missing, no synchronization available.");
        return false;
    }
    return true;
}

/**
 * @brief       Returns if CTRL frame bundle has SyncFrame bit cleared
 *
 * @param       ctrl_bundle Pointer to CTRL frame bundle
 * @return      Is this a command frame
 * @retval      true - This CTRL frame is a command frame (SyncFrame is cleared)
 * @retval      false - This CTRL frame is a synchronization frame (SyncFrame is set)
 */
static bool sync_sm_is_ctrl_cmd(const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    return *((uint8_t *)ctrl_bundle->buffer + ctrl_bundle->protocol->sync_frame_offset) == 0u;
}

/**
 * @brief       Returns if CTRL frame bundle has SyncFrame bit set
 *
 * @param       ctrl_bundle Pointer to CTRL frame bundle
 * @return      Is this a synchronization frame
 * @retval      true - This CTRL frame is a synchronization frame (SyncFrame is set)
 * @retval      false - This CTRL frame is a command frame (SyncFrame is cleared)
 */
static bool sync_sm_is_ctrl_sync(const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    return !sync_sm_is_ctrl_cmd(ctrl_bundle);
}

/**
 * @brief       Initialize synchronization SM
 *
 * @param       sm_state Pointer to state variable
 * @param       info_bundle Pointer to INFO frame bundle which is handled by this SM
 * @param       ctrl_bundle Pointer to CTRL frame bundle which is handled by this SM
 *
 * This function initializes SM and evaluates given frames.
 */
static void sync_sm_init(lindev_table_sync_sm_state_t * sm_state,
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    if (sync_sm_is_matched(info_bundle, ctrl_bundle))
    {
        *sm_state = LINDEV_TABLE_SYNC_SM_STATE_SYNCHRONIZED;
        /* Initial INFO frame condition */
        info_bundle_rst_local_change(info_bundle);
    }
    else
    {
        sm_state = LINDEV_TABLE_SYNC_SM_STATE_NO_SYNC;
    }
}

/**
 * @brief       This is a permanent state of synchronization SM when given INFO & CTRL frames are
 *              not matched.
 *
 * @param       sm_state Pointer to state variable
 * @param       sm_event Event, @see sync_sm_event_t
 * @param       info_bundle Pointer to INFO frame bundle which is handled by this SM
 * @param       ctrl_bundle Pointer to CTRL frame bundle which is handled by this SM
 * @return      lindev_table_status_t Synchronization status
 */
static lindev_table_status_t sync_sm_state_no_sync(
    lindev_table_sync_sm_state_t * sm_state,
    sync_sm_event_t sm_event,
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    return LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE;
}

/**
 * @brief       Synchronization SM state when given INFO & CTRL frames are synchronized.
 *
 * @param       sm_state Pointer to state variable
 * @param       sm_event Event, @see sync_sm_event_t
 * @param       info_bundle Pointer to INFO frame bundle which is handled by this SM
 * @param       ctrl_bundle Pointer to CTRL frame bundle which is handled by this SM
 * @return      lindev_table_status_t Synchronization status
 */
static lindev_table_status_t sync_sm_state_synchronized(
    lindev_table_sync_sm_state_t * sm_state,
    sync_sm_event_t sm_event,
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    switch (sm_event)
    {
    case SYNC_SM_EVENT_CTRL:
        /* No state change */
        /* NOTE:
         * Protocol description states that Master must clear SyncFrame bit in CTRL frame when
         * sending commands. The bit SyncFrame must be set only when a preceeding LocalChange in
         * INFO frame was set.
         */
        if (sync_sm_is_ctrl_cmd(ctrl_bundle))
        {
            return LINDEV_TABLE_STATUS_SYNC_NOT_PENDING;
        }
        else
        {
            return LINDEV_TABLE_STATUS_SYNC_PENDING;
        }
    case SYNC_SM_EVENT_ERR:
        /* No state change */
        return LINDEV_TABLE_STATUS_SYNC_NOT_PENDING;
    case SYNC_SM_EVENT_LC:
        *sm_state = LINDEV_TABLE_SYNC_SM_STATE_NOTIFYING;
        return LINDEV_TABLE_STATUS_SYNC_PENDING;
    case SYNC_SM_EVENT_SLEEP:
        /* No state change */
        return LINDEV_TABLE_STATUS_SYNC_NOT_PENDING;
    default:
        return LINDEV_TABLE_STATUS_SYNC_NOT_PENDING;
    }
}

/**
 * @brief       Synchronization SM state when given INFO & CTRL frames are NOT synchronized.
 *
 * @param       sm_state Pointer to state variable
 * @param       sm_event Event, @see sync_sm_event_t
 * @param       info_bundle Pointer to INFO frame bundle which is handled by this SM
 * @param       ctrl_bundle Pointer to CTRL frame bundle which is handled by this SM
 * @return      lindev_table_status_t Synchronization status
 */
static lindev_table_status_t sync_sm_state_notifying(
    lindev_table_sync_sm_state_t * sm_state,
    sync_sm_event_t sm_event,
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    switch (sm_event)
    {
    case SYNC_SM_EVENT_CTRL:
        /* No state change */
        if (sync_sm_is_ctrl_sync(ctrl_bundle))
        {
            size_t current_page_id = ctrl_bundle_get_ctrl_page_id(ctrl_bundle);
            LINDEV_TABLE_LOG(I, "Received synchronization frame ID [%02x] [CTRL page %u]", ctrl_bundle->frame_id, current_page_id);
            if (current_page_id >= LINDEV_TABLE_MAX_PAGES_PER_FRAME)
            {
                return LINDEV_TABLE_STATUS_ERROR;
            }
            if (sync_sm_bundle_remote_data_is_equal(info_bundle, ctrl_bundle, current_page_id))
            {
                *sm_state = LINDEV_TABLE_SYNC_SM_STATE_SYNCHRONIZED;
                return LINDEV_TABLE_STATUS_SYNC_COMPLETED;
            }
        }
        return LINDEV_TABLE_STATUS_SYNC_PENDING;
    case SYNC_SM_EVENT_ERR:
        /* No state change */
        return LINDEV_TABLE_STATUS_SYNC_PENDING;
    case SYNC_SM_EVENT_LC:
        /* No state change */
        return LINDEV_TABLE_STATUS_SYNC_PENDING;
    case SYNC_SM_EVENT_SLEEP:
        *sm_state = LINDEV_TABLE_SYNC_SM_STATE_SYNCHRONIZED;
        return LINDEV_TABLE_STATUS_SYNC_NOT_PENDING;
    default:
        return LINDEV_TABLE_STATUS_SYNC_PENDING;
    }
}

/**
 * @brief       Dispatch an event to synchronization SM
 *
 * @param       sm_state Pointer to state variable
 * @param       sm_event Event, @see sync_sm_event_t
 * @param       info_bundle Pointer to INFO frame bundle which is handled by this SM
 * @param       ctrl_bundle Pointer to CTRL frame bundle which is handled by this SM
 * @return      lindev_table_status_t Synchronization status
 */
static lindev_table_status_t sync_sm_dispatch(
    lindev_table_sync_sm_state_t * sm_state,
    sync_sm_event_t sm_event,
    const lindev_table_info_bundle_t * info_bundle,
    const lindev_table_ctrl_bundle_t * ctrl_bundle)
{
    switch (*sm_state)
    {
    case LINDEV_TABLE_SYNC_SM_STATE_NO_SYNC:
        return sync_sm_state_no_sync(sm_state, sm_event, info_bundle, ctrl_bundle);
    case LINDEV_TABLE_SYNC_SM_STATE_SYNCHRONIZED:
        return sync_sm_state_synchronized(sm_state, sm_event, info_bundle, ctrl_bundle);
    case LINDEV_TABLE_SYNC_SM_STATE_NOTIFYING:
        return sync_sm_state_notifying(sm_state, sm_event, info_bundle, ctrl_bundle);
    default:
        return LINDEV_TABLE_STATUS_ERROR;
    }
}

/**
 * @brief       Returns if synchronization SM is in notifying state
 *
 * This function is used to evaluate current SM state and see if it is in notifying state. This is
 * particularly significant when deciding whether to set LocalChange bit in an INFO frame bundle.
 *
 * @param       sm_state Pointer to state variable
 * @return      Is synchronization SM in notifying state
 * @retval      true SM is in notifying state
 * @retval      false SM is NOT in notifying state
 */
static bool sync_sm_is_in_notifying_state(const lindev_table_sync_sm_state_t * sm_state)
{
    return *sm_state == LINDEV_TABLE_SYNC_SM_STATE_NOTIFYING;
}

lindev_table_status_t lindev_table_initialize_table_state(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state)
{
    if (lindev_table->bundle_map_length > LINDEV_TABLE_MAX_BUNDLE_MAP_ENTRIES)
    {
        return LINDEV_TABLE_STATUS_ERROR;
    }
    for (size_t bundle_index = 0u; bundle_index < lindev_table->bundle_map_length; bundle_index++)
    {
        for (size_t page_index = 0u; page_index < LINDEV_TABLE_MAX_PAGES_PER_FRAME; page_index++)
        {
            sync_sm_init(
                &lindev_table_state->sync_state[bundle_index][page_index],
                lindev_table->bundle_map[bundle_index].info,
                lindev_table->bundle_map[bundle_index].ctrl);
        }
    }
    return LINDEV_TABLE_STATUS_OK;
}

const lindev_table_ctrl_bundle_t * lindev_table_ctrl_extract_to_dometic(
    const lindev_table_config_t * lindev_table,
    const uint8_t * frame_data,
    uint_fast8_t initial_frame_id)
{
    /* Got through bundle map, find a ctrl_bundle that handles this frame */
    const lindev_table_ctrl_bundle_t * ctrl_bundle;

    ctrl_bundle = NULL;
    for (size_t bundle_index = 0u; bundle_index < lindev_table->bundle_map_length; bundle_index++)
    {
        if (lindev_table->bundle_map[bundle_index].ctrl == NULL)
        {
            continue;
        }

        if (lindev_table->bundle_map[bundle_index].ctrl->frame_id == initial_frame_id)
        {
            ctrl_bundle = lindev_table->bundle_map[bundle_index].ctrl;
            break;
        }
    }
    if (ctrl_bundle == NULL)
    {
        /* No CTRL bundle was found for this frame id, which should not happen, since LINDEV UART
         * will not call us for non-handled frames except when we mapped something in LINDEV UART
         * but didn't mapped in LINDEV table.
         */
        LINDEV_TABLE_LOG(E, "No CTRL bundle was found for frame ID [%02x]", initial_frame_id);
        return NULL;
    }
    /* Extract all frame data to Dometic data */
    ctrl_bundle->extract(ctrl_bundle->buffer, frame_data);
    return ctrl_bundle;
}

lindev_table_status_t lindev_table_ctrl_synchronize_with_info_bundle(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state,
    const lindev_table_ctrl_bundle_t * ctrl_bundle,
    const lindev_table_info_bundle_t ** p_info_bundle)
{
    const lindev_table_info_bundle_t * info_bundle;
    lindev_table_status_t sync_status;
    size_t current_page_id;
    size_t next_page_id;
    size_t bundle_index = 0u;

    /* Find a matching INFO bundle */
    info_bundle = NULL;
    for (; bundle_index < lindev_table->bundle_map_length; bundle_index++)
    {
        if (lindev_table->bundle_map[bundle_index].ctrl == NULL)
        {
            continue;
        }
        if (lindev_table->bundle_map[bundle_index].ctrl == ctrl_bundle)
        {
            info_bundle = lindev_table->bundle_map[bundle_index].info;
            break;
        }
    }
    *p_info_bundle = info_bundle;
    if (info_bundle == NULL)
    {
        /* NOTE:
         * This happens when we have umatched frames in bundle_map.
         */
        return LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE;
    }
    /* In case we are using simple synchronization method these functions will return 0 */
    current_page_id = ctrl_bundle_get_ctrl_page_id(ctrl_bundle);
    next_page_id = ctrl_bundle_get_info_page_id(ctrl_bundle);
    LINDEV_TABLE_LOG(I, "got CTRL bundle: ID [%02x] [CTRL PAGE: %u, INFO PAGE: %u]",
        ctrl_bundle->frame_id,
        current_page_id,
        next_page_id);
    sync_status = sync_sm_dispatch(
        /* sm_state */      &lindev_table_state->sync_state[bundle_index][current_page_id],
        /* sm_event */      SYNC_SM_EVENT_CTRL,
        /* info_bundle */   info_bundle,
        /* ctrl_bundle */   ctrl_bundle);
    /* Prepare next INFO frame page */
    info_bundle_set_page_id(info_bundle, next_page_id);
    /* Evaluate if Local Change needs to be set if next page is in synchronization stage */
    if (sync_sm_is_in_notifying_state(&lindev_table_state->sync_state[bundle_index][next_page_id]))
    {
        info_bundle_set_local_change(info_bundle);
    }
    else
    {
        info_bundle_rst_local_change(info_bundle);
    }
    return sync_status;
}

lindev_table_status_t lindev_table_info_stuff_next_page(
    const lindev_table_config_t * lindev_table,
    uint32_t initial_frame_id,
    uint8_t * frame_data)
{
    const lindev_table_info_bundle_t * info_bundle;

    info_bundle = info_bundle_get_by_initial_frame_id(lindev_table, initial_frame_id);
    if (info_bundle == NULL)
    {
        return LINDEV_TABLE_STATUS_ERROR;
    }
    if ((info_bundle->protocol) && (info_bundle->protocol->page_num != 0u))
    {
        uint32_t page_id;

        page_id = *((uint8_t *)info_bundle->buffer + info_bundle->protocol->page_id_offset);
        page_id++;
        if (page_id == info_bundle->protocol->page_num)
        {
            page_id = 0u;
        }
        *((uint8_t *)info_bundle->buffer + info_bundle->protocol->page_id_offset) = (uint8_t)page_id;
        info_bundle->stuff(info_bundle->buffer, frame_data);
        return LINDEV_TABLE_STATUS_OK;
    }
    else
    {
        return LINDEV_TABLE_STATUS_PAGE_NOT_AVAILABLE;
    }
}

void lindev_table_ctrl_conv_to_ddm(const lindev_table_config_t * lindev_table,
    const lindev_table_ctrl_bundle_t * ctrl_bundle,
    void * ddm_workspace,
    size_t ddm_workspace_size,
    lindev_table_get_ddm_cb * get_ddm_cb,
    lindev_table_set_ddm_cb * set_ddm_cb,
    void * ddm_cb_arg)
{
    for (size_t i = 0u; i < lindev_table->ctrl_link_map_length; i++)
    {
        const lindev_table_ctrl_link_map_entry_t * link_map_entry = &lindev_table->ctrl_link_map[i];
        size_t ddm_data_size;
        size_t current_page_id;

        current_page_id = ctrl_bundle_get_ctrl_page_id(ctrl_bundle);
        /* Is this link map entry managed by the given CTRL bundle and currently selected page */
        if ((link_map_entry->bundle != ctrl_bundle) || (link_map_entry->page != current_page_id))
        {
            continue;
        }
        /* Get DDM value */
        ddm_data_size = get_ddm_cb(ddm_cb_arg, link_map_entry->ddm, ddm_workspace, ddm_workspace_size);
        if (ddm_data_size == 0)
        {
            /* For some reason we didn't get the DDM value (maybe it was never set). In this case we
             * clear the ddm_workspace buffer.
             */
            memset(ddm_workspace, 0, ddm_workspace_size);
        }
        /* Execute data conversion function */
        ddm_data_size = link_map_entry->bundle_to_ddm(
            (const uint8_t *)ctrl_bundle->buffer + link_map_entry->offset,
            link_map_entry->size,
            ddm_workspace,
            ddm_workspace_size);
        if (ddm_data_size == 0)
        {
            /* Converter function didn't write anything to this DDM value, skip setting it */

            continue;
        }
        /* Set DDM value */
        set_ddm_cb(ddm_cb_arg, link_map_entry->ddm, ddm_workspace, ddm_data_size);
    }
}

size_t lindev_table_get_info_link_map_length(const lindev_table_config_t * lindev_table)
{
    return lindev_table->info_link_map_length;
}

const lindev_table_info_link_map_entry_t * lindev_table_match_ddm_with_link_map_entry(
    const lindev_table_config_t * lindev_table,
    uint32_t ddm_parameter,
    size_t index)
{
    for (size_t i = 0u; i < LINDEV_TABLE_MAX_DDM_PER_INFO_LINK_MAP_ENTRY; i++)
    {
        if (lindev_table->info_link_map[index].ddm[i] == ddm_parameter)
        {
            return &lindev_table->info_link_map[index];
        }
    }
    return NULL;
}

lindev_table_status_t lindev_table_info_set_pending_update(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state,
    const lindev_table_info_link_map_entry_t * link_map_entry)
{
    const lindev_table_info_bundle_t * info_bundle;
    const lindev_table_ctrl_bundle_t * ctrl_bundle;
    size_t bundle_index;

    info_bundle = link_map_entry->bundle;
    bundle_index = map_info_bundle_to_bundle_index(lindev_table, info_bundle);
    if (bundle_index == SIZE_MAX)
    {
        return LINDEV_TABLE_STATUS_BUNDLE_NOT_AVAILABLE;
    }
    if (info_bundle->protocol == NULL)
    {
        return LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE;
    }
    ctrl_bundle = lindev_table->bundle_map[bundle_index].ctrl;
    /* If the selected page is the current page then we need to update local change as well */
    size_t current_page = info_bundle_get_page_id(info_bundle);
    if (current_page == link_map_entry->page)
    {
        info_bundle_set_local_change(info_bundle);
    }
    return sync_sm_dispatch(
        /* sm_state */      &lindev_table_state->sync_state[bundle_index][link_map_entry->page],
        /* sm_event */      SYNC_SM_EVENT_LC,
        /* info_bundle */   info_bundle,
        /* ctrl_bundle */   ctrl_bundle);
}

const lindev_table_info_bundle_t * lindev_table_info_conv_to_dometic(
    const lindev_table_config_t * lindev_table,
    const lindev_table_info_link_map_entry_t * link_map_entry,
    const void * const * ddm_data,
    const size_t * ddm_data_size)
{
    void * dometic_data;
    size_t dometic_data_size;
    const lindev_table_info_bundle_t * info_bundle;

    info_bundle = link_map_entry->bundle;
    dometic_data = (uint8_t *)info_bundle->buffer + link_map_entry->offset;
    dometic_data_size = link_map_entry->size;
    link_map_entry->ddm_to_bundle(ddm_data, ddm_data_size, dometic_data_size, dometic_data);
    return link_map_entry->bundle;
}

void lindev_table_info_bundle_stuff_to_frame(
    const lindev_table_config_t * lindev_table,
    const lindev_table_info_bundle_t * info_bundle,
    uint8_t * frame_data,
    uint_fast8_t * initial_frame_id)
{
    info_bundle->stuff(info_bundle->buffer, frame_data);
    *initial_frame_id = info_bundle->frame_id;
}

void lindev_table_request_sleep(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state)
{
    for (size_t bundle_index = 0u; bundle_index < lindev_table->bundle_map_length; bundle_index++)
    {
        for (size_t page_index = 0u; page_index < LINDEV_TABLE_MAX_PAGES_PER_FRAME; page_index++)
        {
            sync_sm_dispatch(
                &lindev_table_state->sync_state[bundle_index][page_index],
                SYNC_SM_EVENT_SLEEP,
                lindev_table->bundle_map[bundle_index].info,
                lindev_table->bundle_map[bundle_index].ctrl);
            info_bundle_rst_local_change(lindev_table->bundle_map[bundle_index].info);
        }
    }
}
