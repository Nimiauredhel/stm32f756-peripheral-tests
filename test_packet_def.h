#ifndef TEST_PACKET_DEF_H
#define TEST_PACKET_DEF_H

/**
 * @file test_packet_def.h
 * @brief
 * Defines common packet structures and other key variables
 * shared between the peripheral testing servers
 * and the PC clients issuing them test requests.
 * @details
 * There are broadly three packet types.
 * * Pairing packets are broadcast over the network to help clients and servers find one another.
 * They are very small as the necessary information is already carried by the IP header.
 @verbatim
 |   Pairing Packet  |START(1)|MSG(1)|END(1)|
 |      3 bytes      |0       |1     |2     |
 @endverbatim
 * * Message packets are slightly larger and include the Test ID and Selection fields,
 * to carry messages pertaining to an existing test request.
 @verbatim
 |Test Message Packet|START(1)|MSG(1)|TEST ID(4)|SELECTION(1)|END(1)|
 |      8 bytes      |0       |1     |2         |6           |7     |
 @endverbatim
 * * Request packets are only used for requesting an entirely new test,
 * and are the largest as they carry a test string of variable length.
 @verbatim
 |Test Request Packet|START(1)|MSG(1)|TEST ID(4)|SELECTION(1)|ITERATIONS(1)|STRLEN(1)|STRING(0-150)|END(1)|
 |   10-160 bytes    |0       |1     |2         |6           | 7           |8        |9            |9-159 |
 @endverbatim
 * 
 * "Why all these cumbersome offsets? Why not use a struct?"
 *
 * I wanted to try something different and maybe learn something from the experience, and I definitely learned not to try this again.
 */

/**
 * @brief The pre-determined 'receive' port to be used by compliant test clients.
 */
#define CLIENT_PORT (34567)
/**
 * @brief The pre-determined 'receive' port to be used by compliant test servers.
 */
#define SERVER_PORT (45678)

/**
 * @brief The pre-determined fixed size of "pairing" packets (@ref TESTMSG_PAIRING_BEACON, @ref TESTMSG_PAIRING_PROBE).
 */
#define PAIRING_PACKET_SIZE_BYTES (3)

/**
 * @brief The pre-determined fixed size of "test message" packets (@ref TESTMSG_TEST_NEW_ACK, @ref TESTMSG_TEST_START_ACK, @ref TESTMSG_TEST_START_REQUEST).
 */
#define TEST_MSG_PACKET_SIZE_BYTES (8)

/**
 * @brief The absolute minimum size of "test request" packets, which currently only includes @ref TESTMSG_TEST_NEW_REQUEST.
 */
#define TEST_REQUEST_PACKET_MIN_SIZE_BYTES (10)

/**
 * @brief The absolute maximum size of "test request" packets, which currently only includes "new test request".
 */
#define TEST_REQUEST_PACKET_MAX_SIZE_BYTES (160)

/**
 * @brief The pre-determined value of the very first byte, to help filter foreign or malformed packets.
 */
#define TEST_PACKET_START_BYTE_VALUE (123)

/**
 * @brief The pre-determined value of the very last byte, to help filter foreign or malformed packets.
 */
#define TEST_PACKET_END_BYTE_VALUE (0)

/**
 * @brief The pre-determined absolute maximum length of a test string.
 */
#define TEST_PACKET_STR_MAX_LEN (150)

/**
 * @brief the 'offset' (array index) of the 'message' byte in a packet.
 * @details
 * The MSG byte describes the purpose and structure of the packet, and is a @ref TestPacketMsg_t value.
 */
#define TEST_PACKET_MSG_BYTE_OFFSET (1)

/**
 * @brief the 'offset' (array index) of the test ID byte in a packet.
 * @details
 * The ID byte is used by client and server to associate individual packets with an ongoing conversation.
 * the client selectes the two high bytes in a "new test request",
 * and the server completes the lower two in a "new test acknowledgement".
 * the complete 32 bit number is then used in all following packets.
 */
