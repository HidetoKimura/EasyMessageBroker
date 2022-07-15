#ifndef __BROKER_H__
#define __BROKER_H__

#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <memory>

#include <ez_stream.h>

using namespace std;

class BrokerImpl;

class Broker
{
    public:
        Broker(std::string broker_id);
        ~Broker();

        int32_t listen(void);

        void run(void);
        void stop(void);

        void add_event(EventLoopItem &item);
        void del_event(int fd); 

    private:
        std::unique_ptr<BrokerImpl> m_impl;
};

#endif
