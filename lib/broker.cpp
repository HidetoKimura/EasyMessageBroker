
#include <stdio.h>
#include <stdlib.h>

#include <sys/un.h>
#include <sys/epoll.h>
//#include <pthread.h>

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include "ez_stream.h"
#include "ez_log.h"

#include "emb.h"
#include "emb_internal.h"

namespace emb {

using namespace ez::stream;

typedef struct {
    emb_id_t        subscription_id;
    std::string     topic;
    int             fd;
} SuscribeItemBroker;

class BrokerImpl
{
    public :
        BrokerImpl()
          : m_serverFd(-1)
          , m_clients()
          , m_subscribers()
          , m_last_subscription_id(10000)
        {
            m_loop = std::make_unique<EventLoop>();
            m_sock = std::make_unique<SocketStream>();
        }

        ~BrokerImpl()
        {
            for (auto it = m_clients.begin(); it != m_clients.end(); it++)
            {
                m_sock->close(*it);
            }
        }

        int32_t listen(std::string broker_id, ConnectionHandler on_listen, ConnectionHandler on_conn)
        {

            m_serverFd = m_sock->listen(broker_id);
            if(m_serverFd < 0) 
            {
                LOGE << "listen failed: ";
                return -1; 
            }


            if(on_listen != nullptr)
            {
                on_listen(m_serverFd);
            }

            EmbCommandItem dispatch_item;
            dispatch_item.type = EMB_MSG_TYPE_PUBLISH;
            dispatch_item.handler = [this](int fd, void* recv_msg) -> void
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

                // TODO only string. it should binary.
                std::string data((char*)msg->data);

                if((int32_t)data.size() > msg->data_len - 1 )
                {
                    LOGE << "bad data";
                }

                LOGI << "event =" << msg->header.type << ", topic =" << msg->topic << ", data =" << msg->data;

                send_publish(msg);

                return;

            };
            m_dispatch_list.push_back(dispatch_item);

            dispatch_item.type = EMB_MSG_TYPE_SUBSCRIBE;
            dispatch_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_SUBSCRIBE_t* msg = (emb_msg_SUBSCRIBE_t*)recv_msg;

                if( (msg->header.len != sizeof(emb_msg_SUBSCRIBE_t)) ||
                    (msg->topic_len  != sizeof(msg->topic)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                LOGI << "event =" << msg->header.type << ", topic =" << msg->topic;

                std::string topic(msg->topic);

                if((int32_t)topic.size() > msg->topic_len - 1 )
                {
                    LOGE << "bad topic";
                }

                SuscribeItemBroker item;
                item.topic = topic;
                item.fd = fd;
                item.subscription_id = m_last_subscription_id;
                m_last_subscription_id++;
                m_subscribers.push_back(item);

                send_suback(fd, topic, item.subscription_id);
                
                dump_subcribes();
                return;
            };
            m_dispatch_list.push_back(dispatch_item);

            dispatch_item.type = EMB_MSG_TYPE_UNSUBSCRIBE;
            dispatch_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_UNSUBSCRIBE_t* msg = (emb_msg_UNSUBSCRIBE_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_UNSUBSCRIBE_t) ) ||
                    (msg->subscription_id_len != sizeof(msg->subscription_id)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                LOGI << "event =" << msg->header.type << ", clinet_id =" << msg->subscription_id;
                
                send_unsuback(fd, msg->subscription_id);

                for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
                {
                    if((*it).subscription_id == msg->subscription_id) {
                        it = m_subscribers.erase(it);
                    }
                    else {
                        it++;
                    }
                }
                
                dump_subcribes();                
                return;
            };

            m_dispatch_list.push_back(dispatch_item);

