/*! \file lindev_table.h
    \brief LINDEV Table is a component of LINDEV stack for mapping of data and describing LIN frames
    \author Nenad Radulovic

    LINDEV Table is a component of LINDEV stack that provides:
     - structures for defining mapping tables
     - structures for describing LIN frame data
     - structures for describing LIN synchronization and LIN error reporting
     - helper functions which read and write LIN data
     - helper functions which execute LIN synchronization protocol

    For more details about this component refer to documentation located in
    `documentation/connectors/lindev`.
 */
/** @{ */
#ifndef LINDEV_TABLE_H_
#define LINDEV_TABLE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "configuration.h"

/**
 * @brief       LINDEV Table verbose logging
 *
 * By default, LINDEV Table will not print any message. Define this macro in local project
 * configuration file to enable verbose logging.
 *
 * @note	    This will generate a lot of logs.
 * @note	    Do not leave this option enabled in firmware production releases.
 * @note	    When logging is enabled, 40ms LIN period is minimal LIN period.
 */
#ifndef LINDEV_TABLE_VERBOSE_LOG
#define LINDEV_TABLE_VERBOSE_LOG				        0
#endif

/**
 * @brief       Define how many DDM parameters we can map to a single info link map member
 *
 * @note        If this parameter is changed, the macro @ref LINDEV_TABLE_INFO_ENTRY needs to be
 *              changed as well.
 */
#define LINDEV_TABLE_MAX_DDM_PER_INFO_LINK_MAP_ENTRY    3

/**
 * @brief       Define how many frame pairs we support
 *
 * A frame pair is one CTRL frame and one INFO frame which are to be synchronized. This number
 * determines how many CTRL & INFO frame pairs we support in LINDEV table.
 */
#define LINDEV_TABLE_MAX_BUNDLE_MAP_ENTRIES             4

/**
 * @brief       Define how many pages we support in each frame
 *
 * This number determines how many frame pages in each frame we support in LINDEV table.
 */
#define LINDEV_TABLE_MAX_PAGES_PER_FRAME                3

/**
 * @brief       Describe the synchronization protocol in LINDEV Table Protocol structure for LIN CTRL
 *              frame
 *
 * This macro is used to create a description of protocol structure in Dometic bundle structure. It
 * will calculate the position of the given member and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has a `sync_frame` bit used for Dometic
 * synchronization protocol.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       sync_frame_member is the name of `sync_frame` member in the Dometic bundle structure
 *              specified in @a type argument.
 */
#define LINDEV_TABLE_CTRL_PROTOCOL(type, sync_frame_member)                                         \
    {                                                                                               \
        .sync_frame_offset = offsetof(type, sync_frame_member),                                     \
        .has_sync_frame = true,                                                                     \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN CTRL frame
 *
 * LINDEV Table Bundle is a structure that:
 * - describes the contents of Dometic bundle structure,
 * - tells which Dometic synchronization protocol description to use
 * - tells which LIN frame is associated with
 * - contains stuff or extract function
 *
 * This macro is used to create a description of remote_data structure in Dometic bundle structure.
 * It will calculate the position of the given member and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has a `remote_data` member in Dometic bundle
 * structure.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       ctrl_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type
 *              used to define the Dometic bundle structure. The Dometic bundle structure is defined
 *              in `dometic.h` header.
 * @param       remote_data_member name of `remote_data` member in Dometic bundle structure. Remote
 *              data member is shared between LIN INFO and LIN CTRL frames which are to be
 *              synchronized. During synchronization this structure in LIN CTRL frame must be equal
 *              to structure in LIN INFO frame.
 * @param       bundle_buffer is pointer to Dometic CTRL bundle structure instance
 * @param       ctrl_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol description.
 *              See @ref lindev_table_ctrl_protocol_t.
 * @param       extract_fn Pointer to Dometic extract function.
 */
#define LINDEV_TABLE_CTRL_BUNDLE(ctrl_frame_id, type, remote_data_member, bundle_buffer, ctrl_protocol, extract_fn)            \
    {                                                                                               \
        .frame_id = (ctrl_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member),                                \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member),                             \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (ctrl_protocol),                                                                \
        .extract = (extract_fn),                                                                    \
    }

/**
 * @brief       Describe a LINDEV Table Link Map entry for LIN CTRL frame
 *
 * LINDEV Table CTRL link map is a map with a primary function to map a Dometic bundle structure
 * member to a particular DDM parameter.
 *
 * The LINDEV Table Link Map entry is a structure that:
 * - describes Dometic bundle structure member mapping to a DDM parameter
 * - tells which LINDEV Table CTRL bundle is used for storing the Dometic bundle structure member
 * - tells which converter function to use from Dometic converter functions
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure.
 * @param       member is name of the Dometic bundle structure member which we want to map to a DDM
 *              parameter.
 * @param       bundle_instance pointer to the LINDEV Table Bundle instance where the member is
 *              stored.
 * @param       conv_to_ddm is a function pointer to Dometic converter function which will convert
 *              the Dometic bundle structure member value into a value which is expected by mapped
 *              DDM parameter.
 * @param       ddm_parameter is DDM parameter identifier to which the Dometic bundle structure
 *              member is mapped to.
 */
#define LINDEV_TABLE_CTRL_ENTRY(type, member, bundle_instance, conv_to_ddm, ddm_parameter) \
    {                                                                       \
        .ddm = (ddm_parameter),                                             \
        .offset = offsetof(type, member),                                   \
        .size = sizeof(((type *)0)->member),                                \
        .page = 0u,                                                         \
        .bundle = (bundle_instance),                                        \
        .bundle_to_ddm = (conv_to_ddm),                                     \
    }

/**
 * @brief       Describe the synchronization protocol in LINDEV Table Protocol structure for LIN INFO
 *              frame
 *
 * This macro is used to create a description of protocol structure in Dometic bundle structure. It
 * will calculate the position of the given member and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has a `local_change` bit used for Dometic
 * synchronization protocol.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       local_change_member is the name of `local_change` member in the Dometic bundle
 *              structure specified in @a type argument.
 */
