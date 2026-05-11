/**
 * @file lin_common.h
 * @author Borjan Bozhinovski (borjan.bozhinovski@qinshift.com)
 * @brief LIN helper macros and data types used by LIN server and LIN device
 * @date 2024-01-11
 */

#ifndef LIN_COMMON__
#define LIN_COMMON__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief LIN helper macros used to define LIN frame bytes position and values
 */
#define LIN_BREAK_FIELD_BYTE        (0u)
#define LIN_BREAK_FIELD_DATA        (0x00u)
#define LIN_SYNC_FIELD_BYTE         (1u)
#define LIN_SYNC_FIELD_DATA         (0x55u)
#define LIN_PID_FIELD_BYTE          (2u)
#define LIN_PID_FIELD_FRAME_ID_MASK (0x3fu)
#define LIN_PID_FIELD_FRAME_ID_POS  (0u)
#define LIN_PID_FIELD_PARITY_MASK   (0xc0u)
#define LIN_PID_FIELD_PARITY_POS    (6u)

#define LIN_EXTRACT_FRAME_ID_FROM_PID_FIELD(pid) \
    (((pid) & LIN_PID_FIELD_FRAME_ID_MASK) >> LIN_PID_FIELD_FRAME_ID_POS)
#define LIN_EXTRACT_PARITY_FROM_PID_FIELD(pid) \
    (((pid) & LIN_PID_FIELD_PARITY_MASK) >> LIN_PID_FIELD_PARITY_POS)

/**
 * @brief LIN transport layer helper macros
 */
#define LIN_PCI_6 (0x06u)
#define LIN_PCI_5 (0x05u)

/**
 * @brief LIN Wildcards
 * @note LIN Specification 2.2A - 4.2.1.1
 */
#define LIN_NAD_WILDCARD         (0x7F)
#define LIN_SUPPLIER_ID_WILDCARD (0x7FFF)
#define LIN_FUNCTION_ID_WILDCARD (0xFFFF)

/**
 * @brief LIN node configuration and identification helper macros
 */
#define LIN_SID_ASSIGN_NAD            (0xb0u)
#define LIN_SID_READ_BY_IDENTIFIER    (0xb2u)
#define LIN_SID_ASSIGN_FRAME_ID_RANGE (0xb7u)

#define LIN_SID_READ_BY_IDENTIFIER_PRODUCT_IDENTIFICATION (0x00u)
#define LIN_SID_READ_BY_IDENTIFIER_SERIAL_NUMBER          (0x01u)

/**
 * @brief LIN node addresses (NAD)
 * @note LIN Specifciation 2.2A - 4.2.3.2
 */
#define LIN_NAD_GO_TO_SLEEP_COMMAND (0x00u)

#define LIN_IS_NAD_GO_TO_SLEEP_COMMAND(nad)         ((nad) == LIN_NAD_GO_TO_SLEEP_COMMAND)
#define LIN_IS_NAD_SLAVE_NODE_ADDRESS(nad)          (((nad) >= 0x01) && ((nad) <= 0x7D))
#define LIN_IS_NAD_FUNCTIONAL_NODE_ADDRESS(nad)     ((nad) == 0x7E)
#define LIN_IS_NAD_SLAVE_NODE_ADDRESS_BRODCAST(nad) ((nad) == LIN_NAD_WILDCARD)
#define LIN_IS_NAD_USER_DEFINED(nad)                (((nad) >= 0x80) && ((nad) <= 0xFF))

/**
 * @brief LIN diagnostic frames helper macros
 */
#define LIN_FRAME_ID_DIAG_REQUEST           (0x3cu)
#define LIN_FRAME_ID_DIAG_RESPONSE          (0x3du)
#define LIN_FRAME_PID_DIAG_REQUEST          (0x3cu)
#define LIN_FRAME_PID_DIAG_RESPONSE         (0x7du)
#define LIN_IS_FRAME_ID_SIGNAL(id)          ((id) < LIN_FRAME_ID_DIAG_REQUEST)
#define LIN_IS_FRAME_ID_DIAGNOSTIC(id)      (((id) == LIN_FRAME_ID_DIAG_REQUEST) || ((id) == LIN_FRAME_ID_DIAG_RESPONSE))
#define LIN_IS_FRAME_ID_FOR_FUTURE(id)      (((id) == 0x3eu) || ((id) == 0x3fu))
#define LIN_IS_FRAME_PID_DIAG_REQUEST(pid)  ((pid) == LIN_FRAME_PID_DIAG_REQUEST)
#define LIN_IS_FRAME_PID_DIAG_RESPONSE(pid) ((pid) == LIN_FRAME_PID_DIAG_RESPONSE)
#define LIN_IS_FRAME_PID_DIAGNOSTIC(pid)    ((LIN_IS_FRAME_PID_DIAG_REQUEST(pid)) || (LIN_IS_FRAME_PID_DIAG_RESPONSE(pid)))
#define LIN_CALCULATE_RSID(sid)             ((sid) + 0x40)

