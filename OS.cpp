#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <cassert>

#include "OS.h"
#include "VirtualMachine.h"
#include "Assembler.h"

using namespace std;

OS::OS()
{
    idle_time = 0;
    sys_time = 0;

    system("ls *.s > progs");
    progs.open("progs", ios::in);
    assert(progs.is_open());

    int base=0, limit;
    string prog;
    while (progs >> prog) {
        fstream src, obj;
        int pos = prog.find(".");
        string prog_name = prog.substr(0, pos);

        src.open(prog.c_str(), ios::in);
        obj.open((prog_name+".o").c_str(), ios::out);
        assert(src.is_open() and obj.is_open());
		
		
        if (as.assemble(src, obj)) {
            cout << "Assembler Error in " << prog << "\n";
            src.close();
            obj.close();
            continue;
        }
        src.close();
        obj.close();
        obj.open((prog_name+".o").c_str(), ios::in);
        assert(obj.is_open());
        vm.load(obj, base, limit);
        obj.close();
		vm.TLB = PT((limit/8)+1);

        PCB * job = new PCB(prog_name, base, limit-base);
		job->pageTable = vm.TLB;
        job->in.open((prog_name+".in").c_str(), ios::in);
        job->out.open((prog_name+".out").c_str(), ios::out);
        job->stack.open((prog_name+".st").c_str(), ios::in | ios::out);
        assert((job->in).is_open() and (job->out).is_open() and (job->stack).is_open());

        pcb.push_back(job);
        base = limit;
    }
    vm.total_limit = limit;

    for (list<PCB *>::iterator i = pcb.begin(); i != pcb.end(); i++)
        readyQ.push(*i);
}

OS::~OS()
{
    list<PCB *>::iterator i;

    int cpu_time = 0;
    for (i = pcb.begin(); i != pcb.end(); i++)
        cpu_time += (*i)->cpu_time;

    for (i = pcb.begin(); i != pcb.end(); i++) {
        (*i)->out << "Turn around time = " << (*i)->turn_around_time << ", CPU time = " << (*i)->cpu_time
              << ", Wait time = " << (*i)->wait_time << ", IO time = " << (*i)->io_time << endl;

        (*i)->out << "Total CPU time = " << cpu_time << ", System time = " << sys_time
              << ", Idle time = " << idle_time << ", Final clock = " << vm.clock << endl
              << "Real CPU Utilization = " << setprecision(3) << (float) cpu_time / vm.clock * 100 << "%, "
              << "System CPU Utilization = " << (float) (vm.clock - idle_time) / vm.clock * 100 << "%, "
              << "Throughput = " << pcb.size() / ((float) vm.clock / 1000) << endl << endl
			  <<"Page Faults = "<<(*i)->pfnum<<endl
			  <<"Hit Ratio = "<<setprecision(3)<<(float)((*i)->success/(((*i)->pfnum) + ((*i)->success)))*100<<"% ";

        (*i)->in.close();
        (*i)->out.close();
        (*i)->stack.close();
    }
    progs.close();
}

void OS::run()
{
    int time_stamp;

    running = readyQ.front();
    readyQ.pop();

    while (running != 0) {
        vm.clock += CONTEXT_SWITCH_TIME;
        sys_time += CONTEXT_SWITCH_TIME;
        loadState();

        time_stamp = vm.clock;
		running->pageTable.page_table[0].frame;
        vm.run(TIME_SLICE, running->in, running->out);
        running->cpu_time += (vm.clock - time_stamp);

        contextSwitch();
    }
}

void OS::contextSwitch() 
{
    while (not waitQ.empty() and waitQ.front()->io_completion <= vm.clock) {
        readyQ.push(waitQ.front());
        waitQ.front()->wait_time_begin = vm.clock;
        waitQ.front()->io_time += (vm.clock - waitQ.front()->io_time_begin);
        waitQ.pop();
    }

    int vm_status = (vm.sr >> 5) & 07;
	int page_fault = (vm.sr >> 10) & 1;
    switch (vm_status) {
        case 0: //Time slice
            readyQ.push(running);
            running->wait_time_begin = vm.clock;
            break;
        case 1: //Halt
            running->out << running->prog << ": Terminated\n";
            running->turn_around_time = vm.clock;
            break;
        case 2: //Out of Bound Error
            running->out << running->prog << ": Out of bound Error, pc = " << vm.pc << endl;
            running->turn_around_time = vm.clock;
            break;
        case 3: //Stack Overflow
            running->out << running->prog << ": Stack overflow, pc = " << vm.pc << ", sp = " << vm.sp << endl;
            running->turn_around_time = vm.clock;
            break;
        case 4: //Stack Underflow
            running->out << running->prog << ": Stack underflow, pc = " << vm.pc << ", sp = " << vm.sp << endl;
            running->turn_around_time = vm.clock;
            break;
        case 5: //Bad Opcode
            running->out << running->prog << ": Bad opcode, pc = " << vm.pc << endl; 
            running->turn_around_time = vm.clock;
            break;
        case 6: //Read
        case 7: //Write
            waitQ.push(running);
            running->io_completion = vm.clock + 27;
            running->io_time_begin = vm.clock;
            break;
        default:
            cerr << running->prog << ": Unexpected status = " << vm_status 
                 << " pc = " << vm.pc << " time = " << vm.clock << endl;
            running->out << running->prog << ": Unexpected status: " << vm_status 
                 << " pc = " << vm.pc << " time = " << vm.clock << endl;
            running->turn_around_time = vm.clock;
    }
	
	if(page_fault){
		page_repl(running->pc);
		running->pfnum++;
	}
	else{
		running->success++;
	}
	
    saveState();
    running = 0;
    if (not readyQ.empty()) {
        running = readyQ.front();
        running->wait_time += (vm.clock - running->wait_time_begin);
        readyQ.pop();
    } 
    else if (not waitQ.empty()){
        running = waitQ.front();
        waitQ.pop();
        idle_time += (running->io_completion - vm.clock);
        vm.clock = running->io_completion;
        // assume all of context switch time is incurred after idle time
        running->io_time += (vm.clock - running->io_time_begin);
    }
}