#define LINDEV_TABLE_INFO_PROTOCOL(type, local_change_member)                                       \
    {                                                                                               \
        .local_change_offset = offsetof(type, local_change_member),                                 \
        .has_local_change = true,                                                                   \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN INFO frame
 *
 * LINDEV Table Bundle is a structure that:
 * - describes the contents of Dometic bundle structure,
 * - tells which Dometic synchronization protocol description to use
 * - tells which LIN frame is associated with
 * - contains stuff or extract function
 *
 * This macro is used to create a description of remote_data structure in Dometic bundle structure.
 * It will calculate the position of the given member and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has a `remote_data` member in Dometic bundle
 * structure.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       info_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       remote_data_member name of `remote_data` member in Dometic bundle structure. Remote
 *              data member is shared between LIN INFO and LIN CTRL frames which are to be
 *              synchronized. During synchronization this structure in LIN CTRL frame must be equal
 *              to structure in LIN INFO frame.
 * @param       bundle_buffer is pointer to Dometic INFO bundle structure instance
 * @param       info_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol
 *              description. See @ref lindev_table_info_protocol_t.
 * @param       stuff_fn Pointer to Dometic stuff function.
 */
#define LINDEV_TABLE_INFO_BUNDLE(info_frame_id, type, remote_data_member, bundle_buffer, info_protocol, stuff_fn)            \
    {                                                                                               \
        .frame_id = (info_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member),                                \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member),                             \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (info_protocol),                                                                \
        .stuff = (stuff_fn),                                                                        \
    }

/**
 * @brief       Describe a LINDEV Table Link Map entry for LIN INFO frame
 *
 * LINDEV Table INFO Link Map is a map with a primary function to map a DDM parameter to a
 * particular Dometic bundle structure member.
 *
 * The LINDEV Table INFO Link Map entry is a structure that:
 * - describes Dometic bundle structure member mapping to a DDM parameter
 * - tells which LINDEV Table CTRL bundle is used for storing the Dometic bundle structure member
 * - tells which converter function to use from Dometic converter functions
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       ddm1 is the first DDM parameter identifier
 * @param       ddm2 is the second DDM parameter identifier
 * @param       ddm3 is the third DDM parameter identifier. All DDM identifiers are stored in an
 *              array which is of fixed length and the lenfth is determined at compile time by
 *              @ref LINDEV_TABLE_DDM_PER_INFO_LINK_MAP_ENTRY macro. All the DDM parameter
 *              identifiers are evaluated and mapped to a single Dometic bundle structure member.
 * @param       conv_to_bundle is a function pointer to Dometic converter function which will
 *              convert DDM parameter values into a value which is expected by mapped Dometic bundle
 *              structure member.
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure.
 * @param       member is the name of the Dometic bundle structure member which will be written to.
 * @param       bundle_instance pointer to the LINDEV Table Bundle instance where the member is
 *              stored.
 */
#define LINDEV_TABLE_INFO_ENTRY(ddm1, ddm2, ddm3, conv_to_bundle, type, member, bundle_instance)   \
    {                                                                           \
        .ddm = {(ddm1), (ddm2), (ddm3)},                                        \
        .offset = offsetof(type, member),                                       \
        .size = sizeof(((type *)0)->member),                                    \
        .page = 0u,                                                             \
        .bundle = (bundle_instance),                                            \
        .ddm_to_bundle = (conv_to_bundle),                                      \
    }

/**
 * @brief       Describe the synchronization protocol in LINDEV Table Protocol structure for LIN CTRL
 *              frame with pages
 *
 * This macro is used to create a description of protocol structure in Dometic bundle structure. It
 * will calculate the position of the given members and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has synchronization and paging bits used for Dometic
 * synchronization protocol.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       sync_frame_member is the name of `sync_frame` member in the Dometic bundle structure
 *              specified in @a type argument.
 * @param       ctrl_page_id_member is the name of `ctrl_page_id` member in the Dometic bundle
 *              structure which is specified ni @a type argument.
 * @param       info_page_id_member is the name of `info_page_id` member in the Dometic bundle
 *              structure which is specified ni @a type argument.
 */
#define LINDEV_TABLE_PAGED_CTRL_PROTOCOL(type, sync_frame_member, ctrl_page_id_member, info_page_id_member) \
    {                                                                                               \
        .sync_frame_offset = offsetof(type, sync_frame_member),                                     \
        .has_sync_frame = true,                                                                     \
        .ctrl_page_id_offset = offsetof(type, ctrl_page_id_member),                                 \
        .info_page_id_offset = offsetof(type, info_page_id_member),                                 \
        .has_ctrl_and_info_page_id = true,                                                          \
    }

/**
 * @brief       Describe the NO synchronization protocol in LINDEV Table Protocol structure for LIN
 *              CTRL frame with pages
 *
 * This macro is used to create a description of protocol structure in Dometic bundle structure. It
 * will calculate the position of the given members and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has no synchronization bits and only paging bits are
 * used for Dometic the protocol.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       sync_frame_member is the name of `sync_frame` member in the Dometic bundle structure
 *              specified in @a type argument.
 * @param       ctrl_page_id_member is the name of `ctrl_page_id` member in the Dometic bundle
 *              structure which is specified ni @a type argument.
 * @param       info_page_id_member is the name of `info_page_id` member in the Dometic bundle
 *              structure which is specified ni @a type argument.
 */