/**
 * @brief   LIN helper macro used to extract bits from data frame byte
 */
#define LIN_EXTRACT_BIT(data, bit_pos) (((data) >> (bit_pos)) & 0x1u)

/**
 * @brief   LIN helper macro used to declare structure attribute of packed data
 */
#define LIN_PACKED __attribute__((packed))

/**
 * @brief       Depth of ISR-to-task event queue
 *
 * LIN helper macro for task queue depth used for transfering RX readings from
 * the interrupt routine. Has to be able to carry at least one frame lenght (11 bytes) not
 * taking into considuration the break signal, as for some platforms an RX interrupt can be
 * triggered after each byte received(as it is the case for esp32).
 */
#define LIN_EVENT_QUEUE_DEPTH (LIN_FRAME_RAW_LEN - 1)

/**
 * @brief       Force LIN UART ISR in IRAM/FLASH
 *
 * By default, we want LIN/UART ISR to be in IRAM, but if there is a need to save IRAM the
 * interrupt service routine can be forced to be in flash. When this is needed define the macro
 * @ref LIN_IS_ISR_IN_IRAM to zero in project configuration header.
 *
 * Note that, when forcing LIN/UART ISR to be in flash, a speed penalty may occur and the LIN
 * communication will be less reliable in that case.
 */
#ifndef LIN_IS_ISR_IN_IRAM
#define LIN_IS_ISR_IN_IRAM 1
#endif

/**
 * @brief       Size of LIN frame start (0x00, 0x55, identifier) in bytes.
 *
 * @note        Macro definition is for internal LIN module usage.
 */
#define LIN_FRAME_START_LEN (3)

/**
 * @brief       Size of LIN frame data (payload) in bytes.
 *
 * @note        Macro definition is for internal LIN module usage.
 */
#define LIN_FRAME_DATA_LEN (8)

/**
 * @brief       Size of LIN checksum field in bytes.
 *
 * @note        Macro definition is for internal LIN module usage.
 */
#define LIN_FRAME_CHECKSUM_LEN (1)

/**
 * @brief       Total size of raw LIN frame in bytes.
 *
 * @note        Macro definition is for internal LIN module usage.
 */
#define LIN_FRAME_RAW_LEN (LIN_FRAME_START_LEN + LIN_FRAME_DATA_LEN + LIN_FRAME_CHECKSUM_LEN)

/**
 * @brief       Size of INFO request frame
 *
 * @note        Macro definition is for internal LIN module usage.
 */
#define LIN_FRAME_INFO_REQUEST_LEN (LIN_FRAME_DATA_LEN + LIN_FRAME_CHECKSUM_LEN)

/**
 * @brief       Position of checkum filed
 *
 * @note        Macro definition is for internal LIN module usage.
 */
#define LIN_CHECKSUM_FIELD_BYTE (LIN_FRAME_START_LEN + LIN_FRAME_DATA_LEN)

/**
 * @brief 		Device configuration used for Diagnostic LIN frames
 *
 * This structure describes relevant data that needs to be supplied to LIN master as part of
 * diagnostic response LIN frame.
 *
 * For details what these fields mean in diagnostic messages refer to LIN v2.2A specification.
 */
