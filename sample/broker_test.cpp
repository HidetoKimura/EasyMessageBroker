#include <memory>
#include <string>

#include "emb.h"
#include <iostream>

using namespace ez::stream;
using namespace emb;

int main()
{
    std::unique_ptr<Broker> broker = std::make_unique<Broker>("/tmp/test");

    auto ret = broker->listen();
    if(ret < 0) {
        std::cout << "listen failed:" << std::endl;
        return -1;
    }

    broker->run();

    return 0;
}