#define LINDEV_TABLE_PAGED_CTRL_PROTOCOL_NO_SYNC(type, sync_frame_member, ctrl_page_id_member, info_page_id_member) \
    {                                                                                               \
        .sync_frame_offset = 0,                                                                     \
        .has_sync_frame = false,                                                                    \
        .ctrl_page_id_offset = offsetof(type, ctrl_page_id_member),                                 \
        .info_page_id_offset = offsetof(type, info_page_id_member),                                 \
        .has_ctrl_and_info_page_id = true,                                                          \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN CTRL frame with 1 page
 *
 * LINDEV Table Bundle is a structure that:
 * - describes the contents of Dometic bundle structure,
 * - tells which Dometic synchronization protocol description to use
 * - tells which LIN frame is associated with
 * - tells remote data pages contents
 * - contains stuff or extract function
 *
 * This macro is used to create a description of remote_data structure in Dometic bundle structure.
 * It will calculate the position of the given members and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has a `remote_data` member in Dometic bundle
 * structure. Up to @ref LINDEV_TABLE_MAX_PAGES_PER_FRAME are supported in a single frame. Use this
 * macro to describe a frame with one page.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       ctrl_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type
 *              used to define the Dometic bundle structure. The Dometic bundle structure is defined
 *              in `dometic.h` header.
 * @param       remote_data_member_0 name of `remote_data` member in Dometic bundle structure.
 *              Remote data member is shared between LIN INFO and LIN CTRL frames which are to be
 *              synchronized. During synchronization this structure in LIN CTRL frame must be equal
 *              to structure in LIN INFO frame.
 * @param       bundle_buffer is pointer to Dometic CTRL bundle structure instance
 * @param       ctrl_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol description.
 *              See @ref lindev_table_ctrl_protocol_t.
 * @param       extract_fn Pointer to Dometic extract function.
 */
#define LINDEV_TABLE_PAGED_CTRL_BUNDLE_1(ctrl_frame_id, type, remote_data_member_0, bundle_buffer, ctrl_protocol, extract_fn)            \
    {                                                                                               \
        .frame_id = (ctrl_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member_0),                              \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member_0),                           \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (ctrl_protocol),                                                                \
        .extract = (extract_fn),                                                                    \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN CTRL frame with 2 pages
 *
 * For details see @ref LINDEV_TABLE_PAGED_CTRL_BUNDLE_1 macro.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       ctrl_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type
 *              used to define the Dometic bundle structure. The Dometic bundle structure is defined
 *              in `dometic.h` header.
 * @param       remote_data_member_0 name of `remote_data` member of first page in Dometic bundle
 *              structure.  Remote data member is shared between LIN INFO and LIN CTRL frames which
 *              are to be synchronized. During synchronization this structure in LIN CTRL frame must
 *              be equal to structure in LIN INFO frame.
 * @param       remote_data_member_1 name of `remote_data` member of second page in Dometic bundle
 *              structure.
 * @param       bundle_buffer is pointer to Dometic CTRL bundle structure instance
 * @param       ctrl_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol description.
 *              See @ref lindev_table_ctrl_protocol_t.
 * @param       extract_fn Pointer to Dometic extract function.
 */
#define LINDEV_TABLE_PAGED_CTRL_BUNDLE_2(ctrl_frame_id, type, remote_data_member_0, remote_data_member_1, bundle_buffer, ctrl_protocol, extract_fn)            \
    {                                                                                               \
        .frame_id = (ctrl_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member_0),                              \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member_0),                           \
        .remote_data_offset[1] = offsetof(type, remote_data_member_1),                              \
        .remote_data_size[1] = sizeof(((type *)0)->remote_data_member_1),                           \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (ctrl_protocol),                                                                \
        .extract = (extract_fn),                                                                    \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN CTRL frame with 3 pages
 *
 * For details see @ref LINDEV_TABLE_PAGED_CTRL_BUNDLE_1 macro.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       ctrl_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type
 *              used to define the Dometic bundle structure. The Dometic bundle structure is defined
 *              in `dometic.h` header.
 * @param       remote_data_member_0 name of `remote_data` member of first page in Dometic bundle
 *              structure.  Remote data member is shared between LIN INFO and LIN CTRL frames which
 *              are to be synchronized. During synchronization this structure in LIN CTRL frame must
 *              be equal to structure in LIN INFO frame.
 * @param       remote_data_member_1 name of `remote_data` member of second page in Dometic bundle
 *              structure.
 * @param       remote_data_member_2 name of `remote_data` member of third page in Dometic bundle
 *              structure.
 * @param       bundle_buffer is pointer to Dometic CTRL bundle structure instance
 * @param       ctrl_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol description.
 *              See @ref lindev_table_ctrl_protocol_t.
 * @param       extract_fn Pointer to Dometic extract function.
 */
#define LINDEV_TABLE_PAGED_CTRL_BUNDLE_3(ctrl_frame_id, type, remote_data_member_0, remote_data_member_1, remote_data_member_2, bundle_buffer, ctrl_protocol, extract_fn)            \
    {                                                                                               \
        .frame_id = (ctrl_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member_0),                              \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member_0),                           \
        .remote_data_offset[1] = offsetof(type, remote_data_member_1),                              \
        .remote_data_size[1] = sizeof(((type *)0)->remote_data_member_1),                           \
        .remote_data_offset[2] = offsetof(type, remote_data_member_2),                              \
        .remote_data_size[2] = sizeof(((type *)0)->remote_data_member_2),                           \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (ctrl_protocol),                                                                \
        .extract = (extract_fn),                                                                    \
    }

/**
 * @brief       Describe a LINDEV Table Link Map entry for LIN CTRL frame with pages
 *
 * LINDEV Table CTRL link map is a map with a primary function to map a Dometic bundle structure
 * member to a particular DDM parameter.
 *
 * The LINDEV Table Link Map entry is a structure that:
 * - describes Dometic bundle structure member mapping to a DDM parameter
 * - tells which LINDEV Table CTRL bundle is used for storing the Dometic bundle structure member
 * - tells which converter function to use from Dometic converter functions
 * - tells which frame page is used in mapping
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure.
 * @param       member is name of the Dometic bundle structure member which we want to map to a DDM
 *              parameter.
 * @param       page_index page index in the frame. Page is used when mapping data from frame with
 *              pages.
 * @param       bundle_instance pointer to the LINDEV Table Bundle instance where the member is
 *              stored.
 * @param       conv_to_ddm is a function pointer to Dometic converter function which will convert
 *              the Dometic bundle structure member value into a value which is expected by mapped
 *              DDM parameter.
 * @param       ddm_parameter is DDM parameter identifier to which the Dometic bundle structure
 *              member is mapped to.
 */
#define LINDEV_TABLE_PAGED_CTRL_ENTRY(type, member, page_index, bundle_instance, conv_to_ddm, ddm_parameter) \
    {                                                                       \
        .ddm = (ddm_parameter),                                             \
        .offset = offsetof(type, member),                                   \
        .size = sizeof(((type *)0)->member),                                \
        .page = (page_index),                                               \
        .bundle = (bundle_instance),                                        \
        .bundle_to_ddm = (conv_to_ddm),                                     \
    }

/**
 * @brief       Describe the synchronization protocol in LINDEV Table Protocol structure for LIN INFO
 *              frame with pages
 *
 * This macro is used to create a description of protocol structure in Dometic bundle structure. It
 * will calculate the position of the given members and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has synchronization and paging bits used for Dometic
 * synchronization protocol.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       local_change_member is the name of `local_change` member in the Dometic bundle
 *              structure specified in @a type argument.
 * @param       page_id_member is the name of `page_id` member in the Dometic bundle structure which
 *              is specified in @a type argument.
 * @param       page_id_number is the number of pages in the described INFO frame.
 */
#define LINDEV_TABLE_PAGED_INFO_PROTOCOL(type, local_change_member, page_id_member, page_id_number) \
    {                                                                                               \
        .local_change_offset = offsetof(type, local_change_member),                                 \
        .has_local_change = true,                                                                   \
        .page_id_offset = offsetof(type, page_id_member),                                           \
        .page_num = (page_id_number),                                                               \
    }

