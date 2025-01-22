#ifndef CIRCUIT_HPP_
#define CIRCUIT_HPP_

//#include "gmw.hpp"
#include <iostream>
#include <vector>
#include "constant.h" 
#include "prg.hpp"

using namespace std;

class Bool{
public:
    bool val;
    Bool(bool b=0):val(b){

    }
};

//const int AND_GATE=0;
//const int XOR_GATE=1;
//const int NOT_GATE=2;
class Circuit { 
	public:
	int num_gate, num_wire, n1, n2, n3,num_andgate;
	my::PRG prg_n;
	char seed[Hash::DIGEST_SIZE/2];
	vector<int> gates;
	vector<Bool> pre_wires;
	vector<Bool> online_wires;
	vector<bool> abshare_andgate;
	bool* Inputwires_andgate_a;
	bool* Inputwires_andgate_b;
	bool* Outputwires_andgate_ab;
	
	Circuit(const char * file) {
		int tmp;
		FILE * f = fopen(file, "r");
		(void)fscanf(f, "%d%d\n", &num_gate, &num_wire);
		(void)fscanf(f, "%d%d%d\n", &n1, &n2, &n3);
		(void)fscanf(f, "\n");
		char str[10];
		gates.resize(num_gate*4);
		pre_wires.resize(num_wire);
		num_andgate=0;
		for(int i = 0; i < num_gate; ++i) {
			(void)fscanf(f, "%d", &tmp);
			if (tmp == 2) {//开头为2,则
				(void)fscanf(f, "%d%d%d%d%s", &tmp, &gates[4*i], &gates[4*i+1], &gates[4*i+2], str);
				if (str[0] == 'A') 
				{
					gates[4*i+3] = 0;
					num_andgate++;

				}
				else if (str[0] == 'X') gates[4*i+3] = 1;
			}
			else if (tmp == 1) {
				(void)fscanf(f, "%d%d%d%s", &tmp, &gates[4*i], &gates[4*i+2], str);
				gates[4*i+3] = 2;
			}
		}
		fclose(f);
		Inputwires_andgate_a = new bool[num_andgate];
		Inputwires_andgate_b = new bool[num_andgate];
		Outputwires_andgate_ab = new bool[num_andgate];
		memcpy(seed,prg_n.key,sizeof(prg_n.key));//保存种子
		
	}



	void pre_wire()
	{
		for(int i =0 ;i<n1;i++)
		{
			pre_wires[i] = prg_n.rand()%2;

		}
		//cout<<"n1+n2 : "<<n1<<endl;
		int and_count=0;
		for(int i = 0; i < num_gate; i++) {
		//	cout<<"num : "<<i<<endl;
        //    if(i%10000==0)printf("%d / %d\n",i,num_gate);
			if(gates[4*i+3] == AND_GATE) {
				
				///////加入andgate的两个输入////////////
				Inputwires_andgate_a[and_count]=pre_wires[gates[4*i]].val;
				Inputwires_andgate_b[and_count]=pre_wires[gates[4*i+1]].val;
				pre_wires[gates[4*i+2]] = prg_n.rand()%2;
				and_count++;
			}
			else if(gates[4*i+3] == XOR_GATE) {
				pre_wires[gates[4*i+2]] = pre_wires[gates[4*i]].val^ pre_wires[gates[4*i+1]].val;
			}
			else  
			{
				pre_wires[gates[4*i+2]] = pre_wires[gates[4*i]].val;
			}	
				
		}
		prg_n.random_bool(Outputwires_andgate_ab,num_andgate);
	}
	~Circuit()
	{
		delete[] Inputwires_andgate_a;
		delete[] Inputwires_andgate_b;
		delete[] Outputwires_andgate_ab;
	}
};

#endif