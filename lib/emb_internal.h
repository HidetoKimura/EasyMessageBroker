#ifndef __EMB_MSG_H__
#define __EMB_MSG_H__

#include <stdint.h>
#include <functional>

#define EMB_ID_NOT_USE    0
#define EMB_ID_BROADCAST  1

namespace emb {

using namespace ez::stream;

typedef enum {
    // Common
    EMB_MSG_TYPE_PUBLISH     = 100,
    // Client -> Broker
    //EMB_MSG_TYPE_CONNECT     = 200,
    EMB_MSG_TYPE_SUBSCRIBE   = 201,
    EMB_MSG_TYPE_UNSUBSCRIBE = 202,
    //EMB_MSG_TYPE_DISCONNECT  = 203,
    //EMB_MSG_TYPE_PINGREQ     = 204,
    // Broker -> Client
    //EMB_MSG_TYPE_CONNACK     = 300,
    EMB_MSG_TYPE_SUBACK      = 301,
    EMB_MSG_TYPE_UNSUBACK    = 302,
    //EMB_MSG_TYPE_PINGRESP    = 304
} emb_msg_type_t; 

#define EMB_MSG_TOPIC_MAX       128
#define EMB_MSG_DATA_MAX        256

typedef struct {
    emb_msg_type_t  type;
    int32_t         len;
} emb_msg_header_t;

typedef struct {
    emb_msg_header_t  header;
    int32_t           topic_len;
    int32_t           data_len;
    int32_t           client_id_len;
    char              topic[EMB_MSG_TOPIC_MAX];
    char              data[EMB_MSG_DATA_MAX];
    emb_id_t          client_id;
} emb_msg_PUBLISH_t;

typedef struct {
    emb_msg_header_t  header;
    int32_t           topic_len;
    char              topic[EMB_MSG_TOPIC_MAX];
} emb_msg_SUBSCRIBE_t;

typedef struct {
    emb_msg_header_t  header;
    int32_t           topic_len;
    int32_t           client_id_len;
    char              topic[EMB_MSG_TOPIC_MAX];
    emb_id_t          client_id;
} emb_msg_SUBACK_t;

typedef struct {
    emb_msg_header_t  header;
    int32_t           client_id_len;
    emb_id_t          client_id;
} emb_msg_UNSUBSCRIBE_t;

typedef struct {
    emb_msg_header_t  header;
    int32_t           client_id_len;
    emb_id_t          client_id;
} emb_msg_UNSUBACK_t;

using EmbHandler = std::function<void(int fd, void* msg)>;

struct EmbCommandItem {
    emb_msg_type_t  type;
    EmbHandler      handler;
};

}
#endif