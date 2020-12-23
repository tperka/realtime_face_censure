#include <iostream>
//#include <menu.h>

#include <unistd.h>
#include <ctime>

#include <sys/times.h>
#include <signal.h>
#include <sched.h>


#include <boost/interprocess/ipc/message_queue.hpp>
#include "names.hpp"


#define N_OF_SUBPROCESSES 3


using namespace std;

tms cpuStart, cpuEnd;
time_t realStart, realEnd;

void handleSIGINT(int sig)
{
    long realTime, cpuTime;
    double percentageOfTime;
    realEnd = times(&cpuEnd);
    realTime = (long)(realEnd - realStart);
    cpuTime = (long)(cpuEnd.tms_utime - cpuStart.tms_utime);
    percentageOfTime = (double)(cpuTime) / (double)(realTime) * 100.0;
    cout << "PID: " << getpid() << "\t percentage of CPU time: " << percentageOfTime << "%" << endl;
    exit(0);
}

void printAffinity(int pid) {
    cpu_set_t mask;
    if(sched_getaffinity(pid, sizeof(cpu_set_t), &mask) != 0) {
        cerr << "Error: could not get affinity" << endl;
        return;
    }
    int nOfProcs = sysconf(_SC_NPROCESSORS_ONLN);
    cout <<"PID: " << pid << "\t affinity = ";
    for(int i = 0; i < nOfProcs; ++i) {
        cout << CPU_ISSET(i, &mask) << " ";
    }
    cout << endl;
}

void printScheduling(int pid) {
    int result = sched_getscheduler(pid);
    if(result == -1) {
        cerr << "Error: could not get scheduler" << endl;
        return;
    }
    string toPrint;
    switch (result) {
        case SCHED_OTHER:
            toPrint = "Standard round-robin";
            break;
        case SCHED_BATCH:
            toPrint = "Batch";
            break;
        case SCHED_IDLE:
            toPrint = "Idle job";
            break;
        case SCHED_FIFO:
            toPrint = "FIFO";
            break;
        case SCHED_RR:
            toPrint = "Round-robin with priority";
            break;
        default:
            toPrint = "Error: unknown scheduling policy";
    }
    cout << "PID: " << pid << "\t scheduling: " << toPrint << endl;
    
}

void setScheduling(int pid, int policy, int priority = 0) {
        sched_param parameter;
        parameter.sched_priority = priority;
        if(sched_setscheduler(pid, policy, &parameter) == -1) {
            cerr << "Error: could not set scheduler" << endl;
        }
}

void setAffinity(int pid, int cpu) {
    if(cpu < 0 || cpu >= sysconf(_SC_NPROCESSORS_ONLN)) {
        cerr << "Error: CPU number can't be negative or equal/greater than number of available CPUs" << endl;    
        return;
    }

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    if(sched_setaffinity(pid, sizeof(cpu_set_t), &mask) != 0) {
        cerr << "Error: could not set affinity" << endl;
    }
}

void changeAffinityMenu(int childrenPids[]) {
    system("clear");
    
    char letter = 'A';
    for(int i = 0; i < N_OF_SUBPROCESSES; ++i) {
        cout << i+1 << ". Process " << (char)(letter+i) << " with ";
        printAffinity(childrenPids[i]);
    }
    cout << endl << "Choose process:" << endl;
    char option;
    while(!(cin >> option) || (int)(option - '0') < 1 || (int)(option - '0') > 3) {
        cin.ignore();
        cin.clear();
        cout << "Invalid option, please select 1-3" << endl;
    }
    cin.ignore();
    int pidToChange;
    switch (option)
    {
    case '1':
    case '2':
    case '3':
        pidToChange = childrenPids[(int)option - (int)'0' - 1];
        break;
    default:
        cout << "Invalid option, please select 1-3" << endl;
        break;
    }

    system("clear");
    printAffinity(pidToChange);
    cout << "Please insert core number to assign process to (integer in range 0-" << sysconf(_SC_NPROCESSORS_ONLN) - 1 << ")" << endl;
    int core;
    while(!(cin >> core)) {
        cin.clear();
        cin.ignore();
        cout << "Please input valid integer" << endl;
    }

    setAffinity(pidToChange, core);

}

