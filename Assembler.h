#include<iostream>
#include<string>
#include<sstream>
#include<vector>
#include<fstream>
#include<stdexcept>
#include<map>

using namespace std;

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

class NullPointerException: public runtime_error{
public:
	NullPointerException(): runtime_error("Null Function Pointer!"){}
};


class Assembler{
private:
	typedef int(Assembler::*FP)(istringstream &);
	map<string, FP>	jumpTable;
public:
	Assembler();
	int assemble(fstream&, fstream&);

   	int loadi(istringstream &);      int store(istringstream &);
    	int add(istringstream &);	 int addi(istringstream &);
	int addc(istringstream &);	 int addci(istringstream &);
	int sub(istringstream &);	 int subi(istringstream &);
	int subc(istringstream &);	 int subci(istringstream &);
	int nd(istringstream &);         int ndi(istringstream &);
	int exor(istringstream &);	 int exori(istringstream &);
	int comp(istringstream &);	 int shl(istringstream &);
	int shla(istringstream &);	 int shr(istringstream &);
	int shra(istringstream &);	 int compr(istringstream &);
	int compri(istringstream &);	 int getstat(istringstream &);
	int putstat(istringstream &); 	 int jump(istringstream &);
	int jumpl(istringstream &);	 int jumpe(istringstream &);
	int jumpg(istringstream &);	 int call(istringstream &);
	int ret(istringstream &);	 int read(istringstream &);
	int write(istringstream &);	 int halt(istringstream &);
	int noop(istringstream &);	 int load(istringstream &);
	
//---------------------------------------------------------------------------

	

};

#endif

/*

*/
