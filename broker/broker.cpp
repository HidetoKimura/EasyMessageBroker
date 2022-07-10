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
#include "event_util.h"
#include "ezlog.h"

using namespace std;

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

            EmbCommandItem command_item;
            command_item.command = EMB_MSG_COMMAND_PUBLISH;
            command_item.handler = [this](int fd, std::string command, void* head) -> void
            {
                emb_msg_t* msg = (emb_msg_t*)head;
                char* p_body = (char*)msg + sizeof(emb_msg_t);
                std::string topic = p_body;
                char* data = (char*)p_body + msg->topic_len;
                LOGI << "event =" << command << ", topic =" << topic << ", data =" << data;
                event_publish(topic, data, msg->data_len);
                dump_subcribes();

            };
            m_command_list.push_back(command_item);

            command_item.command = EMB_MSG_COMMAND_SUBSCRIBE;
            command_item.handler = [this](int fd, std::string command, void* head) -> void
            {
                emb_msg_t* msg = (emb_msg_t*)head;
                char* p_body = (char*)msg + sizeof(emb_msg_t);
                std::string topic = p_body;
                LOGI << "event =" << command << ", topic =" << topic;

                SuscribeItemBroker item;
                item.topic = topic;
                item.fd = fd;
                item.client_id = 10000 + m_subscribers.size();
                m_subscribers.push_back(item);

                event_suback(fd, topic, item.client_id);
                dump_subcribes();
            };
            m_command_list.push_back(command_item);

            command_item.command = EMB_MSG_COMMAND_UNSUBSCRIBE;
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

                event_unsuback(fd, topic, client_id_ul);

                dump_subcribes();
            };

            m_command_list.push_back(command_item);

            m_loop = std::make_unique<EventLoop>();

            EventLoopItem loop_item;
            loop_item.fd = m_serverFd;
            loop_item.dispatch = [this](int fd) -> void
            {
                int  conn_sock;
                sockaddr_un addr;
                socklen_t addrlen = 0;
                LOGD << "listen...";

                conn_sock = accept(m_serverFd,
                                    (struct sockaddr *) &addr, &addrlen);
                if (conn_sock == -1) {
                    LOGE << "accept";
                    return;
                }

                LOGD << "accept" << ", fd=" << conn_sock ;

                EventLoopItem conn_item;
                conn_item.fd = conn_sock;
                conn_item.dispatch = [this](int fd) -> void
                {
                    read_event(fd);
                };
                m_loop->add_item(conn_item);

                m_clients.push_back(conn_sock);

            };
            m_loop->add_item(loop_item);

        }

        ~BrokerImpl()
        {
            m_running = false;
            close(m_serverFd);

            unlink(m_broker_id.c_str());

            for (auto it = m_clients.begin(); it != m_clients.end(); it++)
            {
                close(*it);
            }
        }

        void event_suback(int fd, std::string topic, uint32_t client_id)
        {
            int ret, w_len;
            emb_msg_t msg;
            std::string client_id_str = std::to_string(client_id);

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len = topic.size() + 1;
            msg.data_len  = client_id_str.size() + 1;
            memcpy(msg.command, EMB_MSG_COMMAND_SUBACK, sizeof(msg.command));

            w_len = sizeof(msg);
            ret = write(fd, &msg, w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write topic */
            w_len = msg.topic_len;
            ret = write(fd, topic.c_str(), w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write message */
            w_len = msg.data_len;
            ret = write(fd, client_id_str.c_str(), w_len);
            if(ret != w_len) {
                perror("write fail");
            }

        }
        void dump_subcribes ()
        {
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++) {
                LOGD << (*it).client_id << ", " << (*it).topic << ", " << (*it).fd;          
            }

        }

        void event_unsuback(int fd, std::string topic, uint32_t client_id)
        {
            int ret, w_len;
            emb_msg_t msg;
            std::string client_id_str = std::to_string(client_id);

            msg.head_sign = EMB_MSG_HEAD_SIGN;
            msg.topic_len = 0;
            msg.data_len  = client_id_str.size() + 1;
            memcpy(msg.command, EMB_MSG_COMMAND_UNSUBACK, sizeof(msg.command));

            w_len = sizeof(msg);
            ret = write(fd, &msg, w_len);
            if(ret != w_len) {
                perror("write fail");
            }

            /* write message */
            w_len = msg.data_len;
            ret = write(fd, client_id_str.c_str(), w_len);
            if(ret != w_len) {
                perror("write fail");
            }
        }

        void event_publish(std::string topic, void *buf, int32_t len)
        {
            LOGD << (char*)buf;
            for (auto it = m_subscribers.begin(); it != m_subscribers.end(); it++)
            {
                int ret, w_len;
                emb_msg_t msg;

                LOGD << "broker_topic=" << (*it).topic << " request_topic=" << topic ;

                if((*it).topic != topic) continue;

                msg.head_sign = EMB_MSG_HEAD_SIGN;
                msg.topic_len = (*it).topic.size() + 1;
                msg.data_len  = len;
                memcpy(msg.command, EMB_MSG_COMMAND_PUBLISH, sizeof(msg.command));
 
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
            
            LOGD << "read_event fd=" << fd;

            int ret = read(fd, buf, sizeof buf);
            if( ret > 0 ) {
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
            else if (ret == 0) 
            {
                LOGI << "socket fd = " << fd << " disconnect."; 
                m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), fd), m_clients.end());
                close(fd);
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
        int m_serverFd;
        std::vector<int> m_clients;
        std::unique_ptr<EventLoop> m_loop;
        std::vector<EmbCommandItem>   m_command_list;
        std::list<SuscribeItemBroker> m_subscribers;
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

void Broker::event_loop(int count)
{
    m_impl->event_loop(count);
}