#define TEST_PACKET_ID_BYTE_OFFSET (2)

/**
 * @brief the 'offset' (array index) of the 'selection' byte in a packet.
 * @details
 * The SELECTION byte is used by the client to inform the server which tests are requested,
 * and by the server in test results to indicate which of the requested tests have passed.
 * the test types are represented by the individual bits, with a 1 indicating that a test
 * has been requested or passed, respectively. it is also used by the server in acknowledgements,
 * to indicate whether a request has been accepted or rejected (1 and 0 respectively).
 * with the exception of a "new test request" packet, it is immediately followed by the END byte.
 */
#define TEST_PACKET_SELECTION_BYTE_OFFSET (6)

/**
 * @brief the 'offset' (array index) of the 'test iterations' byte in a packet.
 * @details
 * The ITERATIONS byte is used by the client to inform the server how many test iterations are requested.
 */
#define TEST_PACKET_ITERATIONS_BYTE_OFFSET (7)

/**
 * @brief the 'offset' (array index) of the 'string length' byte in a packet.
 * @details
 * The STRING_LEN byte is used by the client to indicate the length of the following test string.
 */
#define TEST_PACKET_STRING_LEN_OFFSET (8)

/**
 * @brief the 'offset' (array index) of the 'string head' byte in a packet.
 * @details
 * The STRING_HEAD byte is the first byte of the string included in a "new test request" packet.
 * in a "new test request" packet, the final byte of this string is immediately followed by the END byte.
 */
#define TEST_PACKET_STRING_HEAD_OFFSET (9)

/**
 * @brief The pre-determined total number of test types available on the test device.
 */
#define NUM_POSSIBLE_TESTS (5)

/**
 * @brief Numerical values associated with individual peripheral tests.
 * @details
 * The PeripheralTestIdx_t enum values are associated with the SELECTION byte of packets,
 * where they are used to indicate the position (in left-shifts) of the bit associated with a test type.
 * They are also used in other contexts as array indices for data relevant to the different tests.
 */
typedef enum PeripheralTestIdx
{
	TESTIDX_TIMER = 0,
	TESTIDX_UART = 1,
	TESTIDX_SPI = 2,
	TESTIDX_I2C = 3,
	TESTIDX_ADC = 4,
} PeripheralTestIdx_t;

/**
 * @brief Numerical values associated with different packet types.
 * @details
 * The TestPacketMsg_t enum values are associated with the MSG byte of packets, as detailed below.
 *
 * Values 0 and 1 are intentionally left unused, for the following reasons:
 * -# These are common values and excluding them will help filter out invalid packets.
 * -# The low bit can be used to quickly filter between two classes of packet types.
 *
 * Following from the second point, all values associated with CLIENT originating packet types are EVEN,
 * while the values associated with their closest corresponding SERVER originating packet types are ODD.
 */
typedef enum TestPacketMsg
{
    /// Unused, reserved
    TESTMSG_FLAG_CLIENT = 0,
    /// Unused, reserved
    TESTMSG_FLAG_SERVER = 1,
    /// Client requests new test
    TESTMSG_TEST_NEW_REQUEST = 2,
    /// Server acknowledges new test request
    TESTMSG_TEST_NEW_ACK = 3,
    /// (unimplemented) Client requests test be started.
    TESTMSG_TEST_START_REQUEST = 4,
    /// (unimplemented) Server acknowledges test has started.
    TESTMSG_TEST_START_ACK = 5,
    /// Client acknowledges receiving test results
    TESTMSG_TEST_OVER_ACK = 6,
    /// Server: test over, results attached
    TESTMSG_TEST_OVER_RESULTS = 7,
    /// Client probe for auto-pairing
    TESTMSG_PAIRING_PROBE = 8,
    /// Server beacon for auto-pairing
    TESTMSG_PAIRING_BEACON = 9,
} TestPacketMsg_t;

#endif

