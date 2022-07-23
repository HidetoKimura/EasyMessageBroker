#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <thread>
#include <future>

#include <unistd.h>
#include <string.h>

#include <string>
#include <memory>
#include <iostream>
#include <vector>

#include "ez_log.h"
#include "emb.h"

using namespace emb;
using namespace ez::stream;


bool g_running = true;
std::shared_ptr<PubSub> pubsub = {};

void test_thread_1()
{
    emb_id_t sub_id= -1;
    
    std::promise<void> p_sub;
    std::future<void> f_sub = p_sub.get_future();

    pubsub->subscribe("/singal/power", 
        [](std::string topic, void *buf, int32_t len)
        { 
            std::cout << "on_subscribe:" << (char*)buf << std::endl;
        },
        [&sub_id, &p_sub](emb_id_t id)
        { 
            sub_id = id;
            std::cout << "sub id = " << id << std::endl;
            p_sub.set_value();
        });

    f_sub.wait();

    pubsub->publish("/singal/power", (void*)"power_on", strlen("power_on"));

    pubsub->publish("/singal/power", (void*)"power_off", strlen("power_off"));

    std::promise<void> p_unsub;
    std::future<void> f_unsub = p_unsub.get_future();

    pubsub->unsubscribe(sub_id, 
        [&p_unsub](emb_id_t id)
        { 
            std::cout << "sub id = " << id << std::endl;
            p_unsub.set_value();
        });

    f_unsub.wait();    

    g_running = false;

    pubsub->publish("stop", (void*)"dummy", strlen("dummy"));

    return;
 
}
int main(int argc, char *argv[])
{

    int client_fd = -1;
    
    pubsub = std::make_shared<PubSub>();

    auto ret = pubsub->connect("/tmp/test", 
                [&client_fd](int new_fd)
                { 
                    std::cout << "new client fd = " << new_fd << std::endl; 
                    client_fd = new_fd;                    
                });
    if(ret < 0) {
        std::cout << "connect failed:: " << strerror(errno) << std::endl;
        return -1;
    }
    
    std::thread th1(test_thread_1);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOGE << "epoll_create failed:";
        return -1;
    }

    struct epoll_event ev;

    ev.data.fd = client_fd;
    ev.events = EPOLLIN;

    ret = ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    if (ret < 0) {
        LOGE << "epoll_ctl::EPOLL_CTL_ADD";
        LOGE << ::strerror(errno) ;
        ::close(epoll_fd);
        return -1;
    }


    #define MAX_EVENTS 16
    struct epoll_event events[MAX_EVENTS];

    while(g_running) 
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            LOGE << "epoll_wait";
            break;
        }

        for (int n = 0; n < nfds; n++) 
        {
            if(events[n].data.fd == client_fd) {
                pubsub->read_event(client_fd); 
            }
            // do another process
        }
        
    }

    th1.join();
    return -1;
}


