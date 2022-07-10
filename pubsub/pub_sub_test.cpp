#include <unistd.h>
#include <string.h>

#include <string>
#include <memory>
#include <iostream>

#include "pub_sub.h"

using namespace std;

class MySubscribeHandler : public SubscribeHandler
{
    public:
        virtual void handleMessage(std::string topic, void *buf, int32_t len)
        {
            std::cout << " handleMessage : " << (char*)buf << std::endl;
        }
};

static void usage()
{
    std::cout << "Usage:" << std::endl ;
    std::cout << "./pubsub pub <topic> <message>" << std::endl ;
    std::cout << "./pubsub sub <topic>" << std::endl ;
}
int main(int argc, char *argv[])
{
    std::unique_ptr<PubSub> pubsub = std::make_unique<PubSub>("/tmp/test");
    std::shared_ptr<SubscribeHandler> handler = std::make_shared<MySubscribeHandler>();

    if (argc == 4 && 0 == strcmp(argv[1], "pub"))
    {
        std::string topic = argv[2];
        std::string msg = argv[3];
        pubsub->publish(topic, (void*)msg.c_str(), msg.capacity());
    }
    else if (argc == 3 && 0 == strcmp(argv[1], "sub")) {
        std::string topic = argv[2];
        pubsub->subscribe(topic, handler);
        pubsub->event_loop();
    }
    else {
        usage();
        return -1;
    }

    return 0;
}
