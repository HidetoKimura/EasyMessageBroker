#include <stdio.h>
#include <unistd.h>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <functional>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include "event_util.h"

#include "ezlog.h"

using namespace std;

EventLoop::EventLoop() 
    : m_epoll_fd(-1)
    , m_running(true)
{
    if ((m_epoll_fd = epoll_create1(0)) == -1) {
        LOGE << "epoll_create failed:";
        return;
    }
}

EventLoop::~EventLoop() 
{
    m_running = false;
    if(m_epoll_fd) {
        close(m_epoll_fd);
    }
}
        
void EventLoop::add_item(EventLoopItem &item)
{
    struct epoll_event ev;

    ev.data.fd = item.fd;
    ev.events = EPOLLIN;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, item.fd, &ev) == -1) {
        LOGE << "epoll_ctl::EPOLL_CTL_ADD m_fd ";
        close(m_epoll_fd);
        return;
    }
    m_item_list.push_back(item);
}

void EventLoop::del_item(int fd) {
    for (auto it = m_item_list.begin(); it != m_item_list.end();)
    {
        if((*it).fd == fd) {
            it = m_item_list.erase(it);
        }
        else {
            it++;
        }
    }

}
 
void EventLoop::run(int count)
{
    #define MAX_EVENTS 10
    struct epoll_event events[MAX_EVENTS];
    int nfds;
    bool flg = true;

    do {
        nfds = epoll_wait(m_epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            LOGE << "epoll_wait";
            break;
        }

        for (int n = 0; n < nfds; n++) {
            for (auto it = m_item_list.begin(); it != m_item_list.end(); it++)
            {
                if((*it).fd == events[n].data.fd) {
                    (*it).dispatch((*it).fd);
                }
            }

        }
        
        if(count == 0) {
            flg = false;
        }
        else if(count > 0) {
            count--;
        }
    } while (flg);
}

