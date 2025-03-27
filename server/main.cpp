#include <iostream>
#include "../tools/Util/Logger.h"
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace beton;

int main(int argc, char *argv[]) {
    Logger::Instance().add(make_shared<LogConsole>());
    {
        for (int i = 0; i < 100; i++) {
            DebugL << "Hello logger!";
            usleep(1000);
        }

        static semaphore sem;
        signal(SIGINT, [](int) {
            InfoL << "SIGINT:exit";
            signal(SIGINT, SIG_IGN);// 设置退出信号
            sem.notify();
        });// 设置退出信号

        sem.wait();
    }

    return 0;
}