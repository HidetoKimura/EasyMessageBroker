#ifndef __EMB_H__
#define __EMB_H__

//#include <string>
#include <memory>
//#include <vector>

//#include <stdint.h>
//#include <unistd.h>

#include "ez_stream.h"
//#include "emb_msg.h"

namespace emb {

using namespace ez::stream;

typedef uint32_t emb_id_t;

using ConnectionHandler = std::function<void(int new_fd)>;

class BrokerImpl;

class Broker
{
    public:
        Broker();
        ~Broker();

        int32_t listen(std::string broker_id, ConnectionHandler on_listen = nullptr, ConnectionHandler on_conn = nullptr);

        void run(void);
        void stop(void);

        void read_event(int fd);

        void add_event(EventLoopItem &item);
        void del_event(int fd); 

    private:
        std::unique_ptr<BrokerImpl> m_impl;
};


using namespace std;

class PubSubImpl;

/*
class SubscribeHandler
{
    public:
        virtual void handleMessage(std::string topic, void *buf, int32_t len) = 0;
};
*/

using SubscribeHandler = std::function<void(std::string topic, void *buf, int32_t len)>;

using ResultHandler = std::function<void(emb_id_t id)>;

class PubSub 
{
    public:
        PubSub();
        ~PubSub();

        int32_t connect(std::string broker_id, ConnectionHandler on_conn = nullptr);

        void subscribe(std::string topic, SubscribeHandler on_subsribe, ResultHandler on_result);
        void unsubscribe(emb_id_t subscription_id, ResultHandler on_result);
        void publish(std::string topic, void* buf , int32_t len);
        void publish(std::string topic, void* buf , int32_t len, emb_id_t to_id);

        void run(void);
        void stop(void);

        void read_event(int fd);

        void add_event(EventLoopItem &item);
        void del_event(int fd); 

    private:
        std::unique_ptr<PubSubImpl> m_impl;
};

}
#endif