typedef struct lin_device_config_data
{
    uint8_t nad;                      //!< NAD of a device
    uint16_t supplier_id;             //!< Supplier ID
    uint16_t function_id;             //!< Function ID, used when a device has only one Function ID (function_variant_ids_size is zero)
    uint8_t variant_id;               //!< Variant ID, used when a device has only one Variant ID (function_variant_ids_size is zero)
    uint32_t serial_number;           //!< Serial number
    const uint16_t *function_ids;     //!< Function IDs array, contains an array of Functions IDs. This array must be defined when function_variant_ids_size is different than zero.
    const uint8_t *variant_ids;       //!< Variant IDs array, contains an array of Variants IDs. This array must be defined when function_variant_ids_size is different than zero.
    size_t function_variant_ids_size; //!< Size of array, use ELEMENTS macro. When this member is zero, `function_id` and `variant_id` members define the Function ID and Variant ID.
} lin_device_config_data_t;

/**
 * @brief       Frame types supported by LIN module.
 *
 * Types @ref LIN_CONTROL_FRAME and @ref LIN_INFO_FRAME correspond to classic LIN frame definition.
 *
 * The type @ref LIN_INFO_EVENT_FRAME is like @ref LIN_INFO_FRAME but LIN will respond only if
 * `send_update` argument is set when setting frame data with call to @ref lindev_set_frame_data.
 *
 * LIN specification supports the following frames (depending on the behaviour):
 * - Unconditional
 * - Event triggered
 * - Sporadic
 *
 * All the listed frames here are `unconditional`.
 */
typedef enum lin_frame_type
{
    LIN_CONTROL_FRAME = 0,  //!< Control frame (master request to write)
    LIN_INFO_FRAME,         //!< Info frame (master request to read)
    LIN_DIAG_REQ_FRAME,     //!< Diagnostic frame request (master request to send request)
    LIN_DIAG_RES_FRAME,     //!< Diagnostic frame response (slave response to master)
} lin_frame_type_t;

/**
 * @brief Protocol Control Information(PCI) types
 */
typedef enum lin_pci_type
{
    LIN_PCI_TYPE_SF = 0,  //!< Single frame
    LIN_PCI_TYPE_FF = 1,  //!< First frame
    LIN_PCI_TYPE_CF = 2,  //!< Consecutive frame
} lin_pci_type_t;

/**
 * @brief PCI Single Frame(SF) data frame definition
 */
typedef struct lin_packet_data_unit_sf
{
    union
    {
        uint8_t sid;   //!< Service Identifier
        uint8_t rsid;  //!< Response Service Identifier
    };

    uint8_t d1;
    uint8_t d2;
    uint8_t d3;
    uint8_t d4;
    uint8_t d5;
} lin_packet_data_unit_sf_t;

/**
 * @brief PCI generic data frame header definition
 *
 * Shared between all PDU types reflected by the PCI types (SF, FF, CF). It gives information
 * whether the transported message fits into one single or multiple PDUs(Packet Data Unit(s)).
 *
 * For details what these fields mean in diagnostic messages refer to LIN v2.2A specification.
 */
typedef struct lin_packet_data_unit_header
{
    uint8_t nad;
    union
    {
        struct
        {
            uint8_t pci_info : 4;  //!< Additional information
            uint8_t pci_type : 4;  //!< PCI type (SF, FF, CF)
        };
        uint8_t pci;
    };
} lin_packet_data_unit_header_t;

/**
 * @brief PCI generic data frame definition
 *
 * Shared between all PDU types reflected by the PCI types (SF, FF, CF). It gives information
 * whether the transported message fits into one single or multiple PDUs(Packet Data Unit(s)).
 *
 *  @note Only PCI SF is implemented at this point. If needed in the future, the union can be
 * extended with packet_data_unit_ff_t (First Frame) and paket_data_unit_cf_t (Consecutive Frame)
 *
 * For details what these fields mean in diagnostic messages refer to LIN v2.2A specification.
 */
typedef struct lin_packet_data_unit_data
{
    union
    {
        lin_packet_data_unit_sf_t pdu_sf;
    };
} lin_packet_data_unit_data_t;

/**
 * @brief  Diagnostic frame definition type
 *
 * For details what these fields mean in diagnostic messages refer to
 * LIN v2.2A specification, LIN Transport layer (3.2.1.1 Overview)
 */
typedef struct lin_diagnostic_frame
{
    lin_packet_data_unit_header_t pdu_header;
    lin_packet_data_unit_data_t pdu_data;
} lin_diagnostic_frame_t;

uint_fast8_t lin_calculate_parity(uint_fast8_t id);
uint8_t lin_calculate_checksum(uint_fast8_t id, const uint8_t *data, size_t len);

#endif  // LIN_COMMON__
