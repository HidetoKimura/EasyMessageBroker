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

#include "ez_log.h"
#include "ez_stream.h"

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
            m_loop = std::make_unique<EventLoop>();
            m_sock = std::make_unique<SocketStream>(broker_id);
            
        }

        ~PubSubImpl()
        {
            if (m_fd)
            {
                close(m_fd);
            }
        }

        int32_t connect()
        {
            int32_t ret;
            
            ret = m_sock->connect();
            if(ret < 0) 
            {
                LOGE << "connect failed: ";
                return -1; 
            }
            
            m_fd = ret;

            EmbCommandItem command_item;
            command_item.type = EMB_MSG_TYPE_PUBLISH;
            command_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_PUBLISH_t* msg = (emb_msg_PUBLISH_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_PUBLISH_t)) || 
                    (msg->topic_len     != sizeof(msg->topic)) ||
                    (msg->data_len      != sizeof(msg->data)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                std::string topic(msg->topic);

                if((int32_t)topic.size() > msg->topic_len - 1 )
                {
                    LOGE << "bad topic";
                }

                // TODO only string. it should support binary.
                std::string data(msg->data);

                if((int32_t)data.size() > msg->data_len - 1 )
                {
                    LOGE << "bad data";
                }

                LOGI << "event =" << msg->header.type << ", topic =" << msg->topic << ", data =" << msg->topic;

                for(auto it = m_subscribers.begin(); it != m_subscribers.end(); it++ ) {
                    if((*it).topic == topic ) {
                        (*it).handler->handleMessage(topic, msg->data, msg->data_len);
                    }
                }
            };
            m_dispatch_list.push_back(command_item);

            command_item.type = EMB_MSG_TYPE_SUBACK;
            command_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_SUBACK_t* msg = (emb_msg_SUBACK_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_SUBACK_t) ) ||
                    (msg->topic_len     != sizeof(msg->topic)) ||
                    (msg->client_id_len != sizeof(msg->client_id)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                std::string topic(msg->topic);

                if((int32_t)topic.size() > msg->topic_len - 1 )
                {
                    LOGE << "bad topic";
                }

                LOGI << "event =" << msg->header.type << ", topic =" << msg->topic << ", clinet_id =" << msg->client_id;

                m_last_client_id = msg->client_id;
         
            };
            m_dispatch_list.push_back(command_item);

            command_item.type = EMB_MSG_TYPE_UNSUBACK;
            command_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_UNSUBACK_t* msg = (emb_msg_UNSUBACK_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_UNSUBACK_t) ) ||
                    (msg->client_id_len != sizeof(msg->client_id)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                LOGI << "event =" << msg->header.type << ", clinet_id =" << msg->client_id;

                for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
                {
                    if((*it).client_id == msg->client_id) {
                        it = m_subscribers.erase(it);
                    }
                    else {
                        it++;
                    }
                }

                dump_subcribes();

                return;                    
            };
            m_dispatch_list.push_back(command_item);

            EventLoopItem loop_item;
            loop_item.fd = m_fd;
            loop_item.dispatch = [this](int fd) -> void
            {
                read_event(fd);
            };
            m_loop->add_item(loop_item);

            return 0;

        }

        emb_id_t subscribe(std::string topic, std::unique_ptr<SubscribeHandler> handler)
        {
            emb_msg_SUBSCRIBE_t msg;

            if(topic.size() > sizeof(msg.topic) - 1) {
                LOGE << "error :bad topic";
                return EMB_ID_NOT_USE; 
            }

            msg.topic_len = sizeof(msg.topic);
            memset(msg.topic, 0, msg.topic_len);
            strncpy(msg.topic, topic.c_str(), topic.size()); 

            msg.header.type = EMB_MSG_TYPE_SUBSCRIBE;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(m_fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
                return EMB_ID_NOT_USE; 
            }

            read_event(m_fd);

            SubscriberItemClient item;
            item.topic = topic;
            item.handler = std::move(handler);
            item.client_id = m_last_client_id;
            m_subscribers.push_back(item);

            m_last_client_id = EMB_ID_NOT_USE;

            dump_subcribes();

            return (item.client_id);
        }

        void unsubscribe(emb_id_t client_id)
        {
            emb_msg_UNSUBSCRIBE_t msg;

            msg.client_id_len = sizeof(msg.client_id);
            msg.client_id     = client_id;

            msg.header.type = EMB_MSG_TYPE_UNSUBSCRIBE;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(m_fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
                return; 
            }

            return;
        }

        void publish(std::string topic, void* buf , int32_t len)
        {
            emb_msg_PUBLISH_t msg;

            if(topic.size() > sizeof(msg.topic) - 1) {
                LOGE << "error :bad topic";
                return; 
            }

            if(len > (int32_t)sizeof(msg.data) - 1) {
                LOGE << "error :bad data";
                return; 
            }

            msg.topic_len = sizeof(msg.topic);
            memset(msg.topic, 0, msg.topic_len);
            strncpy(msg.topic, topic.c_str(), topic.size()); 

            msg.data_len = sizeof(msg.data);
            memset(msg.data, 0, msg.data_len);
            memcpy(msg.data, buf, len);

            msg.header.type = EMB_MSG_TYPE_PUBLISH;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(m_fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
                return; 
            }

            return;
        }

        void dump_subcribes ()
        {
            std::cout << "#### client subscribers" << std::endl;          
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++) {
                std::cout << (*it).client_id << ", " << (*it).topic << ", " << (*it).handler << std::endl;          
            }
            std::cout << "####" << std::endl;          
        }

        void read_event(int fd)
        {
            #define READ_MAX 512
            char buf[READ_MAX];
            
            int ret = m_sock->read(fd, buf, sizeof buf);
            if( ret > 0 ) {
                emb_msg_header_t*  p_msg = (emb_msg_header_t*)buf;

                for(auto it = m_dispatch_list.begin(); it != m_dispatch_list.end() ; it++) 
                {
                    if( (*it).type == p_msg->type ) 
                    {
                        (*it).handler(fd, buf);
                    }
                }
            }
            else if (ret == 0) 
            {
                LOGI << "socket fd = " << fd << " disconnect."; 
                close(fd);
                exit(-1);
            }
            else
            {
                LOGE << "read failed:";
                return; 
            }
        }

        void event_loop(void) 
        {
            m_loop->run();
        }

        void add_loop_item(EventLoopItem &item)
        {
            m_loop->add_item(item);

        }

        void del_loop_item(int fd)
        {
            m_loop->del_item(fd);            
        }

    private:
        int                                 m_fd;
        emb_id_t                            m_last_client_id;
        std::unique_ptr<EventLoop>          m_loop;
        std::unique_ptr<SocketStream>       m_sock;
        std::vector<SubscriberItemClient>   m_subscribers;
        std::vector<EmbCommandItem>         m_dispatch_list;
};

PubSub::PubSub(std::string broker_id)
{
    m_impl = std::make_unique<PubSubImpl>(broker_id);
}

PubSub::~PubSub()
{
}

int32_t PubSub::connect()
{
    return m_impl->connect();
}

emb_id_t PubSub::subscribe(std::string topic, std::unique_ptr<SubscribeHandler> handler)
{
    return m_impl->subscribe(topic, std::move(handler));
}

void PubSub::unsubscribe(emb_id_t client_id)
{
    return m_impl->unsubscribe(client_id);
}

void PubSub::publish(std::string topic, void* buf , int32_t len)
{
    m_impl->publish(topic, buf, len);
}

void PubSub::event_loop(void)
{
    m_impl->event_loop();
}

void PubSub::add_loop_item(EventLoopItem &item)
{
    m_impl->add_loop_item(item);
}

void PubSub::del_loop_item(int fd)
{
    m_impl->del_loop_item(fd);
}
