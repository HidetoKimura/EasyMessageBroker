#ifndef __EVENT_UTIL_H__
#define __EVENT_UTIL_H__

#include <list>
#include <functional>

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
        
        void add_item(EventLoopItem &item);
        void del_item(int fd); 
        void run(int count);
    private:
	    int  m_epoll_fd;
        bool m_running;
        std::list<EventLoopItem> m_item_list;
};

#endif