            EventLoopItem loop_item;
            loop_item.fd = m_serverFd;
            loop_item.dispatch = [this, on_conn](int fd) -> void
            {
                int  conn_sock;

                conn_sock = m_sock->accept(fd);
                if (conn_sock < 0) {
                    LOGE << "accept failed:";
                    return;
                }

                if(on_conn != nullptr)
                {
                    on_conn(conn_sock);
                }

                EventLoopItem conn_item;
                conn_item.fd = conn_sock;
                conn_item.dispatch = [this](int fd) -> void
                {
                    read_event(fd);
                };
                m_loop->add_event(conn_item);

                m_clients.push_back(conn_sock);

            };
            m_loop->add_event(loop_item);

            return 0;

        }

        void send_suback(int fd, std::string topic, uint32_t subscription_id)
        {
            emb_msg_SUBACK_t msg;

            if(topic.size() > sizeof(msg.topic) - 1) {
                LOGE << "bad topic";
            }

            msg.topic_len = sizeof(msg.topic);
            memset(msg.topic, 0, msg.topic_len);
            strncpy(msg.topic, topic.c_str(), topic.size()); 

            msg.subscription_id_len = sizeof(msg.subscription_id);
            msg.subscription_id = subscription_id;

            msg.header.type = EMB_MSG_TYPE_SUBACK;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
            }
            return;
        }

        void send_unsuback(int fd, uint32_t subscription_id)
        {
            emb_msg_UNSUBACK_t msg;

            msg.subscription_id_len = sizeof(msg.subscription_id);
            msg.subscription_id = subscription_id;

            msg.header.type = EMB_MSG_TYPE_UNSUBACK;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
            }
            return;
        }

        void send_publish(emb_msg_PUBLISH_t *msg)
        {
            std::string topic(msg->topic);
            emb_id_t tmp_subscription_id = msg->subscription_id;

            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
            {
                if((*it).topic != topic) continue;
                if(tmp_subscription_id != EMB_ID_BROADCAST) {
                    if((*it).subscription_id != tmp_subscription_id) continue;
                }
                msg->subscription_id = (*it).subscription_id;
                int w_len = msg->header.len;
                int ret = m_sock->write((*it).fd, msg, w_len);
                if(ret != w_len) {
                    LOGE << "write fail";
                }
            }
        }

        void dump_subcribes ()
        {
            std::cout << "#### broker subscribers" << std::endl;          
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++) {
                std::cout << (*it).subscription_id << ", " << (*it).topic << ", " << (*it).fd << std::endl;          
            }
            std::cout << "####" << std::endl;          
        }

        void dump(char* buf, int len)
        {
            for(;len > 0; len--, buf++) {
                printf("%02hhX ", *buf);
            }
            printf("\n");
        }

        void read_event(int fd) 
        {
            #define READ_MAX 512
            char buf[READ_MAX];
            
            int ret = m_sock->read(fd, buf, sizeof buf);
            if( ret > 0 ) {
                emb_msg_header_t*  p_msg = (emb_msg_header_t*)buf;
                //dump(buf, ret);

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
                m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), fd), m_clients.end());
                m_sock->close(fd);
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

    private:
        int m_serverFd;
        std::vector<int>                m_clients;
        std::unique_ptr<EventLoop>      m_loop;
        std::unique_ptr<SocketStream>   m_sock;
        std::vector<EmbCommandItem>     m_dispatch_list;
        std::list<SuscribeItemBroker>   m_subscribers;
        std::string                     m_broker_id;
        int32_t                         m_last_subscription_id;
};

Broker::Broker()
{
    m_impl = std::make_unique<BrokerImpl>();
}

Broker::~Broker()
{
}

int32_t Broker::listen(std::string broker_id, ConnectionHandler on_listen, ConnectionHandler on_conn)
{
    return m_impl->listen(broker_id, on_listen, on_conn);    
}
void Broker::run(void)
{
    m_impl->run();
}

void Broker::stop(void)
{
    m_impl->stop();
}

void Broker::read_event(int fd)
{
    m_impl->read_event(fd);
}

}