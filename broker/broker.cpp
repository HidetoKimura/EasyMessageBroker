#include "broker.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include "emb_msg.h"
#include "emb_log.h"

using namespace std;

typedef struct {
    std::string     topic;
    int             fd;
} subscriber_item_t;

class BrokerImpl
{
    public :
        BrokerImpl(std::string broker_id)
          : m_serverFd(-1)
          , m_clients()
          , m_subscribers()
          , m_pid(-1)
          , m_running(true)
          , m_broker_id(broker_id)
        {
            m_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
            sockaddr_un addr;
            bzero(&addr, sizeof addr);
            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, m_broker_id.c_str());

            // delete file that going to bind
            unlink(m_broker_id.c_str());

            int ret = bind(m_serverFd, (struct sockaddr*)&addr, sizeof addr);
            if (ret < 0)
            {
                LOGE << "bind error:";
                return ;
            }

            listen(m_serverFd, 64);
        }

        ~BrokerImpl()
        {
            m_running = false;
            pthread_join(m_pid, NULL);
            close(m_serverFd);

            unlink(m_broker_id.c_str());

            for (auto it = m_clients.begin(); it != m_clients.end(); it++)
            {
                close(*it);
            }
        }

        void event_publish(std::string topic, void *buf, int32_t len)
        {
            LOGD << (char*)buf;
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
            {
                int ret, w_len;
                emb_msg_t msg;

                if((*it).topic != topic) continue;

                msg.head_sign = EMB_MSG_HEAD_SIGN;
                msg.topic_len = (*it).topic.size() + 1;
                msg.data_len  = len;
                strncpy(msg.command, EMB_MSG_COMMAND_PUBLISH, sizeof(msg.command));
 
                w_len = sizeof(msg);
                ret = write((*it).fd, &msg, w_len);
                if(ret != w_len) {
                    perror("write fail");
                }

                /* write topic */
                w_len = (*it).topic.size() + 1;
                ret = write((*it).fd, (*it).topic.c_str(), w_len);
                if(ret != w_len) {
                    perror("write fail");
                }

                /* write message */
                ret = write((*it).fd, buf, len);
                if(ret != len) {
                    perror("write fail");
                }

            }
        }

        void read_event(int fd) 
        {
            char buf[512];
            bzero(buf, sizeof buf);
            int ret = read(fd, buf, sizeof buf);
            if( ret > 0 ) {

                emb_msg_t*  p_msg =  (emb_msg_t*)buf;
                uint8_t*    p_body = (uint8_t*)(p_msg + 1);

                if( p_msg->head_sign != EMB_MSG_HEAD_SIGN) {
                    LOGE << "msg header destoyed";
                }

                uint32_t    topic_len = p_msg->topic_len;
                uint32_t    data_len = p_msg->data_len;

                char str[5];
                bzero(str, sizeof str);
                strncpy(str, p_msg->command, sizeof(p_msg->command)); 

                std::string command(str);

                if (command == EMB_MSG_COMMAND_SUBSCRIBE) {
                    std::string topic = (char*)p_body;
                    LOGI << "event =" << command << ", topic =" << topic;

                    subscriber_item_t item;
                    item.topic = topic;
                    item.fd = fd;                                ;
                    m_subscribers.push_back(item);
                }
                else if (command == EMB_MSG_COMMAND_PUBLISH) {
                    std::string topic = (char*)p_body;
                    char* data = (char*)p_body + topic_len;
                    LOGI << "event =" << command << ", topic =" << topic << ", data =" << data;
                    event_publish(topic, data, p_msg->data_len);
                }

            }
            else if (ret == 0) 
            {
                LOGI << "socket fd = " << fd << "disconnect."; 
                m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), fd), m_clients.end());
                close(fd);
            }
        }

        void event_loop()
        {
            #define MAX_EVENTS 10
           struct epoll_event ev, events[MAX_EVENTS];
           int conn_sock, nfds, epollfd;

            if ((epollfd = epoll_create1(0)) == -1) {
                LOGE << "epoll_create";
                return;
            }
            ev.data.fd = m_serverFd;
            ev.events = EPOLLIN;

            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, m_serverFd, &ev) == -1) {
                LOGE << "epoll_ctl::EPOLL_CTL_ADD m_serverFd ";
                (void) close(epollfd);
                return;
            }

            while (m_running)
            {
                nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
                if (nfds == -1) {
                    LOGE << "epoll_wait";
                    break;
                }

                for (int n = 0; n < nfds; n++) {
                    if (events[n].data.fd == m_serverFd) {
                        sockaddr_un addr;
                        socklen_t addrlen = 0;
                        conn_sock = accept(m_serverFd,
                                            (struct sockaddr *) &addr, &addrlen);
                        if (conn_sock == -1) {
                            LOGE << "accept";
                            continue;
                        }
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = conn_sock;
                        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                            LOGE << "epoll_ctl: conn_sock";
                            continue;
                        }
                        m_clients.push_back(conn_sock);

                   } else {
                       read_event(events[n].data.fd);
                   }
               }
            }
        }

    private:
        int m_serverFd;
        std::vector<int> m_clients;
        std::vector<subscriber_item_t> m_subscribers;
        pthread_t       m_pid;
        bool        m_running;
        std::string m_broker_id;
};

Broker::Broker(std::string broker_id)
{
    m_impl = std::make_unique<BrokerImpl>(broker_id);
}

Broker::~Broker()
{
}

void Broker::event_loop(void)
{
    m_impl->event_loop();
}


