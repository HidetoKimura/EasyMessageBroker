#ifndef __EZ_STREAM_H__
#define __EZ_STREAM_H__

#include <list>
#include <functional>

using DispatchHandler = std::function<void(int fd)>;

#define EVENT_LOOP_FOREVER -1

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
        
        void add_item(EventLoopItem &item);
        void del_item(int fd); 
        void run(void);
    private:
	    int  m_epoll_fd;
        bool m_running;
        std::list<EventLoopItem> m_item_list;
};

class SocketStream
{
    public:
        SocketStream(std::string addr);
        ~SocketStream();
        
        int32_t listen();
        int32_t dial();
    
        int32_t read(void* buf, int32_t len);
        int32_t write(void* buf, int32_t len);

        int get_fd(void);

    private:
        std::string m_addr;
	    int  m_fd;
        bool m_running;
};


#endif
