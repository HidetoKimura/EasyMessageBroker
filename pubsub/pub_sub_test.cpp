#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <string.h>

#include <string>
#include <memory>
#include <iostream>
#include <vector>

#include "ez_stream.h"
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

static void usage(const char* name )
{
    std::cout << "Usage:" << std::endl ;
    std::cout << "$ ./pubsub " << name << std::endl ;
    std::cout << "$ echo \"pub <topic> <message>\" > " << name << std::endl ;
    std::cout << "$ echo \"sub <topic> \" > " << name << std::endl ;
    std::cout << "$ echo \"unsub <id> \" > " << name << std::endl ;
}

static int create_fifo_fd(const char *name)
{
    int fd;

    unlink(name);
   
    if(0 != mkfifo(name, 0666)) {
        perror("can't create fifo file");
        return -1;
    }
    
    fd = open(name, O_RDONLY);
    if(fd == -1){
        perror("can't open fifo file");
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[])
{
    std::unique_ptr<PubSub> pubsub = std::make_unique<PubSub>("/tmp/test");

    if( argc != 2)
    {
        usage("<fifo name>");
        return -1;
    }
    else 
    {
        usage(argv[1]);
    }

    auto ret = pubsub->connect();
    if(ret < 0) {
        std::cout << "connect failed:" << std::endl;
        return -1;
    }

    int fd = create_fifo_fd(argv[1]);
    if(fd < 0)
    {
        std::cout << "create fifo failed:" << std::endl;
        return -1;
    }

    std::cout << "fd = " << fd << std::endl;

    EventLoopItem item;
    item.fd = fd;
    item.dispatch = [&](int fd) -> void
    {
        char buf[4096];
        int  len;
        len = read(fd, buf, sizeof(buf)-1);
        if(len < 0 ) {
            return;
        }

        std::string  s = (char*)buf;
        auto list = std::vector<std::string>();

        auto offset = std::string::size_type(0);
        do {
            auto pos = s.find(' ', offset); 
            if(pos == std::string::npos) {
                list.push_back(s.substr(offset));
                break;
            }
            list.push_back(s.substr(offset, pos - offset));
            offset = pos + 1;
        } while(1);

        if(list.size() == 2 && list[0] == "sub")
        {
            std::unique_ptr<SubscribeHandler> handler = std::make_unique<MySubscribeHandler>();
            auto id = pubsub->subscribe(list[1], std::move(handler));
            std::cout << "sub id = " << id << std::endl;
        }
        if(list.size() == 3 && list[0] == "pub")
        {
            std::unique_ptr<SubscribeHandler> handler = std::make_unique<MySubscribeHandler>();
            pubsub->publish(list[1], (void*)list[2].c_str(), list[2].size() + 1);
        }
        if(list.size() == 2 && list[0] == "unsub")
        {
            std::unique_ptr<SubscribeHandler> handler = std::make_unique<MySubscribeHandler>();
            
            pubsub->unsubscribe(std::stoi(list[1].c_str()));
        }

        return;
    };
    
    pubsub->add_loop_item(item);
    
    pubsub->event_loop();

    return 0;
}

