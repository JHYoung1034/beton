#include <iostream>
#include "../tools/Util/Logger.h"
#include "../tools/Util/Util.h"
#include "../tools/threadpool/ThreadPool.h"
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace beton;

int main(int argc, char *argv[]) {
    //initialize logger
    Logger::Instance().add(make_shared<LogConsole>());
    Logger::Instance().add(make_shared<LogFile>());
    //initialize thread pool
    ThreadPool::initialize(0, 0, true);
    ThreadPool::Instance().getPoller()->async([]() {
        InfoL << "Hello from poller thread!" << endl;
    });
    ThreadPool::Instance().getThread()->async([]() {
        InfoL << "Hello from task thread!" << endl;
    });

    return 0;
}