/**
 * @brief       Describe the NO synchronization protocol in LINDEV Table Protocol structure for LIN
 *              INFO frame with pages
 *
 * This macro is used to create a description of protocol structure in Dometic bundle structure. It
 * will calculate the position of the given members and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has no synchronization bits and only paging bits are
 * used for Dometic protocol.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       local_change_member is the name of `local_change` member in the Dometic bundle
 *              structure specified in @a type argument.
 * @param       page_id_member is the name of `page_id` member in the Dometic bundle structure which
 *              is specified in @a type argument.
 * @param       page_id_number is the number of pages in the described INFO frame.
 */
#define LINDEV_TABLE_PAGED_INFO_PROTOCOL_NO_SYNC(type, local_change_member, page_id_member, page_id_number)  \
    {                                                                                               \
        .local_change_offset = 0,                                                                   \
        .has_local_change = false,                                                                  \
        .page_id_offset = offsetof(type, page_id_member),                                           \
        .page_num = (page_id_number),                                                               \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN INFO frame with 1 page
 *
 * LINDEV Table Bundle is a structure that:
 * - describes the contents of Dometic bundle structure,
 * - tells which Dometic synchronization protocol description to use
 * - tells which LIN frame is associated with
 * - contains stuff or extract function
 *
 * This macro is used to create a description of remote_data structure in Dometic bundle structure.
 * It will calculate the position of the given member and set appropriate flags to signal to LINDEV
 * Table component that the described LIN frame has a `remote_data` member in Dometic bundle
 * structure.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       info_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       remote_data_member_0 name of `remote_data` member in Dometic bundle structure. Remote
 *              data member is shared between LIN INFO and LIN CTRL frames which are to be
 *              synchronized. During synchronization this structure in LIN CTRL frame must be equal
 *              to structure in LIN INFO frame.
 * @param       bundle_buffer is pointer to Dometic INFO bundle structure instance
 * @param       info_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol
 *              description. See @ref lindev_table_info_protocol_t.
 * @param       stuff_fn Pointer to Dometic stuff function.
 */
#define LINDEV_TABLE_PAGED_INFO_BUNDLE_1(info_frame_id, type, remote_data_member_0, bundle_buffer, info_protocol, stuff_fn)            \
    {                                                                                               \
        .frame_id = (info_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member_0),                              \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member_0),                           \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (info_protocol),                                                                \
        .stuff = (stuff_fn),                                                                        \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN INFO frame with 2 pages
 *
 * For details see @ref LINDEV_TABLE_PAGED_INFO_BUNDLE_1 macro.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       info_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       remote_data_member_0 name of `remote_data` member of the first page in Dometic
 *              bundle structure. Remote data member is shared between LIN INFO and LIN CTRL frames
 *              which are to be synchronized. During synchronization this structure in LIN CTRL
 *              frame must be equal to structure in LIN INFO frame.
 * @param       remote_data_member_1 name of `remote_data` member of the second page in Dometic
 *              bundle structure.
 * @param       bundle_buffer is pointer to Dometic INFO bundle structure instance
 * @param       info_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol
 *              description. See @ref lindev_table_info_protocol_t.
 * @param       stuff_fn Pointer to Dometic stuff function.
 */
#define LINDEV_TABLE_PAGED_INFO_BUNDLE_2(info_frame_id, type, remote_data_member_0, remote_data_member_1, bundle_buffer, info_protocol, stuff_fn)            \
    {                                                                                               \
        .frame_id = (info_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member_0),                              \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member_0),                           \
        .remote_data_offset[1] = offsetof(type, remote_data_member_1),                              \
        .remote_data_size[1] = sizeof(((type *)0)->remote_data_member_1),                           \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (info_protocol),                                                                \
        .stuff = (stuff_fn),                                                                        \
    }

/**
 * @brief       Describe a LINDEV Table Bundle for LIN INFO frame with 3 pages
 *
 * For details see @ref LINDEV_TABLE_PAGED_INFO_BUNDLE_1 macro.
 *
 * @note        This macro is for products that do not use paging machanism in frames.
 *
 * @param       info_frame_id is LIN frame identifier
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure. The Dometic bundle structure is defined in
 *              `dometic.h` header.
 * @param       remote_data_member_0 name of `remote_data` member of the first page in Dometic
 *              bundle structure. Remote data member is shared between LIN INFO and LIN CTRL frames
 *              which are to be synchronized. During synchronization this structure in LIN CTRL
 *              frame must be equal to structure in LIN INFO frame.
 * @param       remote_data_member_1 name of `remote_data` member of the second page in Dometic
 *              bundle structure.
 * @param       remote_data_member_2 name of `remote_data` member of the third page in Dometic
 *              bundle structure.
 * @param       bundle_buffer is pointer to Dometic INFO bundle structure instance
 * @param       info_protocol is pointer to LINDEV Table LINDEV Table synchronization protocol
 *              description. See @ref lindev_table_info_protocol_t.
 * @param       stuff_fn Pointer to Dometic stuff function.
 */
#define LINDEV_TABLE_PAGED_INFO_BUNDLE_3(info_frame_id, type, remote_data_member_0, remote_data_member_1, remote_data_member_2, bundle_buffer, info_protocol, stuff_fn)            \
    {                                                                                               \
        .frame_id = (info_frame_id),                                                                \
        .remote_data_offset[0] = offsetof(type, remote_data_member_0),                              \
        .remote_data_size[0] = sizeof(((type *)0)->remote_data_member_0),                           \
        .remote_data_offset[1] = offsetof(type, remote_data_member_1),                              \
        .remote_data_size[1] = sizeof(((type *)0)->remote_data_member_1),                           \
        .remote_data_offset[2] = offsetof(type, remote_data_member_2),                              \
        .remote_data_size[2] = sizeof(((type *)0)->remote_data_member_2),                           \
        .buffer = (bundle_buffer),                                                                  \
        .protocol = (info_protocol),                                                                \
        .stuff = (stuff_fn),                                                                        \
    }

