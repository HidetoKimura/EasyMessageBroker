#include <memory>
#include <string>

#include "broker.h"

using namespace std;

int main()
{
    std::unique_ptr<Broker> broker = std::make_unique<Broker>("/tmp/test");

    broker->event_loop(-1);

    return 0;
}
