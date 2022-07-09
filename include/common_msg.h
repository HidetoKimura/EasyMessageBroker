#ifndef __COMMON_MSG_H__
#define __COMMON_MSG_H__

#include <stdint.h>

#define COMMON_MSG_HEAD_SIGN                    0x11223344

#define COMMON_MSG_COMMAND_SUBSCRIBE_REQ       "RSUB"
#define COMMON_MSG_COMMAND_SUBSCRIBE_ANS       "ASUB"
#define COMMON_MSG_COMMAND_PUBLISH_REQ         "RPUB"
#define COMMON_MSG_COMMAND_PUBLISH_NTY         "NPUB"

typedef struct {
    uint32_t    head_sign;
    uint32_t    length;
    char            command[4];
    uint32_t    reserve;
} common_msg_t;

#endif