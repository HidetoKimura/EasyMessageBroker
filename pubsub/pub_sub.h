#ifndef __PUB_SUB_H__
#define __PUB_SUB_H__

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <memory>

#include "emb_msg.h"
#include "ezlog.h"

using namespace std;

class PubSubImpl;

class SubscribeHandler
{
    public:
        virtual void handleMessage(std::string topic, void *buf, int32_t len) = 0;
};

class PubSub
{
    public:
        PubSub(std::string broker_id);
        ~PubSub();

        emb_id_t subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler);
        void unsubscribe(emb_id_t client_id);
        void publish(std::string topic, void* buf , int32_t len);

        void event_loop(int count);

    private:
        std::unique_ptr<PubSubImpl> m_impl;
};

#endif
