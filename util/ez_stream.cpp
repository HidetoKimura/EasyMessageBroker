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

#include "ez_stream.h"
#include "ez_log.h"

using namespace std;

EventLoop::EventLoop() 
    : m_epoll_fd(-1)
    , m_running(true)
    , m_item_list()
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

    if(m_epoll_fd == -1) return;

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
 
void EventLoop::run(void))
{
    #define MAX_EVENTS 10
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    if(m_epoll_fd == -1) return;

    while(m_running) {
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
        
    }
}

#include <stdio.h>
#include <sys/socket.h>

SocketStream::SocketStream(std::string addr)
    : m_fd(-1)
    , m_addr()
{

}

SocketStream::~SocketStream()
{
    if(m_fd) {
        close(m_fd);
    }

    unlink(m_addr.c_str());
}

int32_t SocketStream::listen(void)
{
    int ret;

    m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd < 0)
    {
        perror("socket error:");
        return -1;
    }

    sockaddr_un addr;
    bzero(&addr, sizeof addr);
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, m_addr.c_str());

    // delete file that going to bind
    unlink(m_addr.c_str());

    ret = bind(m_fd, (struct sockaddr*)&addr, sizeof addr);
    if (ret < 0)
    {
        perror("bind error:");
        return -1;
    }

    ret = listen(m_fd, 64);
    if (ret < 0)
    {
        perror("listen error:");
        return -1;
    }

    return 0;

}

int32_t SocketStream::connect(void)
{
    m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd < 0)
    {
        perror("socket error:");
        return -1;
    }

    struct sockaddr_un addr;
    bzero(&addr, sizeof addr);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, m_addr.c_str());

    int ret = connect(m_fd, (struct sockaddr*)&addr, sizeof addr);
    if (ret < 0)
    {
        perror("connect failed:");
        return -1;
    }

    return 0;
}

int32_t SocketStream::accept(void)
{
    int fd;
    sockaddr_un addr;
    socklen_t addrlen = 0;
 
    fd = accept(m_fd, (struct sockaddr *) &addr, &addrlen);
    if (ret < 0) {
        perror("accept failed:");
        return -1;
    }

    return fd;
}


int32_t SocketStream::read(void* buf, int32_t len)
{

}

int32_t SocketStream::write(void* buf, int32_t len)
{

}

int SocketStream::get_fd(void)
{
    return m_fd;
}

