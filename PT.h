#ifndef PT_H
#define PT_H
#include <iostream>
#include <vector>

using namespace std;

// forward declarations
class OS;
class VirtualMachine;

class Row
{
    int frame;
    bool valid;
    bool modified;

friend
    class PT;

friend
    class OS;

friend
	class VirtualMachine;
};

class PT
{
    vector<Row> page_table;

public:
    PT() {
		
		page_table.reserve(8); 
		for (int i = 0; i < 8; i++) 
        {
            page_table[i].valid = false;
            page_table[i].modified = false;
        }
			
	}
    PT(int size)
    {
		
        page_table.reserve(size);
		
        for (int i = 0; i < size; i++) 
        {
            page_table[i].valid = false;
            page_table[i].modified = false;
        }
    }
    void operator=(const PT & p)
    {
        page_table = p.page_table;
    }
    void operator=(PT && p)
    {
        swap(page_table, p.page_table);
    }
    int log_to_phys(int addr)
    {
        //assuming 8 word pages
        int page_num = addr/8;
        if (page_table[page_num].valid) {
            int frame = page_table[page_num].frame;
            int phys_addr = frame*8 + addr%8;
            return phys_addr;
        } else // page fault
            return -1;
    }

friend
    class OS;

friend
    class VirtualMachine;
};

#endif
