#ifndef __BROKER_H__
#define __BROKER_H__

#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <memory>

using namespace std;

class BrokerImpl;

class Broker
{
    public:
        Broker(std::string broker_id);
        ~Broker();

        int32_t listen(void);

        void event_loop(void);

        void dispatch(int fd);
        int  get_fd(void);

    private:
        std::unique_ptr<BrokerImpl> m_impl;
};

#endif
