#include "RecIO.hpp" 
//#include "gmw.hpp"
#include <iostream>
#include <vector>
#include "constant.h"
#include "program.hpp"
#include <time.h>
//#include "ot.hpp"
#include "dh-keyexchange.hpp"
#include "mpio.hpp"
#include <thread>


void recv_message(int prover,Key_Ex<MPIO<RecIO,prover_num+1>> * &DH, bool*** &tmp,bool**&pre_all_a,bool**&pre_all_b,bool**&pre_all_ab,int andgate_num)
{
    //tmp[prover_num][REP][andgate_num]
    //pre_all_a[REP][andgate_num];
    cout<<"enter recv from "<<prover<<endl;
    for(int j=0;j<REP;j++)
    {
        DH->recv(prover,tmp[prover-1][j],padding_aes_length(andgate_num));
        
    }
    
    for(int j=0;j<REP;j++)
    {
        std::transform(pre_all_a[j],pre_all_a[j]+andgate_num , tmp[prover-1][j], pre_all_a[j], std::bit_xor<bool>());
    }
         
        //DH->recv(i,tmp,and_cnt*REP);
    for(int j=0;j<REP;j++)
    {
        DH->recv(prover,tmp[prover-1][j],padding_aes_length(andgate_num));
    }
    
    for(int j=0;j<REP;j++)
    {
        std::transform(pre_all_b[j],pre_all_b[j]+andgate_num , tmp[prover-1][j], pre_all_b[j], std::bit_xor<bool>());
    }
 
    for(int j=0;j<REP;j++)
    {
        DH->recv(prover,tmp[prover-1][j],padding_aes_length(andgate_num));
    }
        
        //DH->recv(i,tmp,and_cnt*REP);
    for(int j=0;j<REP;j++)
    {
        std::transform(pre_all_ab[j],pre_all_ab[j]+andgate_num , tmp[prover-1][j], pre_all_ab[j], std::bit_xor<bool>());
    }    

    cout<<"finish recv from "<<prover<<endl;
}
 
using namespace std; 
int main(int argc,char **argv){
    if(argc!=3){
        puts("./main <party> <port>");
        return 0;
    }
    int third_party,port;
    sscanf(argv[1],"%d",&third_party);
    sscanf(argv[2],"%d",&port);
 
    vector<string>ip;
    for(int i=0;i<=prover_num+1;i++)
        ip.push_back(string("127.0.0.1"));
 

 
    MPIO<RecIO,prover_num+1> *io=new MPIO<RecIO,prover_num+1>(third_party,ip,port,false);//party是用户输入，假设n=3*8
    cout<<"MPIO connected"<<endl;
    Key_Ex<MPIO<RecIO,prover_num+1>> * DH = new Key_Ex<MPIO<RecIO,prover_num+1>> (io);
    cout<<"DH setup"<<endl;
 
    bool** pre_all_a;
    bool** pre_all_b;
    bool** pre_all_ab;
    bool** aux;
    bool*** tmp;
     
    int REP;
    int andgate_num;
     
    io->recv_data(1,&andgate_num,sizeof(andgate_num));
    io->recv_data(1,&REP,sizeof(REP));
    cout<<"recv num"<<endl;
    cout<<"REP is :"<<REP<<endl;
    cout<<"andgate_num is :"<<andgate_num<<endl;
    pre_all_a = new bool*[REP];
    pre_all_b = new bool*[REP];
    pre_all_ab = new bool*[REP];
    tmp = new bool**[prover_num];
    aux = new bool*[REP];
    for(int i=0;i<prover_num;i++)
    {
        tmp[i] = new bool*[REP];
        for(int j=0;j<REP;j++)
        {
            tmp[i][j] = new bool[padding_aes_length(andgate_num)];
        }
    }
    for(int i=0;i<REP;i++)
    {
        pre_all_a[i]=new bool[padding_aes_length(andgate_num)]();
        pre_all_b[i]=new bool[padding_aes_length(andgate_num)]();
        pre_all_ab[i]=new bool[padding_aes_length(andgate_num)]();
        aux[i]=new bool[padding_aes_length(andgate_num)]();
        //tmp[i]=new bool[padding_aes_length(andgate_num)];
    }

    //std::vector<std::thread> threads;
   /* for (int j = 1; j <= prover_num; j++) {
        threads.push_back(std::thread(recv_message, j, std::ref(DH),ref(tmp),ref(pre_all_a),ref(pre_all_b),ref(pre_all_ab) ));
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    threads.clear();*/
    for (int j = 1; j <= prover_num; j++) {
       recv_message(j,DH,tmp,pre_all_a,pre_all_b,pre_all_ab,andgate_num);
    }
    cout<<"recv msg is over"<<endl;
    //
    for(int i=0;i<prover_num;i++){
        for(int j=0;j<REP;j++)
        {
            delete[] tmp[i][j];
        }
    }
    delete[] tmp;

//////////////////////////
    bool r;
    for(int i=0;i<REP;i++)
    {
        for(int j=0;j<andgate_num;j++)
        {
            r = (pre_all_a[i][j]&pre_all_b[i][j])^pre_all_ab[i][j];
            aux[i][j]=r; 
        }
    }
    //Key_Ex<MPIO<RecIO,n+1>> * DH1 = new Key_Ex<MPIO<RecIO,n+1>> (io);
    cout<<"aux gen is over"<<endl;
    for(int i=0;i<REP;i++)
    {
        DH->send(prover_num,aux[i],padding_aes_length(andgate_num));
    }
    cout<<"aux send is over"<<endl;
    delete DH;
    delete io;
    cout<<"delete io is over"<<endl;
    for(int i=0;i<REP;i++)
    {
        delete[] pre_all_a[i];
        delete[] pre_all_b[i];
        delete[] pre_all_ab[i];
        delete[] aux[i];
    }
    cout<<"delete a,b,ab is over"<<endl;
    delete[] aux;
    delete[] pre_all_a;
    delete[] pre_all_b;
    delete[] pre_all_ab;
 
    return 0;
    }