void changeSchedulingMenu(int childrenPids[]) {
    system("clear");
    
    char letter = 'A';
    for(int i = 0; i < N_OF_SUBPROCESSES; ++i) {
        cout << i+1 << ". Process " << (char)(letter+i) << " with ";
        printScheduling(childrenPids[i]);
    }
    cout << "Choose process:" << endl;
    char option;
    while(!(cin >> option) || (int)(option - '0') < 1 || (int)(option - '0') > 3) {
        cin.ignore();
        cin.clear();
        cout << "Invalid option, please select 1-3" << endl;
    }
    cin.ignore();
    int pidToChange;
    switch (option)
    {
        case '1':
        case '2':
        case '3':
            pidToChange = childrenPids[(int)option - (int)'0' - 1];
            break;
        default:
            cout << "Invalid option, please select 1-3" << endl;
            break;
    }
    system("clear");
    printScheduling(pidToChange);

    int choosenPolicy = -1, priority = 0;
    cout << endl << "Choose scheduling option: " << endl << "1. SCHED_OTHER" << endl << "2. SCHED_BATCH"
        << endl << "3. SCHED_IDLE" << endl << "4. SCHED_FIFO" << endl << "5. SCHED_RR" << endl;
    
    while(!(cin >> option) || (int)(option - '0') < 1 || (int)(option - '0') > 5) {
        cin.ignore();
        cout << "Invalid option, please select 1-3" << endl;
    }
    cin.ignore();
    switch (option)
    {
        case '1':
            choosenPolicy = SCHED_OTHER;
            break;
        case '2':
            choosenPolicy = SCHED_BATCH;
            break;
        case '3':
            choosenPolicy = SCHED_IDLE;
            break;
        case '4':
            choosenPolicy = SCHED_FIFO;
            cout << "Please enter priority (a valid integer in range " << sched_get_priority_min(SCHED_FIFO) << "-" << sched_get_priority_max(SCHED_FIFO) << ")" << endl;
            while(!(cin >> priority)) {
                cin.clear();
                cin.ignore();
                cout << "Please input valid integer" << endl;
            }
            break;
        case '5':
            choosenPolicy = SCHED_RR;
            cout << "Please enter priority (a valid integer in range " << sched_get_priority_min(SCHED_RR) << "-" << sched_get_priority_max(SCHED_RR) << ")" << endl;
            while(!(cin >> priority)) {
                cin.clear();
                cin.ignore();
                cout << "Please input valid integer" << endl;
            }
            break;
        default:
            cout << "Invalid option, please select 1-5" << endl;
            break;
    }

    setScheduling(pidToChange, choosenPolicy, priority);

}




void changeCensureMenu(boost::interprocess::message_queue & mq){
    //system("clear");
    
    cout << "Enter censure mode: 0 for red rectangle, 1 for gaussian blur" << endl;
    int mode;
    while(!(cin >> mode) || (mode != 0 && mode != 1)) {
        cin.clear();
        cin.ignore();
        cout << "Please input valid mode" << endl;
    }
    mq.send(&mode, sizeof(mode), 0);
}

void changeFpsMenu(boost::interprocess::message_queue & mq){
    cout << "Enter upper fps cap: " << endl;
    int fps;
    while(!(cin >> fps) ) {
        cin.clear();
        cin.ignore();
        cout << "Please input valid integer" << endl;
    }
    mq.send(&fps, sizeof(fps), 0);
}

int main(int argc, char const *argv[])
{

    struct q_remover{
        q_remover(){ boost::interprocess::message_queue::remove(CENSURE_MODE_Q_NAME); }
        ~q_remover(){ boost::interprocess::message_queue::remove(CENSURE_MODE_Q_NAME); }
    } remover;

    struct fps_q_remover{
        fps_q_remover(){ boost::interprocess::message_queue::remove(FPS_Q_NAME); }
        ~fps_q_remover(){ boost::interprocess::message_queue::remove(FPS_Q_NAME); }
    } fps_remover;

    boost::interprocess::message_queue censure_mode_mq
         (boost::interprocess::create_only               //only create
         ,CENSURE_MODE_Q_NAME    //name
         ,10                        //max message number
         ,sizeof(int)               //max message size
         );
    boost::interprocess::message_queue fps_mq
         (boost::interprocess::create_only               //only create
         ,FPS_Q_NAME    //name
         ,10                        //max message number
         ,sizeof(int)               //max message size
         );

    signal(SIGINT, handleSIGINT);

    int childrenPids[N_OF_SUBPROCESSES];
    int pid;
    
    pid = fork();
    if(pid == 0) {
        char* args[] = {"./A.out", NULL};
        execvp(args[0], args);
        cerr << "Could not execv..." << endl;
    }
    childrenPids[0] = pid;

    sleep(1);

    pid = fork();
    if(pid == 0) {
        char* args[] = {"./B.out", NULL};
        execvp(args[0], args);
        cerr << "Could not execv..." << endl;
    }
    childrenPids[1] = pid;
    

    
    pid = fork();
    if(pid == 0) {
        char* args[] = {"./C.out", NULL};
        execvp(args[0], args);
        cerr << "Could not execv..." << endl;
    }
    childrenPids[2] = pid;

    
    while(true) {
        //system("clear");
        char option;
        cout << "Current affinity for all processes:" << endl;
        for(int i = 0; i < N_OF_SUBPROCESSES; ++i)
            printAffinity(childrenPids[i]);
        
        cout << "Current scheduling for all processes:" << endl;
        for(int i = 0; i < N_OF_SUBPROCESSES; ++i)
            printScheduling(childrenPids[i]);
        
        cout << "1. Change censure" << endl << "2. Change affinity" << endl << "3. Change scheduling" << endl << "4. Set fps cap" << endl << 
         "5. Exit" << endl;
        cin >> option;
        cin.ignore();
        switch(option) {
            case '1':
                changeCensureMenu(censure_mode_mq);
                break;
            case '2':
                changeAffinityMenu(childrenPids);
                break;
            case '3':
                changeSchedulingMenu(childrenPids);
                break;
            case '4':
                changeFpsMenu(fps_mq);
                break;
            case '5':
                for(int i = 0; i < N_OF_SUBPROCESSES; ++i) 
                    kill(childrenPids[i], SIGINT);
                return 0;
            default:
                cout << "Invalid option, please select 1-5" << endl;
                break;

        }
   }


    
    return 1;
}
