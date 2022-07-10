#include <stdio.h>
#include <string>
#include <memory>

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#include "emb_msg.h"
#include "emb_log.h"
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
            emb_msg_t msg;

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len  = topic.size() + 1;
            strncpy(msg.command, EMB_MSG_COMMAND_SUBSCRIBE, sizeof(msg.command));
            
            w_len = sizeof(msg);
            ret = write(m_fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail : msg";
            }

            /* write topic */
            w_len = msg.topic_len;
            ret = write(m_fd, topic.c_str(), w_len);
            if(ret != w_len) {
                LOGE << "write fail : topic";
            }

            pthread_create(&m_pid, NULL, PubSubImpl::thread_run, this);            
        }

        void publish(std::string topic, void* buf , int32_t len)
        {
            int ret, w_len;
            emb_msg_t msg;

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len  = topic.size() + 1;
            msg.data_len  = len;
            strncpy(msg.command, EMB_MSG_COMMAND_PUBLISH, sizeof(msg.command));
            
            w_len = sizeof(msg);
            ret = write(m_fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail : msg";
            }

            /* write topic */
            w_len = msg.topic_len;
            ret = write(m_fd, topic.c_str(), w_len);
            if(ret != w_len) {
                LOGE << "write fail : topic"; 
            }

            /* write data */
            w_len = msg.data_len;
            ret = write(m_fd, buf, w_len);
            if(ret != w_len) {
                LOGE << "write fail : data";
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
                            emb_msg_t*  p_msg = (emb_msg_t*)buf;
                            uint8_t*    p_body = (uint8_t*)(p_msg + 1);
                            

                            if( p_msg->head_sign != EMB_MSG_HEAD_SIGN) {
                                    LOGE << "msg header destoyed";
                                }

                            uint32_t    topic_len = p_msg->topic_len;
                            uint32_t    data_len  = p_msg->data_len;

                            char str[5];
                            bzero(str, sizeof str);
                            strncpy(str, p_msg->command, sizeof(p_msg->command)); 

                            std::string command(str);

                            if( command == EMB_MSG_COMMAND_PUBLISH ) {
                                std::string topic = (char*)p_body;
                                char* data = (char*)p_body + topic_len;
                                LOGI << "event =" << command << ", topic =" << topic << ", data =" << data;
                                m_handler->handleMessage(topic, data, p_msg->data_len);
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
