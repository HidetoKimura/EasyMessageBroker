#ifndef __SUB_H__
#define __SUB_H__

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <memory>

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

        void subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler);
        void publish(std::string topic, void* buf , int32_t len);

        void event_loop();

    private:
        std::unique_ptr<PubSubImpl> m_impl;
};

#endif
