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
#include "ez_log.h"
#include "emb.h"

using namespace emb;
using namespace ez::stream;

/*
class MySubscribeHandler : public SubscribeHandler
{
    public:
        virtual void handleMessage(std::string topic, void *buf, int32_t len)
        {
            std::cout << this << ":handleMessage():" << (char*)buf << std::endl;
        }
};
*/

static void usage(const char* name )
{
    std::cout << std::endl ;
    std::cout << "Usage:" << std::endl ;
    std::cout << "$ ./pubsub " << name << " &" << std::endl ;
    std::cout << "$ echo \"sub <topic>\" > " << name << std::endl ;
    std::cout << "$ echo \"pub <topic> <message>\" > " << name << std::endl ;
    std::cout << "$ echo \"pubid <topic> <message> <id>\" > " << name << std::endl ;
    std::cout << "$ echo \"unsub <id>\" > " << name << std::endl ;
    std::cout << "$ echo \"stop\" > " << name << std::endl ;
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

static void parse_command(char* s, std::function<void(int argc, char* argv[])> handler)
{
    #define     DELIM   " \t\n\r"
    #define     ARG_MAX 32
    int32_t     argc;
    char*       argv[ARG_MAX];
    uint32_t    i;
    char*       tok;

    tok = strtok(s, DELIM); 
    if(tok == NULL) {
        return;
    }
        
    argv[0] = tok;
    for(i = 1; i < (sizeof(argv)/sizeof(argv[0])); i++) {
        if((tok = strtok(NULL, DELIM)) == NULL) break;
            argv[i] = tok;
    }
    argc = i;
    
    handler(argc, argv);

    return;
}


int main(int argc, char *argv[])
{
    std::unique_ptr<PubSub> pubsub = std::make_unique<PubSub>();

    if( argc != 2)
    {
        usage("<fifo name>");
        return -1;
    }
    else 
    {
        usage(argv[1]);
    }

    auto ret = pubsub->connect("/tmp/test", 
                [](int new_fd){ std::cout << "new client fd = " << new_fd << std::endl; });
    if(ret < 0) {
        std::cout << "connect failed:: " << strerror(errno) << std::endl;
        return -1;
    }

    int fd = create_fifo_fd(argv[1]);
    if(fd < 0)
    {
        std::cout << "create fifo failed:" << std::endl;
        return -1;
    }

    EventLoopItem item;
    item.fd = fd;
    item.dispatch = [&](int fd) -> void
    {
        #define MAX_SIZE    256
        char buf[MAX_SIZE];
        int  len;
        bzero(buf, sizeof(buf));

        len = read(fd, buf, sizeof(buf)-1);
        if(len < 0 ) 
        {
            perror("read error:");
            return;
        }
        else if (len == 0)
        {
            sleep(1);
            return;
        }

        auto handler = [&](int argc, char* argv[]) -> void {
            if(argc == 2 && strcmp(argv[0], "sub") == 0)
            {
                pubsub->subscribe(std::string(argv[1]), 
                    [](std::string topic, void *buf, int32_t len){ std::cout << "on_subscribe:" << (char*)buf << std::endl;},
                    [](emb_id_t id){ std::cout << "sub id = " << id << std::endl;});
                
            }
            else if(argc == 3 && strcmp(argv[0], "pub") == 0)
            {
                pubsub->publish(std::string(argv[1]), (void*)argv[2], strlen(argv[2]));
            }
            else if(argc == 4 && strcmp(argv[0], "pubid") == 0)
            {
                pubsub->publish(std::string(argv[1]), (void*)argv[2], strlen(argv[2]), std::stoi(argv[3]));
            }
            else if(argc == 2 && strcmp(argv[0], "unsub") == 0)
            {
                pubsub->unsubscribe(std::stoi(argv[1]), [](emb_id_t id){ std::cout << "sub id = " << id << std::endl;});
            }
            else if(argc == 1 && strcmp(argv[0], "stop") == 0)
            {
                pubsub->stop();
            }
            else {
                std::cout << "bad command:" << (char*)buf << std::endl;
            }

            return;

        };

        parse_command((char*)buf, handler);

        return;

    };
    
    pubsub->add_event(item);
    
    pubsub->run();

    pubsub->del_event(fd);
 
    return 0;
}

