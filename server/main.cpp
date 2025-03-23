#include <iostream>
#include "../tools/Util/Logger.h"
#include <signal.h>

using namespace std;
using namespace beton;

int main(int argc, char *argv[]) {

    Logger::Instance().add(make_shared<LogConsole>());

    {
        LogDebug << "Hello logger!" << endl;

        static semaphore sem;
        signal(SIGINT, [](int) {
            LogInfo << "SIGINT:exit";
            signal(SIGINT, SIG_IGN);// 设置退出信号
            sem.notify();
        });// 设置退出信号

        sem.wait();
    }

    return 0;
}