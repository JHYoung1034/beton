#include "../tools/Util/Logger.h"
#include "../tools/Util/Util.h"
#include "../tools/Util/Timer.h"
#include "../tools/threadpool/ThreadPool.h"
#include <signal.h>
#include <unistd.h>

using namespace std;
using namespace beton;

static semaphore sem;

int main_func(int argc, char *argv[]) {
    //initialize logger
    {
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

        Timer timer(2, []() {
            InfoL << "Hello from timer!" << endl;
            return 0;
        });

        signal(SIGINT, [](int sig) {
            signal(SIGINT, SIG_IGN);
            sem.notify();
        });
        sem.wait();
    }
    InfoL << "exiting..." << endl;
    sleep(1);
    return 0;
}

int main(int argc, char *argv[]) {
    return main_func(argc, argv);
}