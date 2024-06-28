#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    std::cout << "smash: got ctrl-C" << std::endl;
    int pid = read_from_config();
    if (pid > 0) {
        std::cout << "smash: process " << pid << " was killed." << std::endl;
        kill(pid, 9);
    }
}
