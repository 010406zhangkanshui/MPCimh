#ifndef PROGRAM_HPP_
#define PROGRAM_HPP_


#include <iostream>
#include <vector>
#include "constant.h" 
#include "prg.hpp"

using namespace std;
#define AES_BLOCK_SIZE 16


void delete_3Darray(char ***& array, int dim1,int dim2)
{
    for (int i = 0; i < dim1; ++i) {
        for (int j = 0; j < dim2; ++j) {
            delete[] array[i][j];
        }
        delete[] array[i];
    }
    delete[] array;
    cout<<"delete 3D is over"<<endl;

}

void delete_4Darray(char **** & array,int dim1,int dim2,int dim3)
{

    for (int i = 1; i < dim1; ++i) {
        for (int j = 0; j < dim2; ++j) {
            for (int k = 1; k < dim3; ++k) {
                delete[] array[i][j][k];
            }
            delete[] array[i][j];
        }
        delete[] array[i];
    }
    delete[] array;
    cout<<"delete 4D is over"<<endl;
}

int  padding_aes_length(int origin_len)
{
    int block_num =  (origin_len+AES_BLOCK_SIZE-1)/AES_BLOCK_SIZE;
    
    int padding_length = block_num*AES_BLOCK_SIZE;
    return padding_length;
}

int compress_bool_to_byte(char *&out,vector<char> &src,int len)
{
    int bytes=(len+7)/8;
    for(int i=0;i<bytes;i++){
        *(out+i)=0;//初始化0
        for(int j=0;j<8&&i*8+j<len;j++){
            if(src[i*8+j])
                *(out+i)|=1<<j;//trans为1,则写入，但这里是逆序
        }
    }
    return bytes;
}

int compress_bool_to_byte(char *&out,bool* &src,int len)
{
    
    int bytes=(len+7)/8;
    for(int i=0;i<bytes;i++){
        *(out+i)=0;//初始化0
        for(int j=0;j<8&&i*8+j<len;j++){
            if(src[i*8+j])
                *(out+i)|=1<<j;//trans为1,则写入，但这里是逆序
        }
    }
    return bytes;

}


void byte_to_bool(const char * in,vector<char> &dst,int bool_len)//对应于compress_bool_to_byte
{
	int bytes=(bool_len+7)/8;
	//cout<<"bytes is "<<bytes<<endl;
	for(int i=0;i<bytes;i++)
	{//
		//cout<<"bytes "<<i<<endl;
		for(int j=0;j<8&&i*8+j<bool_len;j++)
		{
            dst[i*8+j]=(*(in+i))>>j&1;
        }
	}
	
}




void get_openparty(my::PRG & p,bool * &open_party)
{
    int count=0;
	vector<int> prover_count;
	prover_count.resize(prover_num,0);
    for(int i=0;i<prover_num*party_in_a_head+1;i++)
    {
        open_party[i]=0;
    }//清空

	for(int i=0;i<prover_num;i++)
	{
		count=0;
		while(count<party_in_a_head-1)
		{
			int open = p.rand()%party_in_a_head+1;
			if(open_party[i*party_in_a_head+open]==0)
			{
				open_party[i*party_in_a_head+open]=1;
				count++;
			}
			
		}
	}


	cout<<"open_party is : ";
	for(int i=1;i<party_in_a_head*prover_num+1;i++)
	{
		cout<<int(open_party[i]);
	}
	cout<<""<<endl;
}

void get_online_round(my::PRG &p,int run_num,bool* &run_round)
{
    int count=0;
    for(int i=0;i<REP;i++)
    {
        run_round[i]=0;
    }
    while(count<run_num)
    {
        int r=p.rand()%REP;
        if(run_round[r]==0)
        {
            run_round[r]=1;
            count++;
        }
    }
}

int get_numleaves(int underlying_leaves)
{
    int result = ceil(log2(underlying_leaves));
    
    //cout << "log2(" << underlying_leaves << ") = " << result << endl;
    int num_leaves =pow(2, result)-1+underlying_leaves;
    cout<<"all num_leaves is "<<num_leaves<<endl; 
    return num_leaves;

}

#endif