/**
 * @brief       Describe a LINDEV Table Link Map entry for LIN INFO frame with pages
 *
 * LINDEV Table INFO Link Map is a map with a primary function to map a DDM parameter to a
 * particular Dometic bundle structure member.
 *
 * The LINDEV Table INFO Link Map entry is a structure that:
 * - describes Dometic bundle structure member mapping to a DDM parameter
 * - tells which LINDEV Table CTRL bundle is used for storing the Dometic bundle structure member
 * - tells which converter function to use from Dometic converter functions
 * - tells which frame page is use for mapping
 *
 * @param       ddm1 is the first DDM parameter identifier
 * @param       ddm2 is the second DDM parameter identifier
 * @param       ddm3 is the third DDM parameter identifier. All DDM identifiers are stored in an
 *              array which is of fixed length and the lenfth is determined at compile time by
 *              @ref LINDEV_TABLE_DDM_PER_INFO_LINK_MAP_ENTRY macro. All the DDM parameter
 *              identifiers are evaluated and mapped to a single Dometic bundle structure member.
 * @param       conv_to_bundle is a function pointer to Dometic converter function which will
 *              convert DDM parameter values into a value which is expected by mapped Dometic bundle
 *              structure member.
 * @param       type is the type of Dometic bundle structure. The macro expects here the C type used
 *              to define the Dometic bundle structure.
 * @param       member is the name of the Dometic bundle structure member which will be written to.
 * @param       page_index page index in the frame. Page is used when mapping data from frame with
 *              pages.
 * @param       bundle_instance pointer to the LINDEV Table Bundle instance where the member is
 *              stored.
 */
#define LINDEV_TABLE_PAGED_INFO_ENTRY(ddm1, ddm2, ddm3, conv_to_bundle, type, member, page_index, bundle_instance)   \
    {                                                                           \
        .ddm = {(ddm1), (ddm2), (ddm3)},                                        \
        .offset = offsetof(type, member),                                       \
        .size = sizeof(((type *)0)->member),                                    \
        .page = (page_index),                                                   \
        .bundle = (bundle_instance),                                            \
        .ddm_to_bundle = (conv_to_bundle),                                      \
    }

/**
 * @brief       LINDEV Table synchronization protocol description for LIN CTRL frame
 *
 * The members of this structure are defined using @ref LINDEV_TABLE_CTRL_PROTOCOL or
 * @ref LINDEV_TABLE_PAGED_CTRL_PROTOCOL, @ref LINDEV_TABLE_PAGED_CTRL_PROTOCOL_NO_SYNC macros. Each
 * LINDEV Table Bundle should have a protocol description structure.
 */
typedef struct lindev_table_ctrl_protocol
{
    size_t sync_frame_offset;               //!< Offset of `sync_frame` member in Dometic bundle structure
    bool has_sync_frame;                    //!< Tells if Dometic bundle structure has `sync_frame` member
    size_t ctrl_page_id_offset;             //!< Offset of `ctrl_page_id` member in Dometic bundle structure
    size_t info_page_id_offset;             //!< Offset of `info_page_id` member in Dometic bundle structure
    bool has_ctrl_and_info_page_id;         //!< Tells if Dometic bundle structure has `ctrl_page_id` and `info_page_id` members
} lindev_table_ctrl_protocol_t;

/**
 * @brief       LINDEV Table synchronization protocol description for LIN INFO frame
 *
 * The members of this structure are defined using @ref LINDEV_TABLE_INFO_PROTOCOL or
 * @ref LINDEV_TABLE_PAGED_INFO_PROTOCOL macros. Each LINDEV Table Bundle should have a protocol
 * description structure.
 */
typedef struct lindev_table_info_protocol
{
    size_t local_change_offset;             //!< Offset of `local_change` member in Dometic bundle structure
    bool has_local_change;                  //!< Tells if Dometic bundle structure has `local_change` member
    size_t response_error_offset;           //!< Offset of `response_error` member in Dometic bundle structure
    bool has_response_error;                //!< Tells if Dometic bundle structure has `response_error` member
    size_t page_id_offset;                  //!< Offset of `page_id_offset` member in Dometic bundle structure
    size_t page_num;                        //!< Tells if Dometic bundle structure has pages and how many of them
} lindev_table_info_protocol_t;

/**
 * @brief       LINDEV Table Bundle for LIN CTRL frame
 *
 * LINDEV Table Bundle is a structure that:
 * - describes the contents of Dometic bundle structure,
 * - tells which Dometic synchronization protocol description to use
 * - tells which LIN frame is associated with
 * - contains stuff or extract function
 *
 * The members of this structure are defined using @ref LINDEV_TABLE_CTRL_BUNDLE,
 * @ref LINDEV_TABLE_PAGED_CTRL_BUNDLE_1, @ref LINDEV_TABLE_PAGED_CTRL_BUNDLE_2 or
 * @ref LINDEV_TABLE_PAGED_CTRL_BUNDLE_3 macros.
 */
typedef struct lindev_table_ctrl_bundle
{
    size_t frame_id;                                                    //!< LIN frame identifier
    size_t remote_data_offset[LINDEV_TABLE_MAX_PAGES_PER_FRAME];        //!< Offset of `remote_data` member in Dometic bundle structure
    size_t remote_data_size[LINDEV_TABLE_MAX_PAGES_PER_FRAME];          //!< Size of `remote_data` member in Dometic bundle structure
    void * buffer;                                                      //!< Pointer to Dometic CTRL bundle structure instance
    const lindev_table_ctrl_protocol_t * protocol;                      //!< Pointer to LINDEV Table synchronization protocol description
    void (* extract)(void * bundle, const uint8_t * data);              //!< Pointer to Dometic extract function
} lindev_table_ctrl_bundle_t;

/**
 * @brief       LINDEV Table Bundle for LIN INFO frame
 *
 * LINDEV Table Bundle is a structure that:
 * - describes the contents of Dometic bundle structure,
 * - tells which Dometic synchronization protocol description to use
 * - tells which LIN frame is associated with
 * - contains stuff or extract function
 *
 * The members of this structure are defined using @ref LINDEV_TABLE_INFO_BUNDLE,
 * @ref LINDEV_TABLE_PAGED_INFO_BUNDLE_1, @ref LINDEV_TABLE_PAGED_INFO_BUNDLE_2 or
 * @ref LINDEV_TABLE_PAGED_INFO_BUNDLE_3 macros.
 */
typedef struct lindev_table_info_bundle
{
    size_t frame_id;                                                    //!< LIN frame identifier
    size_t remote_data_offset[LINDEV_TABLE_MAX_PAGES_PER_FRAME];        //!< Offset of `remote_data` member in Dometic bundle structure
    size_t remote_data_size[LINDEV_TABLE_MAX_PAGES_PER_FRAME];          //!< Size of `remote_data` member in Dometic bundle structure
    void * buffer;                                                      //!< Pointer to Dometic INFO bundle structure instance
    const lindev_table_info_protocol_t * protocol;                      //!< Pointer to LINDEV Table synchronization protocol description
    void (* stuff)(const void * bundle, uint8_t * data);                //!< Pointer to Dometic extract function
} lindev_table_info_bundle_t;

