#ifndef __EZ_STREAM_H__
#define __EZ_STREAM_H__

#include <list>
#include <functional>

namespace ez::stream {

using DispatchHandler = std::function<void(int fd)>;

struct EventLoopItem 
{
    int             fd;
    DispatchHandler dispatch;
};

class EventLoop
{
    public:
        EventLoop();
        ~EventLoop() ;
        
        void add_event(EventLoopItem &item);
        void del_event(int fd); 
        void run(void);
        void stop(void);
    private:
	    int  m_epoll_fd;
        bool m_running;
        std::list<EventLoopItem> m_item_list;
};

class SocketStream
{
    public:
        SocketStream(std::string addr, bool non_block = false);
        ~SocketStream();
        
        int  listen();
        int  accept(int listen_fd);
        int  connect();
        void close(int fd);
    
        int32_t read(int fd, void* buf, int32_t size);
        int32_t write(int fd, void* buf, int32_t size);

    private:
        std::string m_addr;
        bool        m_non_block;
};

}
#endif
