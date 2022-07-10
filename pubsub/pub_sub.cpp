#include <stdio.h>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <functional>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include "emb_msg.h"
#include "pub_sub.h"

#include "ezlog.h"
#include "event_util.h"

using namespace std;

struct SubscriberItemClient {
    emb_id_t        client_id;
    std::string     topic;
    std::shared_ptr<SubscribeHandler> handler;
};

class PubSubImpl
{
    public:
        PubSubImpl(std::string broker_id)
            : m_fd(-1)
            , m_last_client_id(EMB_ID_NOT_USE)
            , m_subscribers()
        {
            m_fd = socket(AF_UNIX, SOCK_STREAM, 0);

            struct sockaddr_un addr;
            bzero(&addr, sizeof addr);

            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, broker_id.c_str());

            int ret = connect(m_fd, (struct sockaddr*)&addr, sizeof addr);
            if (ret < 0)
            {
                LOGE << "sub sonnect failed : ";
                return;
            }
            
            EmbCommandItem command_item;
            command_item.command = EMB_MSG_COMMAND_PUBLISH;
            command_item.handler = [this](int fd, std::string command, void* head) -> void
            {
                emb_msg_t* msg = (emb_msg_t*)head;
                char* p_body = (char*)msg + sizeof(emb_msg_t);
                std::string topic = p_body;
                char* data = (char*)p_body + msg->topic_len;
                LOGI << "event =" << command << ", topic =" << topic << ", data =" << data;
                for(auto it = m_subscribers.begin(); it != m_subscribers.end(); it++ ) {
                    if((*it).topic == topic ) {
                        (*it).handler->handleMessage(topic, data, msg->data_len);
                    }
                }
            };
            m_command_list.push_back(command_item);

            command_item.command = EMB_MSG_COMMAND_SUBACK;
            command_item.handler = [this](int fd, std::string command, void* head) -> void
            {
                emb_msg_t* msg = (emb_msg_t*)head;
                char* p_body = (char*)msg + sizeof(emb_msg_t);
                std::string topic = p_body;
                char* data = (char*)p_body + msg->topic_len;
                std::string client_id = data;
                const char* p = client_id.c_str();
                char* end;
                emb_id_t client_id_ul = strtoul(p, &end, 10);

                LOGI << "event =" << command << ", topic =" << topic << ", clinet_id =" << client_id_ul;

                m_last_client_id = client_id_ul;
         
            };
            m_command_list.push_back(command_item);

            command_item.command = EMB_MSG_COMMAND_UNSUBACK;
            command_item.handler = [this](int fd, std::string command, void* head) -> void
            {
                emb_msg_t* msg = (emb_msg_t*)head;
                char* p_body = (char*)msg + sizeof(emb_msg_t);
                std::string topic = p_body;
                char* data = (char*)p_body + msg->topic_len;
                std::string client_id = data;
                LOGI << "event =" << command << ", topic =" << topic << ", clinet_id =" << client_id;

                const char* p = client_id.c_str();
                char* end;
                emb_id_t client_id_ul = strtoul(p, &end, 10);

                for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
                {
                    if((*it).client_id == client_id_ul) {
                        it = m_subscribers.erase(it);
                    }
                    else {
                        it++;
                    }
                }

            };
            m_command_list.push_back(command_item);

            m_loop = std::make_unique<EventLoop>();

            EventLoopItem loop_item;
            loop_item.fd = m_fd;
            loop_item.dispatch = [this](int fd) -> void
            {
                read_event(fd);
            };
            m_loop->add_item(loop_item);

        }

        ~PubSubImpl()
        {
            if (m_fd)
            {
                close(m_fd);
            }
        }

        emb_id_t subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler)
        {
            int ret, w_len;
            emb_msg_t msg;

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len  = topic.size() + 1;
            memcpy(msg.command, EMB_MSG_COMMAND_SUBSCRIBE, sizeof(msg.command));
            
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

            read_event(m_fd);


            SubscriberItemClient item;
            item.topic = topic;
            item.handler = handler;
            item.client_id = m_last_client_id;
            m_subscribers.push_back(item);

            m_last_client_id = EMB_ID_NOT_USE;

            dump_subcribes();

            return (item.client_id);
        }
        void unsubscribe(emb_id_t client_id)
        {
            int ret, w_len;
            emb_msg_t msg;
            std::string client_id_str = std::to_string(client_id);

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len = 0;
            msg.data_len  = client_id_str.size() + 1;
            memcpy(msg.command, EMB_MSG_COMMAND_UNSUBSCRIBE, sizeof(msg.command));

            w_len = sizeof(msg);
            ret = write(m_fd, &msg, w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write message */
            w_len = msg.data_len;
            ret = write(m_fd, client_id_str.c_str(), w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            read_event(m_fd);

            for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
            {
                if((*it).client_id == client_id) {
                    it = m_subscribers.erase(it);
                }
                else {
                    it++;
                }
            }
            dump_subcribes();

        }

        void dump_subcribes ()
        {
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++) {
                LOGD << (*it).client_id << ", " << (*it).topic << ", " << (*it).handler;          
            }

        }


        void publish(std::string topic, void* buf , int32_t len)
        {
            int ret, w_len;
            emb_msg_t msg;

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len  = topic.size() + 1;
            msg.data_len  = len;
            memcpy(msg.command, EMB_MSG_COMMAND_PUBLISH, sizeof(msg.command));
            
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

        void read_event(int fd)
        {
            char buf[512];
            bzero(buf, sizeof buf);
            int len = read(fd, buf, sizeof buf);
            if (len > 0) 
            {
                emb_msg_t*  p_msg = (emb_msg_t*)buf;
                
                if( p_msg->head_sign != EMB_MSG_HEAD_SIGN) {
                        LOGE << "msg header destoyed";
                }

                char str[5];
                bzero(str, sizeof str);
                memcpy(str, p_msg->command, sizeof(p_msg->command)); 
                std::string command(str);

                for(auto it = m_command_list.begin(); it != m_command_list.end() ; it++) 
                {
                    if( (*it).command == command ) 
                    {
                        (*it).handler(fd, command, p_msg);
                    }
                }
            }
            else if (len == 0) 
            {
                LOGE << "socket " <<  fd << "disconnected";
                return; 
            }
            else
            {
                //TODO add EINTR, EAGAIN...
                LOGE << "read failed:";
                return; 
            }
        }

        void event_loop(int count) 
        {
            m_loop->run(count);
        }

    private:
        int m_fd;
        emb_id_t    m_last_client_id;
        std::unique_ptr<EventLoop> m_loop;
        std::vector<SubscriberItemClient> m_subscribers;
        std::vector<EmbCommandItem>   m_command_list;
};

PubSub::PubSub(std::string broker_id)
{
    m_impl = std::make_unique<PubSubImpl>(broker_id);
}

PubSub::~PubSub()
{
}

emb_id_t PubSub::subscribe(std::string topic, std::shared_ptr<SubscribeHandler> handler)
{
    return m_impl->subscribe(topic, handler);
}

void PubSub::unsubscribe(emb_id_t client_id)
{
    return m_impl->unsubscribe(client_id);
}

void PubSub::publish(std::string topic, void* buf , int32_t len)
{
    m_impl->publish(topic, buf, len);
}

void PubSub::event_loop(int count)
{
    m_impl->event_loop(count);
}
