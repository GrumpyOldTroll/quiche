#include <iostream>
#include <fstream>
#include <string>
#include "Top.pb.h"
using namespace std;

int main()
{
    ServerPushInterface::Top cmd;

    auto &activate_pool = *cmd.mutable_activate_pool();

    activate_pool.set_pool_id(42);

    std::cout << cmd.DebugString() << std::endl;

    cmd.SerializeToOstream(&std::cerr);
}