/**
 * @brief       LINDEV Table Bundle mapping table entry
 *
 * LINDEV Table Bundle map is used to map LIN INFO and LIN CTRL frames which are involved in Dometic
 * synchronization protocol. In this map list all CTRL and INFO pairs which should be synchronized.
 *
 * If CTRL frame has no matched INFO frame leave INFO field empty.
 * If INFO frame has no matched CTRL frame leave CTRL field empty.
 */
typedef struct lindev_table_bundle_map_entry
{
    const lindev_table_ctrl_bundle_t * ctrl;    //!< Pointer to LINDEV Table Bundle for LIN CTRL frame
    const lindev_table_info_bundle_t * info;    //!< Pointer to LINDEV Table Bundle for LIN INFO frame
} lindev_table_bundle_map_entry_t;

/**
 * @brief       Function declaration of Dometic converter functions (to bundle)
 *
 * The prototype of this declaration must match the declaration of Dometic converter functions which
 * are converting DDM parameter values to Dometic bundle value.
 *
 * The parameters and return values are described in `dometic.h`.
 */
typedef void (lindev_table_ddm_to_bundle_fn_t)(
    const void * const * ddm_data,
    const size_t * ddm_data_size,
    size_t conv_data_size,
    void * conv_data);

/**
 * @brief       Function declaration of Dometic converter functions (to DDM)
 *
 * The prototype of this declaration must match the declaration of Dometic converter functions which
 * are converting Dometic bundle value to DDM parameter value.
 *
 * The parameters and return values are described in `dometic.h`.
 */
typedef size_t (lindev_table_bundle_to_ddm_fn_t)(
    const void * conv_data,
    size_t conv_data_size,
    void * ddm_data,
    size_t ddm_data_size);

/**
 * @brief       LINDEV Table Link Map entry for LIN CTRL frame
 *
 * LINDEV Table CTRL link map is a map with a primary function to map a Dometic bundle structure
 * member to a particular DDM parameter.
 *
 * The LINDEV Table CTRL Link Map entry is a structure that:
 * - describes Dometic bundle structure member mapping to a DDM parameter
 * - tells which LINDEV Table CTRL bundle is used for storing the Dometic bundle structure member
 * - tells which converter function to use from Dometic converter functions
 *
 * The members of this structure are defined using @ref LINDEV_TABLE_CTRL_ENTRY or
 * @ref LINDEV_TABLE_PAGED_CTRL_ENTRY macros.
 */
typedef struct lindev_table_ctrl_link_map_entry
{
    uint32_t ddm;                                                       //!< DDM parameter identifier
    size_t offset;                                                      //!< Offset of Dometic bundle structure member
    size_t size;                                                        //!< Size of Dometic bundle structure member in bytes
    uint32_t page;                                                      //!< Page of the mapped data in frame
    const lindev_table_ctrl_bundle_t * bundle;                          //!< Pointer to LINDEV Table CTRL bundle
    lindev_table_bundle_to_ddm_fn_t * bundle_to_ddm;                    //!< Pointer to Dometic converter function
} lindev_table_ctrl_link_map_entry_t;

/**
 * @brief       LINDEV Table Link Map entry for LIN INFO frame
 *
 * LINDEV Table INFO Link Map is a map with a primary function to map a DDM parameter to a
 * particular Dometic bundle structure member.
 *
 * The LINDEV Table INFO Link Map entry is a structure that:
 * - describes Dometic bundle structure member mapping to a DDM parameter
 * - tells which LINDEV Table CTRL bundle is used for storing the Dometic bundle structure member
 * - tells which converter function to use from Dometic converter functions
 *
 * The members of this structure are defined using @ref LINDEV_TABLE_INFO_ENTRY or
 * @ref LINDEV_TABLE_PAGED_INFO_ENTRY macros.
 */
typedef struct lindev_table_info_link_map_entry
{
    const uint32_t ddm[LINDEV_TABLE_MAX_DDM_PER_INFO_LINK_MAP_ENTRY];
    size_t offset;
    size_t size;
    uint32_t page;
    const lindev_table_info_bundle_t * bundle;
    lindev_table_ddm_to_bundle_fn_t * ddm_to_bundle;
} lindev_table_info_link_map_entry_t;

/**
 * @brief       LINDEV Table synchronization method enumeration
 *
 * LINDEV Table is itended to cover multiple synchronization algorithms. This
 * enum lists all possible synchronization protocol variants.
 */
typedef enum lindev_table_sync_method
{
    LINDEV_TABLE_SYNC_METHOD_SIMPLE,            //!< Use single bit per frame
    LINDEV_TABLE_SYNC_METHOD_FRAME_PAGE,        //!< Use frame paging
} lindev_table_sync_method_t;

/**
 * @brief       Main LINDEV table configuration structure
 *
 * This structure is passed to all LINDEV table functions as the first argument.
 */
typedef struct lindev_table_config
{
    lindev_table_sync_method_t                  sync_method;            //!< LINDEV Table synchronization method
    const lindev_table_ctrl_link_map_entry_t *  ctrl_link_map;          //!< Pointer to array of LINDEV Table Link Map entries for LIN CTRL frame
    size_t                                      ctrl_link_map_length;   //!< Number of elements in ctrl_link_map array
    const lindev_table_info_link_map_entry_t *  info_link_map;          //!< Pointer to array of LINDEV Table Link Map entries for LIN INFO frame
    size_t                                      info_link_map_length;   //!< Number of elements in info_link_map array
    const lindev_table_bundle_map_entry_t *     bundle_map;             //!< Pointer to array of LINDEV Table Bundle map array
    size_t                                      bundle_map_length;      //!< Number of elements in bundle_map array
} lindev_table_config_t;

/**
 * @brief       Synchronization procedure status
 */
typedef enum lindev_table_status
{
    LINDEV_TABLE_STATUS_OK,                                             //!< Operation completed successfully
    LINDEV_TABLE_STATUS_ERROR,                                          //!< Error in synchronization protocol, check mapping
    LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE,                             //!< No sync is needed, since the LIN frame does not require it
    LINDEV_TABLE_STATUS_SYNC_NOT_PENDING,                               //!< Sync is not needed
    LINDEV_TABLE_STATUS_SYNC_PENDING,                                   //!< Sync is needed but CTRL frame is not a sync frame
    LINDEV_TABLE_STATUS_SYNC_COMPLETED,                                 //!< Sync is needed and we just have synchronized
    LINDEV_TABLE_STATUS_PAGE_NOT_AVAILABLE,                             //!< Page is not available
    LINDEV_TABLE_STATUS_BUNDLE_NOT_AVAILABLE                            //!< Frame bundle is not available for DDM2 parameter's mapped frame
} lindev_table_status_t;

