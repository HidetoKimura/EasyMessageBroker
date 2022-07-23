#include <memory>
#include <string>

#include "emb.h"
#include <iostream>

using namespace ez::stream;
using namespace emb;

int main()
{
    std::unique_ptr<Broker> broker = std::make_unique<Broker>();

    auto ret = broker->listen("/tmp/test", 
                [](int new_fd){ std::cout << "new listen fd = " << new_fd << std::endl; },
                [](int new_fd){ std::cout << "new broker fd = " << new_fd << std::endl; });
    if(ret < 0) {
        std::cout << "listen failed:" << std::endl;
        return -1;
    }

    broker->run();

    return 0;
}
