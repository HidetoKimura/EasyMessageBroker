#include "broker.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <pthread.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include "common_msg.h"

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
                perror("bind error:");
                return ;
            }

            listen(m_serverFd, 64);

            // start thread to listen
            pthread_create(&m_pid, NULL, BrokerImpl::thread_run, this);
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
            printf("[BROKER] event_publish: %s, %d \n", (char*)buf, len);
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
            {
                int ret, w_len;
                common_msg_t msg;

                if((*it).topic != topic) continue;

                msg.head_sign = COMMON_MSG_HEAD_SIGN;
                msg.length  = (*it).topic.size() + 1 + len;
                strncpy(msg.command, COMMON_MSG_COMMAND_PUBLISH_NTY, sizeof(msg.command));
 
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

        void event_loop()
        {
            while(true) 
            {
                sleep(10);
            }
        }

        static void* thread_run(void* args)
        {
            BrokerImpl* m_impl = (BrokerImpl*)args;
            m_impl->run();

            return NULL;
        }

        void run()
        {
            printf("[BROKER]thread running....\n");
            while (m_running)
            {
                struct pollfd *fd;
                int count = m_clients.size() + 1;
                fd = (struct pollfd*)malloc(sizeof(struct pollfd) * count);
                bzero(fd, sizeof(struct pollfd) * count);

                fd[0].fd = m_serverFd;
                fd[0].events = POLLIN;

                // TODO: lock m_clients?
                for (int i = 1; i < count; i++)
                {
                    fd[i].fd = m_clients[i-1];
                    fd[i].events = POLLIN;
                }

                int ret = poll(fd, count, 500);
                if (ret > 0)
                {
                    printf("[BROKER]fd[0].revnts %d, pollin is %d\n", fd[0].revents, POLLIN);
                    if (fd[0].revents & POLLIN)
                    {
                        // connection come
                        sockaddr_un cli_addr;
                        socklen_t len = 0;
                        printf("[BROKER]start to accept...\n");
                        int fd = accept(m_serverFd, (struct sockaddr*)&cli_addr, &len);
                        printf("[BROKER]accpet result is %d \n", fd);
                        if (fd == -1)
                        {
                            perror("accpet connection failed:");
                            continue;
                        }

                        m_clients.push_back(fd);
                        printf("[BROKER]connection ok with fd : %d \n", fd);
                    }

                    for (int i = 1; i < count; i++)
                    {
                        if (fd[i].revents & POLLIN)
                        {
                            // socket event...
                            char buf[512];
                            bzero(buf, sizeof buf);
                            int ret = read(m_clients[i-1], buf, sizeof buf);
                            if( ret > 0 ) {

                                common_msg_t*  p_msg = (common_msg_t*)buf;
                                uint8_t*       p_body = (uint8_t*)(p_msg + 1);

                                if( p_msg->head_sign != COMMON_MSG_HEAD_SIGN) {
                                    perror("msg header destoyed");
                                }
                                printf("[BROKER] event_read : %c%c%c%c \n", p_msg->command[0], p_msg->command[1], p_msg->command[2], p_msg->command[3]);

                                if( 0 == strncmp(p_msg->command, COMMON_MSG_COMMAND_PUBLISH_REQ, sizeof(p_msg->command)) ) {
                                    std::string topic = (char*)p_body;
                                    printf("topic = %s \n", topic.c_str());
                                    event_publish(topic, p_body + topic.size() + 1, p_msg->length);
                                }
                                if( 0 == strncmp(p_msg->command, COMMON_MSG_COMMAND_SUBSCRIBE_REQ, sizeof(p_msg->command)) ) {
                                    std::string topic = (char*)p_body;
                                    printf("topic = %s \n", topic.c_str());
                                    subscriber_item_t item;
                                    item.topic = topic;
                                    item.fd = m_clients[i-1];
                                    m_subscribers.push_back(item);
                                }
                            }
                            else if (ret == 0) 
                            {
                                printf("[BROKER]socket %d disconnect.\n", fd[i].fd);
                                m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), fd[i].fd), m_clients.end());
                                close(fd[i].fd);
                            }
                        }
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


