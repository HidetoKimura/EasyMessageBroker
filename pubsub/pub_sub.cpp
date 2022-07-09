#include <stdio.h>
#include <string>
#include <memory>

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#include "common_msg.h"
#include "pub_sub.h"

using namespace std;


class PubSubImpl
{
    public:
        PubSubImpl(std::string broker_id)
            : m_fd(-1)
            , m_pid(-1)
        {
            m_fd = socket(AF_UNIX, SOCK_STREAM, 0);

            struct sockaddr_un addr;
            bzero(&addr, sizeof addr);

            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, broker_id.c_str());

            int ret = connect(m_fd, (struct sockaddr*)&addr, sizeof addr);
            if (ret < 0)
            {
                perror("sub sonnect failed : ");
                return;
            }

        }

        ~PubSubImpl()
        {
            if (m_fd)
            {
                close(m_fd);
            }
        }

        void subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler)
        {
            m_topic = topic;
            m_handler = handler;

            int ret, w_len;
            common_msg_t msg;

            msg.head_sign = COMMON_MSG_HEAD_SIGN;
            msg.length  = topic.size() + 1;
            strncpy(msg.command, COMMON_MSG_COMMAND_SUBSCRIBE_REQ, sizeof(msg.command));
            
            w_len = sizeof(msg);
            ret = write(m_fd, &msg, w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write topic */
            w_len = topic.size() + 1;
            ret = write(m_fd, topic.c_str(), w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            pthread_create(&m_pid, NULL, PubSubImpl::thread_run, this);            
        }

        void publish(std::string topic, void* buf , int32_t len)
        {
            int ret, w_len;
            common_msg_t msg;

            printf("[PUBSUB]publish: %s, %d \n", (char*)buf, len);

            msg.head_sign = COMMON_MSG_HEAD_SIGN;
            msg.length  = topic.size() + 1 + len;
            strncpy(msg.command, COMMON_MSG_COMMAND_PUBLISH_REQ, sizeof(msg.command));
            
            w_len = sizeof(msg);
            ret = write(m_fd, &msg, w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write topic */
            w_len = topic.size() + 1;
            ret = write(m_fd, topic.c_str(), w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write message */
            ret = write(m_fd, buf, len);
            if(ret != len) {
                perror("write fail");
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
            PubSubImpl* impl = (PubSubImpl*)args;
            impl->run();

            return NULL;
        }

        void run()
        {
            printf("sub thread running....\n");
            while (1)
            {
                struct pollfd fd[1];
                fd[0].fd = m_fd;
                fd[0].events = POLLIN;

                int ret = poll(fd, 1, -1);
                
                if (ret > 0)
                {
                    if (fd[0].revents & POLLIN)
                    {
                        char buf[512];
                        bzero(buf, sizeof buf);
                        int len = read(fd[0].fd, buf, sizeof buf);
                        if (len > 0) 
                        {
                            common_msg_t*  p_msg = (common_msg_t*)buf;
                            uint8_t*       p_body = (uint8_t*)(p_msg + 1);
                            
                            if( p_msg->head_sign != COMMON_MSG_HEAD_SIGN) {
                                perror("msg header destoyed");
                            }

                            printf("[PUBSUB] event_read : %c%c%c%c \n", p_msg->command[0], p_msg->command[1], p_msg->command[2], p_msg->command[3]);

                            if( 0 == strncmp(p_msg->command, COMMON_MSG_COMMAND_PUBLISH_NTY, sizeof(p_msg->command)) ) {
                                std::string topic = (char*)p_body;
                                m_handler->handleMessage(topic, p_body + topic.size() + 1 , p_msg->length);
                            }

                        }
                        else if (len == 0) 
                        {
                            printf("[PUBSUB]socket %d disconnected\n", fd[0].fd);
                            break;
                        }
                        else
                        {
                            perror("read failed:");
                        }
                    }
                }
            }
        }


    private:
        int m_fd;
        pthread_t m_pid;
        std::string m_topic;
        std::shared_ptr<SubscribeHandler> m_handler = nullptr;
};

PubSub::PubSub(std::string broker_id)
{
    m_impl = std::make_unique<PubSubImpl>(broker_id);
}

PubSub::~PubSub()
{
}

void PubSub::subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler)
{
    m_impl->subscribe(topic, handler);
}

void PubSub::publish(std::string topic, void* buf , int32_t len)
{
    m_impl->publish(topic, buf, len);
}

void PubSub::event_loop(void)
{
    m_impl->event_loop();

}
