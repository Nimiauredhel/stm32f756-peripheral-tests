#include "packet_ops.h"

void packet_init(uint8_t *buff)
{
    explicit_bzero(buff, sizeof(buff));
    buff[0] = TEST_PACKET_START_BYTE_VALUE;
    buff[TEST_PACKET_MSG_BYTE_OFFSET] = TESTMSG_NONE;
    buff[TEST_PACKET_STRING_HEAD_OFFSET] = TEST_PACKET_END_BYTE_VALUE;
}
