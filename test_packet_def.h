#ifndef TEST_PACKET_DEF_H
#define TEST_PACKET_DEF_H

/***
 * Packet Structures:
 *
 * pairing packet (2 bytes)
 * [START(1)][MSG(1)][END(1)]
 * 0         1       3
 *
 * test message packet (8 bytes)
 * [START(1)][MSG(1)][TEST ID(4)[SELECTION(1)][END(1)]
 * 0         1       2          6             7
 *
 * test request packet (10-160 bytes)
 * [START(1)][MSG(1)][TEST ID(4)[SELECTION(1)][ITERATIONS(1)][STRLEN(1)][STRING(0-150)][END(1)]
 * 0         1       2          6             7              8          9              9-159
 ***/

/***
 * the pre-determined fixed size of "pairing" packets:
 * new test ack, start test request, start test request.
 ***/
#define PAIRING_PACKET_SIZE_BYTES (3)

/***
 * the pre-determined fixed size of "test message" packets:
 * new test ack, start test request, start test request.
 ***/
#define TEST_MSG_PACKET_SIZE_BYTES (8)

/***
 * the absolute minimum size of "test request" packets,
 * which currently only includes "new test request".
 ***/
#define TEST_REQUEST_PACKET_MIN_SIZE_BYTES (10)

/***
 * the absolute maximum size of "test request" packets,
 * which currently only includes "new test request".
 ***/
#define TEST_REQUEST_PACKET_MAX_SIZE_BYTES (160)

/***
 * the pre-determined value of the very first byte,
 * to help filter foreign or malformed packets.
 ***/
#define TEST_PACKET_START_BYTE_VALUE (123)

/***
 * the pre-determined value of the very last byte,
 * to help filter foreign or malformed packets.
 ***/
#define TEST_PACKET_END_BYTE_VALUE (0)

/***
 * the pre-determined absolute maximum length of a test string.
 ***/
#define TEST_PACKET_STR_MAX_LEN (150)

/***
 * the MSG byte describes the purpose and structure of the packet.
 ***/
#define TEST_PACKET_MSG_BYTE_OFFSET (1)

/***
 * the ID byte is used by client and server to associate individual packets with an ongoing conversation.
 * the client selectes the two high bytes in a "new test request",
 * and the server completes the lower two in a "new test acknowledgement".
 * the complete 32 bit number is then used in all following packets.
 ***/
#define TEST_PACKET_ID_BYTE_OFFSET (2)

/***
 * the SELECTION byte is used by the client to inform the server which tests are requested,
 * and by the server in test results to indicate which of the requested tests have passed.
 * the test types are represented by the individual bits, with a 1 indicating that a test
 * has been requested or passed, respectively. it is also used by the server in acknowledgements,
 * to indicate whether a request has been accepted or rejected (1 and 0 respectively).
 * with the exception of a "new test request" packet, it is immediately followed by the END byte.
 ***/
#define TEST_PACKET_SELECTION_BYTE_OFFSET (6)

/***
 * the SELECTION byte is used by the client to inform the server how many test iterations are requested.
 ***/
#define TEST_PACKET_ITERATIONS_BYTE_OFFSET (7)

/***
 * the STRING_LEN byte is used by the client to indicate the length of the following test string.
 ***/
#define TEST_PACKET_STRING_LEN_OFFSET (8)

/***
 * the STRING_HEAD byte is the first byte of the string included in a "new test request" packet.
 * in a "new test request" packet, the final byte of this string is immediately followed by the END byte.
 ***/
#define TEST_PACKET_STRING_HEAD_OFFSET (9)

/***
 * the pre-determined total number of test types available on the test device.
 ***/
#define NUM_POSSIBLE_TESTS (5)

/***
 * the PeripheralTestIdx_t enum values are associated with the SELECTION byte of packets,
 * where they are used to indicate the position (in left-shifts) of the bit associated with a test type.
 * in other contexts they are used as simple array indices for data relevant to the different tests.
 ***/
typedef enum PeripheralTestIdx
{
	TESTIDX_TIMER = 0,
	TESTIDX_UART = 1,
	TESTIDX_SPI = 2,
	TESTIDX_I2C = 3,
	TESTIDX_ADC = 4,
} PeripheralTestIdx_t;

/***
 * the TestPacketMsg_t enum values are associated with the MSG byte of packets,
 * as detailed above.
 * values 0 and 1 are intentionally unused for two reasons:
 * 1. these are common values and excluding them will help filter out invalid packets.
 * 2. the low bit can be used to quickly filter between two classes of packet types.
 * following from the second point, all values associated with CLIENT originating packet types are EVEN,
 * while the values associated with their closest corresponding SERVER originating packet types are ODD.
 ***/
typedef enum TestPacketMsg
{
    TESTMSG_FLAG_CLIENT = 0,
    TESTMSG_FLAG_SERVER = 1,

    TESTMSG_TEST_NEW_REQUEST = 2,
    TESTMSG_TEST_NEW_ACK = 3,

    TESTMSG_TEST_START_REQUEST = 4,
    TESTMSG_TEST_START_ACK = 5,

    TESTMSG_TEST_OVER_ACK = 6,
    TESTMSG_TEST_OVER_RESULTS = 7,

    TESTMSG_PAIRING_PROBE = 8,
    TESTMSG_PAIRING_BEACON = 9,
} TestPacketMsg_t;

#endif