/**
 * @brief       Synhronization state machine states
 *
 * This state machine is internal to LINDEV Table. It is used to manage synchronization status of
 * CTRL & INFO pairs.
 */
typedef enum lindev_table_sync_sm_state
{
    LINDEV_TABLE_SYNC_SM_STATE_NO_SYNC,                                 //!< Synchronization is not available
    LINDEV_TABLE_SYNC_SM_STATE_SYNCHRONIZED,                            //!< CTRL & INFO pair is synchronized
    LINDEV_TABLE_SYNC_SM_STATE_NOTIFYING,                               //!< Notifying master about local change
} lindev_table_sync_sm_state_t;

/**
 * @brief       Lindev table state
 *
 * This structure contains current state of frame synchronization protocol.
 */
typedef struct lindev_table_state
{
    /**
     * @brief   Synhronization status.
     *
     * This matrix contains synhronization status for each CTRL-INFO frame pair and for each PAGE of
     * the CTRL-INFO frame pair.
     */
    lindev_table_sync_sm_state_t sync_state[LINDEV_TABLE_MAX_BUNDLE_MAP_ENTRIES][LINDEV_TABLE_MAX_PAGES_PER_FRAME];
} lindev_table_state_t;

/**
 * @brief       Initialize lindev table state
 *
 * Table state structure contains runtime data which is used to store current state of frame
 * synchronization protocol.
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       lindev_table_state Pointer to lindev_table_state_t structure that needs to be
 *              synchronized.
 * @return      Operation status
 * @retval      LINDEV_TABLE_STATUS_OK Operation completed successfully
 * @retval      LINDEV_TABLE_STATUS_ERROR An error has occured
 */
lindev_table_status_t lindev_table_initialize_table_state(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state);

/**
 * @brief       Extract LIN CTRL frame into LINDEV Table Bundle
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       frame_data Pointer to raw LIN frame data
 * @param       initial_frame_id Initial LIN frame identifier
 * @return      Return pointer to const lindev_table_ctrl_bundle_t * which is responsible for the
 *              given LIN frame
 * @retval      NULL When LINDEV table can not find CTRL bundle for given @a initial_frame_id. This
 *              is error in mapping tables which needs to be corrected.
 */
const lindev_table_ctrl_bundle_t * lindev_table_ctrl_extract_to_dometic(
    const lindev_table_config_t * lindev_table,
    const uint8_t * frame_data,
    uint_fast8_t initial_frame_id);

/**
 * @brief       Synchronize the LINDEV Table CTRL bundle to paired LINDEV Table INFO Bundle
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       lindev_table_state Pointer to lindev table state structure.
 * @param       ctrl_bundle Pointer to LINDEV Table CTRL bundle. Usually this is the pointer which
 *              was obtained by a call to @ref lindev_table_ctrl_extract_to_dometic function.
 * @param [out] info_bundle Pointer to pointer to INFO bundle. This pointer is set to proper INFO
 *              bundle when an associated bundle is found. When no association exists between CTRL
 *              and INFO bundle this pointer is set to NULL.
 * @return      Dometic protocol synchronization operation status.
 * @retval      LINDEV_TABLE_STATUS_ERROR When LINDEV table can not find INFO bundle associated with
 *              given CTRL bundle. This is error in mapping tables which needs to be corrected.
 * @retval      LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE Is returned when mapped CTRL frame has no
 *              synchronization INFO bundle pair or no synchronization bits are mapped in the CTRL
 *              bundle.
 * @retval      LINDEV_TABLE_STATUS_SYNC_NOT_PENDING Is returned when given CTRL bundle is
 *              synchronized.
 * @retval      LINDEV_TABLE_STATUS_SYNC_PENDING Is returned when given CTRL bundle needs
 *              synchronization.
 * @retval      LINDEV_TABLE_STATUS_SYNC_COMPLETED Is returned when given CTRL bundle was needing
 *              synchronization and the synchronization has just got completed.
 */
lindev_table_status_t lindev_table_ctrl_synchronize_with_info_bundle(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state,
    const lindev_table_ctrl_bundle_t * ctrl_bundle,
    const lindev_table_info_bundle_t ** info_bundle);

/**
 * @brief       Prepare INFO bundle for next page (if it supports pages).
 *
 * @param       lindev_table LINDEV Table configuration structure instance.
 * @param       initial_frame_id Initial LIN frame identifier.
 * @param [out] frame_data Pointer to LIN frame data buffer where to put stuff data.
 * @return      Operation status.
 * @retval      LINDEV_TABLE_STATUS_ERROR When LINDEV table can not find INFO bundle associated with
 *              given initial_frame_id. This is error in mapping tables which needs to be corrected.
 * @retval      LINDEV_TABLE_STATUS_OK Operation completed successfully, the INFO frame page was
 *              incremented.
 * @retval      LINDEV_TABLE_STATUS_PAGE_NOT_AVAILABLE The INFO frame page is not defined for this
 *              bundle, so no operation was needed.
 */
lindev_table_status_t lindev_table_info_stuff_next_page(
    const lindev_table_config_t * lindev_table,
    uint32_t initial_frame_id,
    uint8_t * frame_data);

/**
 * @brief       LINDEV Table set DDM parameter value callback type
 *
 * This callback allows LINDEV Table to set a DDM parameter value while processing CTRL frames.
 *
 * The callback function receives the following arguments:
 * - arg: An argument which is just passed to callback function. LINDEV Table has no notion on what
 *   this pointer should point to. It is up to user of LINDEV Table component to provide a pointer
 *   that is just provided to this callback.
 * - ddm_parameter: Is DDM parameter identifier provided by LINDEV Table to callback function. It
 *   notes which DDM parameter value needs to be changed.
 * - ddm_data: Pointer to a memory provided by LINDEV Table component. This memory contains a value
 *   which needs to be written into specified DDM parameter.
 *
 *   Note that LINDEV Table component may try to set the same value which is already stored in the
 *   specified DDM parameter. It is up to LINDEV Table user to distinguish if the value has changed
 *   from last update.
 * - ddm_data_size: Size of value in bytes which is pointed by ddm_data pointer.
 */
typedef void (lindev_table_set_ddm_cb)(
    void * arg,
    uint32_t ddm_parameter,
    const void * ddm_data,
    size_t ddm_data_size);

