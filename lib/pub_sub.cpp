#include <stdio.h>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <mutex>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include "emb.h"
#include "emb_internal.h"

#include "ez_log.h"
#include "ez_stream.h"

namespace emb {

using namespace ez::stream;

struct SubscriberItemClient {
    emb_id_t            subscription_id;
    std::string         topic;
    SubscribeHandler    on_subscribe;
    ResultHandler       on_result;
};

class PubSubImpl
{
    public:
        PubSubImpl()
            : m_fd(-1)
            , m_mtx()
            , m_last_subscription_id(EMB_ID_NOT_USE)
            , m_subscribers()
        {
            m_loop = std::make_unique<EventLoop>();
            m_sock = std::make_unique<SocketStream>();
            
        }

        ~PubSubImpl()
        {
            if (m_fd)
            {
                m_sock->close(m_fd);
            }
        }
        
        int32_t connect(std::string broker_id, ConnectionHandler on_conn)
        {
            int32_t ret;
            
            
            ret = m_sock->connect(broker_id);
            if(ret < 0) 
            {
                LOGE << "connect failed: ";
                return -1; 
            }
            
            m_fd = ret;
            if(on_conn != nullptr)
            {
                on_conn(m_fd);
            }

            EmbCommandItem command_item;
            command_item.type = EMB_MSG_TYPE_PUBLISH;
            command_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_PUBLISH_t* msg = (emb_msg_PUBLISH_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_PUBLISH_t)) || 
                    (msg->topic_len     != sizeof(msg->topic)) ||
                    (msg->data_len      != sizeof(msg->data))  ||
                    (msg->subscription_id_len != sizeof(msg->subscription_id)) )
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
                std::string data((char*)msg->data);

                if((int32_t)data.size() > msg->data_len - 1 )
                {
                    LOGE << "bad data";
                }

                LOGI << "event =" << msg->header.type << ", topic =" << msg->topic << ", data =" << msg->data << ", subscription_id =" << msg->subscription_id;

