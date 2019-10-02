#ifndef VIRTUALMACHINE_H
#define VIRTUALMACHINE_H

#include <queue>
#include <vector>
#include <fstream>
#include "PT.h"

using namespace std;

class VirtualMachine {
	int msize;
	int rsize;
	int pc, ir, sr, sp, clock, spc;
	vector<int> mem;
	vector<int> r;
	int base, limit;
	int total_limit;
	PT TLB;
	fstream ob;
	string curProg;
	vector<int> lruStack;
	queue<int> fifoStack;
public:
	VirtualMachine(): msize(256), rsize(4), clock(0) { mem.reserve(msize); r.reserve(rsize);}
	void load(fstream&, int base, int & limit);
	void load(fstream&, int base);
	void run(int, fstream &, fstream &);
	void page_repl(int adpc);
	void lru();
	void fifo();
    	
friend
	class OS;
}; // VirtualMachine

#endif
