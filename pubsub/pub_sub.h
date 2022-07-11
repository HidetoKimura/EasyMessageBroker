#ifndef __PUB_SUB_H__
#define __PUB_SUB_H__

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <memory>

#include "ez_stream.h"
#include "emb_msg.h"

using namespace std;

class PubSubImpl;

class SubscribeHandler
{
    public:
        virtual void handleMessage(std::string topic, void *buf, int32_t len) = 0;
};

class PubSub : public EventLoop, public SocketStream
{
    public:
        PubSub(std::string broker_id);
        ~PubSub();

        int32_t dial(void);

        emb_id_t subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler);
        void unsubscribe(emb_id_t client_id);
        void publish(std::string topic, void* buf , int32_t len);

        void dispatch(int fd);
        int  get_fd(void);

    private:
        std::unique_ptr<PubSubImpl> m_impl;
};

#endif