                std::unique_lock<std::mutex> lk(m_mtx);
                for(auto it = m_subscribers.begin(); it != m_subscribers.end(); it++ ) {
                    if((*it).subscription_id == msg->subscription_id ) {
                        (*it).on_subscribe(topic, msg->data, msg->data_len);
                    }
                }
                lk.unlock();
            };
            m_dispatch_list.push_back(command_item);

            command_item.type = EMB_MSG_TYPE_SUBACK;
            command_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_SUBACK_t* msg = (emb_msg_SUBACK_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_SUBACK_t) ) ||
                    (msg->topic_len     != sizeof(msg->topic)) ||
                    (msg->subscription_id_len != sizeof(msg->subscription_id)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                std::string topic(msg->topic);

                if((int32_t)topic.size() > msg->topic_len - 1 )
                {
                    LOGE << "bad topic";
                }

                LOGI << "event =" << msg->header.type << ", topic =" << msg->topic << ", clinet_id =" << msg->subscription_id;

                std::unique_lock<std::mutex> lk(m_mtx);
                for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
                {
                    if((*it).subscription_id == EMB_ID_NOT_USE && (*it).topic == topic && (*it).on_result != nullptr) {
                        (*it).subscription_id = msg->subscription_id;   
                        (*it).on_result(msg->subscription_id);
                        (*it).on_result = nullptr;
                        break;
                    }
                }
                lk.unlock();

                dump_subcribes();

                return;

            };
            m_dispatch_list.push_back(command_item);

            command_item.type = EMB_MSG_TYPE_UNSUBACK;
            command_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_UNSUBACK_t* msg = (emb_msg_UNSUBACK_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_UNSUBACK_t) ) ||
                    (msg->subscription_id_len != sizeof(msg->subscription_id)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                LOGI << "event =" << msg->header.type << ", subscription_id =" << msg->subscription_id;

                std::unique_lock<std::mutex> lk(m_mtx);
                for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
                {
                    if((*it).subscription_id == msg->subscription_id) {
                        if((*it).on_result != nullptr) {                        
                            (*it).on_result(msg->subscription_id);
                            (*it).on_result = nullptr;
                        }
                        it = m_subscribers.erase(it);
                    }
                    else {
                        it++;
                    }
                }
                lk.unlock();    

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
            m_loop->add_event(loop_item);

            return 0;

        }

        void subscribe(std::string topic, SubscribeHandler on_subscribe, ResultHandler on_result)
        {
            emb_msg_SUBSCRIBE_t msg;

            if(topic.size() > sizeof(msg.topic) - 1) {
                LOGE << "error :bad topic";
                return; 
            }

            SubscriberItemClient item;
            item.topic = topic;
            item.on_subscribe = on_subscribe;
            item.on_result = on_result;
            item.subscription_id = EMB_ID_NOT_USE;

            std::unique_lock<std::mutex> lk(m_mtx);
            m_subscribers.push_back(item);
            lk.unlock();

            msg.topic_len = sizeof(msg.topic);
            memset(msg.topic, 0, msg.topic_len);
            strncpy(msg.topic, topic.c_str(), topic.size()); 

            msg.header.type = EMB_MSG_TYPE_SUBSCRIBE;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(m_fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
                return ; 
            }


            return ;
        }

        void unsubscribe(emb_id_t subscription_id, ResultHandler on_result)
        {
            emb_msg_UNSUBSCRIBE_t msg;

            std::unique_lock<std::mutex> lk(m_mtx);
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
            {
                if((*it).subscription_id == subscription_id) {
                    if((*it).on_result != nullptr) {
                        LOGE << "NOT completed previous request.";
                        return;
                    }
                    (*it).on_result = on_result;
                }
            }
            lk.unlock();

            msg.subscription_id_len = sizeof(msg.subscription_id);
            msg.subscription_id     = subscription_id;

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
        void publish(std::string topic, void* buf , int32_t len, emb_id_t to_id)
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

            msg.subscription_id_len = sizeof(msg.subscription_id_len);
            msg.subscription_id     = to_id;

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
            std::unique_lock<std::mutex> lk(m_mtx);

            std::cout << "#### client subscribers" << std::endl;          
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++) {
                std::cout << (*it).subscription_id << ", " << (*it).topic << std::endl;          
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
                m_sock->close(fd);
                exit(-1);
            }
            else
            {
                LOGE << "read failed:";
                return; 
            }
        }

        void run(void) 
        {
            m_loop->run();
        }

        void stop(void) 
        {
            m_loop->stop();
        }

        void add_event(EventLoopItem &item)
        {
            m_loop->add_event(item);

        }

        void del_event(int fd)
        {
            m_loop->del_event(fd);            
        }

    private:
        int                                 m_fd;
        std::mutex                          m_mtx;
        emb_id_t                            m_last_subscription_id;
        std::unique_ptr<EventLoop>          m_loop;
        std::unique_ptr<SocketStream>       m_sock;
        std::vector<SubscriberItemClient>   m_subscribers;
        std::vector<EmbCommandItem>         m_dispatch_list;
};

PubSub::PubSub()
{
    m_impl = std::make_unique<PubSubImpl>();
}

PubSub::~PubSub()
{
}

int32_t PubSub::connect(std::string broker_id,  ConnectionHandler on_conn)
{
    return m_impl->connect(broker_id, on_conn);
}

void PubSub::subscribe(std::string topic, SubscribeHandler on_subsribe, ResultHandler on_result)
{
    return m_impl->subscribe(topic, on_subsribe, on_result);
}

void PubSub::unsubscribe(emb_id_t subscription_id, ResultHandler on_result)
{
    return m_impl->unsubscribe(subscription_id, on_result);
}

void PubSub::publish(std::string topic, void* buf , int32_t len)
{
    m_impl->publish(topic, buf, len, EMB_ID_BROADCAST);
}

void PubSub::publish(std::string topic, void* buf , int32_t len, emb_id_t to_id)
{
    m_impl->publish(topic, buf, len, to_id);
}

void PubSub::run(void)
{
    m_impl->run();
}

void PubSub::stop(void)
{
    m_impl->stop();
}

void PubSub::add_event(EventLoopItem &item)
{
    m_impl->add_event(item);
}

void PubSub::del_event(int fd)
{
    m_impl->del_event(fd);
}

void PubSub::read_event(int fd)
{
    m_impl->read_event(fd);
}

}