/**
 * @brief       LINDEV table get DDM parameter value callback type
 *
 * This callback allows LINDEV Table to get a DDM parameter value while processing CTRL frames.
 *
 * This callback is needed to fetch existing DDM value and provide it to converter function so they
 * can modify the value or provide completelly new value.
 *
 * The callback function receives the following arguments:
 * - arg: An argument which is just passed to callback function. LINDEV Table has no notion on what
 *   this pointer should point to. It is up to user of LINDEV Table component to provide a pointer
 *   that is just provided to this callback.
 * - ddm_parameter: Is DDM parameter identifier provided by LINDEV Table to callback function. It
 *   notes which DDM parameter value needs to be changed.
 * - ddm_data: Pointer to a memory provided by LINDEV Table component. This memory will contain a
 *   value which is already into specified DDM parameter.
 * - ddm_data_size: Size of value in bytes which is pointed by ddm_data pointer.
 */
typedef size_t (lindev_table_get_ddm_cb)(void * arg, uint32_t ddm_parameter, void * ddm_data, size_t ddm_data_size);

/**
 * @brief       Convert an extracted Dometic value to a DDM value and set it
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       ctrl_bundle Pointer to LIDNEV Table CTRL bundle. Usually this is the  pointer which
 *              was obtained by a call to @ref lindev_table_ctrl_extract_to_dometic function.
 * @param       ddm_workspace Pointer to a temporary memory which can be used by LINDEV Table to
 *              convert a value and store it in this memory. The size of this memory should be of
 *              enough size as maximum expected size of a DDM parameter. If the mapping table
 *              contains exclusivelly `int32_t` DDM parameter types then the size of this can be 4
 *              bytes.
 * @param       ddm_workspace_size Specifies memory size in bytes. It is used to prevent/detect
 *              buffer overflows.
 * @param       get_ddm_cb Pointer to set DDM parameter value callback. See
 *              @ref lindev_table_get_ddm_cb function callback type.
 * @param       set_ddm_cb Pointer to set DDM parameter value callback. See
 *              @ref lindev_table_set_ddm_cb function callback type.
 * @param       ddm_cb_arg Pointer which is passed to `get_ddm_cb` and `set_ddm_cb` callback.
 */
void lindev_table_ctrl_conv_to_ddm(const lindev_table_config_t * lindev_table,
    const lindev_table_ctrl_bundle_t * ctrl_bundle,
    void * ddm_workspace,
    size_t ddm_workspace_size,
    lindev_table_get_ddm_cb * get_ddm_cb,
    lindev_table_set_ddm_cb * set_ddm_cb,
    void * ddm_cb_arg);

/**
 * @brief       Get the number of elements in LINDEV Table INFO Link Map.
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @return      The number of elements in LINDEV Table INFO Link Map.
 */
size_t lindev_table_get_info_link_map_length(const lindev_table_config_t * lindev_table);

/**
 * @brief       Try to match given DDM parameter with a LINDEV Table INFO Link Map
 *              entry.
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       ddm_parameter DDM parameter identifier
 * @param       index Index of the current LINDEV Table INFO Link Map entry
 * @return      Pointer to LINDEV Table INFO Link Map entry if the match is positive.
 *              For one DDM parameter there might be multiple LINDEV Table INFO link
 *              map entries.
 * @retval      NULL Is returned when no match was possible between given DDM
 *              parameter and a LINDEV Table INFO Link Map entry. This is an error in mapping tables
 *              and it needs to be corrected.
 */
const lindev_table_info_link_map_entry_t * lindev_table_match_ddm_with_link_map_entry(
    const lindev_table_config_t * lindev_table,
    uint32_t ddm_parameter,
    size_t index);

/**
 * @brief       Mark the LINDEV Table INFO bundle as pending synchronization.
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       link_map_entry Pointer to link_map_entry which was obtained by
 *              a call to @ref lindev_table_match_ddm_with_link_map_entry function.
 * @return      Dometic protocol synchronization operation status.
 * @retval      LINDEV_TABLE_STATUS_ERROR When LINDEV table can not find INFO bundle pointed by the
 *              given Link Map entry in bundle map. This is an error in mapping tables and it needs
 *              to be corrected.
 * @retval      LINDEV_TABLE_STATUS_SYNC_NOT_AVAILABLE Is returned when mapped CTRL frame has no
 *              synchronization INFO bundle pair or no synchronization bits are mapped in the CTRL
 *              bundle.
 * @retval      LINDEV_TABLE_STATUS_SYNC_PENDING Is returned when given INFO bundle needs
 *              synchronization.
 */
lindev_table_status_t lindev_table_info_set_pending_update(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state,
    const lindev_table_info_link_map_entry_t * link_map_entry);

/**
 * @brief       Convert the DDM parameter value to Dometic bundle value
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       link_map_entry Pointer to link_map_entry which was obtained by
 *              a call to @ref lindev_table_match_ddm_with_link_map_entry function.
 * @param       ddm_data Pointer to memory where the DDM parameter value is
 *              stored.
 * @param       ddm_data_size Size of memory pointed by ddm_data to prevent or
 *              detect buffer.
 * @return      Returns pointer to INFO bundle which handles this link map entry.
 */
const lindev_table_info_bundle_t * lindev_table_info_conv_to_dometic(
    const lindev_table_config_t * lindev_table,
    const lindev_table_info_link_map_entry_t * link_map_entry,
    const void * const * ddm_data,
    const size_t * ddm_data_size);

/**
 * @brief       Stuff LINDEV Table INFO bundle into LIN INFO frame
 *
 * @param       lindev_table LINDEV Table configuration structure instance
 * @param       info_bundle Pointer to info_bundle which was obtained by
 *              a call to @ref lindev_table_info_conv_to_dometic function.
 * @param [out] frame_data Pointer to LIN frame data buffer where to put stuff
 *              data.
 * @param [out] initial_frame_id Pointer to variable which will contain the LIN
 *              frame identifier associated with the DDM parameter.
 */
void lindev_table_info_bundle_stuff_to_frame(
    const lindev_table_config_t * lindev_table,
    const lindev_table_info_bundle_t * info_bundle,
    uint8_t * frame_data,
    uint_fast8_t * initial_frame_id);

/**
 * @brief       Request sleep in all synchronization states
 *
 * This function is called when LIN goes into sleep and we need to forget current synchronization
 * states.
 *
 * @param       lindev_table LINDEV Table configuration structure instance.
 * @param       lindev_table_state Pointer to lindev table state structure.
 */
void lindev_table_request_sleep(
    const lindev_table_config_t * lindev_table,
    lindev_table_state_t * lindev_table_state);

#endif /* LINDEV_TABLE_H_ */
/** @} */
