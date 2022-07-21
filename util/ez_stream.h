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
        SocketStream(bool non_block = false);
        ~SocketStream();
        
        int  listen(std::string addr);
        int  accept(int listen_fd);
        int  connect(std::string addr);
        void close(int fd);
    
        int32_t read(int fd, void* buf, int32_t size);
        int32_t write(int fd, void* buf, int32_t size);

    private:
        bool        m_non_block;
};

}
#endif
