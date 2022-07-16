
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
    emb_id_t        client_id;
    std::string     topic;
    int             fd;
} SuscribeItemBroker;

class BrokerImpl
{
    public :
        BrokerImpl(std::string broker_id)
          : m_serverFd(-1)
          , m_clients()
          , m_subscribers()
        {
            m_loop = std::make_unique<EventLoop>();
            m_sock = std::make_unique<SocketStream>(broker_id);
        }

        ~BrokerImpl()
        {
            for (auto it = m_clients.begin(); it != m_clients.end(); it++)
            {
                m_sock->close(*it);
            }
        }

        int32_t listen(void)
        {

            m_serverFd = m_sock->listen();
            if(m_serverFd < 0) 
            {
                LOGE << "listen failed: ";
                return -1; 
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
                std::string data(msg->data);

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
                item.client_id = 10000 + m_subscribers.size();
                m_subscribers.push_back(item);

                send_suback(fd, topic, item.client_id);
                
                dump_subcribes();
                return;
            };
            m_dispatch_list.push_back(dispatch_item);

            dispatch_item.type = EMB_MSG_TYPE_UNSUBSCRIBE;
            dispatch_item.handler = [this](int fd, void* recv_msg) -> void
            {
                emb_msg_UNSUBSCRIBE_t* msg = (emb_msg_UNSUBSCRIBE_t*)recv_msg;

                if( (msg->header.len    != sizeof(emb_msg_UNSUBSCRIBE_t) ) ||
                    (msg->client_id_len != sizeof(msg->client_id)) )
                {
                    LOGE << "length error";
                    return;                    
                }

                LOGI << "event =" << msg->header.type << ", clinet_id =" << msg->client_id;
                
                send_unsuback(fd, msg->client_id);

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

            m_dispatch_list.push_back(dispatch_item);

            EventLoopItem loop_item;
            loop_item.fd = m_serverFd;
            loop_item.dispatch = [this](int fd) -> void
            {
                int  conn_sock;

                conn_sock = m_sock->accept(fd);
                if (conn_sock < 0) {
                    LOGE << "accept failed:";
                    return;
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

        void send_suback(int fd, std::string topic, uint32_t client_id)
        {
            emb_msg_SUBACK_t msg;

            if(topic.size() > sizeof(msg.topic) - 1) {
                LOGE << "bad topic";
            }

            msg.topic_len = sizeof(msg.topic);
            memset(msg.topic, 0, msg.topic_len);
            strncpy(msg.topic, topic.c_str(), topic.size()); 

            msg.client_id_len = sizeof(msg.client_id);
            msg.client_id = client_id;

            msg.header.type = EMB_MSG_TYPE_SUBACK;
            msg.header.len = sizeof(msg);

            int32_t w_len = msg.header.len;
            int32_t ret = m_sock->write(fd, &msg, w_len);
            if(ret != w_len) {
                LOGE << "write fail";
            }
            return;
        }

        void send_unsuback(int fd, uint32_t client_id)
        {
            emb_msg_UNSUBACK_t msg;

            msg.client_id_len = sizeof(msg.client_id);
            msg.client_id = client_id;

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
            emb_id_t tmp_client_id = msg->client_id;

            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
            {
                if((*it).topic != topic) continue;
                if(tmp_client_id != EMB_ID_BROADCAST) {
                    if((*it).client_id != tmp_client_id) continue;
                }
                msg->client_id = (*it).client_id;
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
                std::cout << (*it).client_id << ", " << (*it).topic << ", " << (*it).fd << std::endl;          
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
        std::string m_broker_id;
};

Broker::Broker(std::string broker_id)
{
    m_impl = std::make_unique<BrokerImpl>(broker_id);
}

Broker::~Broker()
{
}

int32_t Broker::listen(void)
{
    return m_impl->listen();    
}
void Broker::run(void)
{
    m_impl->run();
}

void Broker::stop(void)
{
    m_impl->stop();
}

}