#include <memory>
#include <string>

#include "broker.h"
#include <iostream>

using namespace std;

int main()
{
    std::unique_ptr<Broker> broker = std::make_unique<Broker>("/tmp/test");

    auto ret = broker->listen();
    if(ret < 0) {
        std::cout << "listen failed:" << std::endl;
        return -1;
    }

    broker->event_loop();

    return 0;
}
