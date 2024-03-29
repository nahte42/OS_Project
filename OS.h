#ifndef OS_H
#define OS_H

#include <vector>
#include <fstream>
#include <list>
#include <queue>
#include <string>

#include "VirtualMachine.h"
#include "Assembler.h"
#include "PT.h"

using namespace std;

class OS;

class PCB {
    int pc;
    int r[4];
    int ir;
    int sr;
    int sp;
    int base;
    int limit;
    PT pageTable;
	

    string prog;
    fstream in;
    fstream out;
    fstream stack;

    int io_completion;

    int turn_around_time;
    int cpu_time;
    int wait_time; int wait_time_begin;
    int io_time;   int io_time_begin;
	int pfnum = 10;
	int success = 0;
	
public:
    PCB(const string &p, const int &b, const int &l):
        prog(p), base(b), limit(l), pc(b), sr(2), sp(256/*vm.msize*/), 
        cpu_time(0), wait_time(0), io_time(0), wait_time_begin(0) { }
friend 
    class OS;
friend
    class VirtualMachine;
};




class OS {
    VirtualMachine vm;
    Assembler as;
    
	list<PCB *> pcb; // jobs
	list<PCB *> cpcb;
    queue<PCB *> readyQ;
    queue<PCB *> waitQ;
    PCB * running;
	vector<int> lruStack;
	queue<int> fifoStack;
    const static int TIME_SLICE = 15;
    const static int CONTEXT_SWITCH_TIME = 5;

    fstream progs;
    int idle_time;
    int sys_time; // total sys time
public:
    OS();
    ~OS();
    void run();
    void loadState();
    void saveState();
    void contextSwitch();
	void page_repl(int adpc);
	void lru(int victim);
	void fifo(int victim);
};





#endif
