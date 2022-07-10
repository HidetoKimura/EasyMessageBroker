#ifndef __EMB_MSG_H__
#define __EMB_MSG_H__

#include <stdint.h>

#define EMB_MSG_HEAD_SIGN                    0x23424D45 // EMB#

// Common
#define EMB_MSG_COMMAND_PUBLISH             "PUBL"

// Client -> Broker
#define EMB_MSG_COMMAND_CONNECT             "CONN"
#define EMB_MSG_COMMAND_SUBSCRIBE           "SBSC"
#define EMB_MSG_COMMAND_UNSUBSCRIBE         "UNSB"
#define EMB_MSG_COMMAND_DISCONNECT          "DCON"
#define EMB_MSG_COMMAND_PINGREQ             "PNRQ"

// Broker -> Client
#define EMB_MSG_COMMAND_CONNACK             "CNAK"
#define EMB_MSG_COMMAND_SUBACK              "SBAK"
#define EMB_MSG_COMMAND_UNSUBACK            "USAK"
#define EMB_MSG_COMMAND_PINGRESP            "PNRS"

typedef struct {
    uint32_t    head_sign;
    char        command[4];
    uint32_t    topic_len;
    uint32_t    data_len;
} emb_msg_t;

#endif