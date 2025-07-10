#ifndef TEST_PACKET_DEF_H
#define TEST_PACKET_DEF_H

/***
 * Packet Structure:
 * [START(1)][TEST ID(4)[MSG(1)][SELECTION(1)][ITERATIONS(1)][STRLEN(1)][STRING(32)][END(1)]
 * 0         1          5       6             7              8          9           41
 ***/

#define TEST_PACKET_SIZE_BYTES (42)
#define TEST_PACKET_START_BYTE_VALUE (123)
#define TEST_PACKET_END_BYTE_VALUE (0)
#define TEST_PACKET_STR_MAX_LEN (32)

#define TEST_PACKET_ID_BYTE_OFFSET (1)
#define TEST_PACKET_MSG_BYTE_OFFSET (5)
#define TEST_PACKET_SELECTION_BYTE_OFFSET (6)
#define TEST_PACKET_STRING_LEN_OFFSET (8)
#define TEST_PACKET_STRING_HEAD_OFFSET (9)

typedef enum PeripheralTestIdx
{
	TESTIDX_TIMER = 0,
	TESTIDX_UART = 1,
	TESTIDX_SPI = 2,
	TESTIDX_I2C = 3,
	TESTIDX_ADC = 4,
} PeripheralTestIdx_t;

typedef enum TestPacketMsg
{
    TESTMSG_NONE = 0,
    TESTMSG_NEWTEST = 1,
    TESTMSG_ACK = 2,
    TESTMSG_RESULT = 3,
} TestPacketMsg_t;

#endif