void OS::loadState()
{
    vm.curProg = running->prog;
	vm.pc = running ->pc;
    for (int i = 0; i < 4; i++)
        vm.r[i] = running->r[i];
    vm.ir = running->ir;
    vm.sr = running->sr;
    vm.base = running->base;
    vm.limit = running->limit;
    vm.sp = running->sp;
	vm.TLB = running->pageTable;
    running->stack.seekg(0, ios::beg);
    for (int i = vm.sp; i < vm.msize and not running->stack.fail(); i++)
        running->stack >> vm.mem[i];
    assert(not running->stack.fail());
}

void OS::saveState()
{
    running->pc = vm.pc;
    for (int i = 0; i < 4; i++)
        running->r[i] = vm.r[i];
    running->ir = vm.ir;
    running->sr = vm.sr;
    running->base = vm.base;
    running->limit = vm.limit;
    running->sp = vm.sp;
    running->stack.seekp(0, ios::beg);
	running->pageTable = vm.TLB;
    for (int i = vm.sp; i < vm.msize; i++)
        running->stack << vm.mem[i] << endl;
}  


void OS::page_repl(int adpc){

	int pIndex = adpc/8;
	
	running -> pageTable.page_table[pIndex].valid = true;
	for(int i = 0; i < 32; i++){
		if(vm.mem[i*8] == 0){ // There is an empty frame
			progs.open((running->prog+".o").c_str(), ios::in);
			vm.load(progs, (i*8));
			progs.close();
			return;
		}
	}
	//No empty frame, need to figure out a victim
	for(int i = 0 ; i < lruStack.size(); i++){
		if(i == lruStack.size()-1)
			lruStack.push_back(pIndex);
		if(lruStack[i] == pIndex){
			for(int j = i; j < lruStack.size()-1; j++)
				lruStack[j] = lruStack[j+1];
			lruStack[lruStack.size()-1] = pIndex;
		}
	}	
	fifoStack.push(pIndex);
	
	int victim = lruStack[0];
	for(int i = 0; i < lruStack.size()-1; i++){
		lruStack[i] = lruStack[i+1];
	}
	lruStack.pop_back();
	//int victim = fifoStack.front();
	//fifoStack.pop();
	lru(victim);
	//fifo(victim);
}

void OS::lru(int victim){
	 int inst;
	 list<PCB *>::iterator i = pcb.begin();
	 for (i; i != pcb.end(); i++)
	 {
		progs.open(((*i)->prog+".o").c_str(), ios::in);
		while(progs>>inst){
			if(vm.mem[((*i)->pageTable.page_table[victim].frame)*8] == inst){
				(*i)->pageTable.page_table[victim].valid = false;
				break;
			}
		}
		progs.close();
	 }
	 progs.open(((running->prog)+".o").c_str(), ios::in);
	 vm.load(progs, ((*i)->pageTable.page_table[victim].frame)*8);
	 progs.close();
	 running -> pageTable.page_table[running->pc/8].valid = true;
}

void OS::fifo(int victim){//I Apologize, but my brain is fried
	 int inst;
	 list<PCB *>::iterator i = pcb.begin();
	 for (i; i != pcb.end(); i++)
	 {
		progs.open(((*i)->prog+".o").c_str(), ios::in);
		while(progs>>inst){
			if(vm.mem[((*i)->pageTable.page_table[victim].frame)*8] == inst){
				(*i)->pageTable.page_table[victim].valid = false;
				break;
			}
		}
		progs.close();
	 }
	 progs.open(((running->prog)+".o").c_str(), ios::in);
	 vm.load(progs, ((*i)->pageTable.page_table[victim].frame)*8);
	 progs.close();
	 running -> pageTable.page_table[running->pc/8].valid = true;
	
}