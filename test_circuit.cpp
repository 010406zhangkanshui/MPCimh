
#include <iostream>  // std::cout
#include <functional> // std::bit_xor
#include "prg.hpp"


#include <iostream>
#include <vector>
#include "constant.h"
#include "mpio.hpp"
#include "prg.hpp"
#include "circuit.hpp"
#include <time.h>
#include <thread>
#include "program.hpp"
#include "dh-keyexchange.hpp"


int main(int argc,char **argv) {
   PRG p;
    const int SIZE = 5;
    bool array1[2][10] ;
    bool array2[2][10] ;
    for(int i=0;i<2;i++)
    {
        for(int j=0;j<10;j++)
        {
            array1[i][j]=p.rand()%2;
            array2[i][j]=p.rand()%2;

        }
    }
    cout<<""<<endl;
    for(int i=0;i<10;i++)
    {
        cout<<int(array1[1][i]);
        //cout<<int(array2[1][i]);
    }
    cout<<""<<endl;
    for(int i=0;i<10;i++)
    {
        //cout<<int(array1[1][i]);
        cout<<int(array2[1][i]);
    }
    cout<<""<<endl;

    // 使用 std::transform 进行逐元素异或，并将结果存回 array1
    std::transform(array1[1], array1[1] + SIZE, array2[1], array1[1], std::bit_xor<bool>());

    // 输出结果以验证异或操作是否成功
    for (int i = 0; i < 10; ++i) {
        std::cout  << int(array1[1][i]) ;
    }
    cout<<""<<endl;

/*
    int prover,port,secret;
    PRG p;
    sscanf(argv[1],"%d",&prover);
    sscanf(argv[2],"%d",&port);
    vector<string>ip;
    for(int i=0;i<=prover_num+1;i++)
        ip.push_back(string("127.0.0.1"));


    MPIO<RecIO,prover_num> *io=new MPIO<RecIO,prover_num>(prover,ip,port,true);//party是用户输入，假设n=3*8
    cout<<"MPIO connected"<<endl;
    Key_Ex<MPIO<RecIO,prover_num>> * DH = new Key_Ex<MPIO<RecIO,prover_num>> (io);
    cout<<"DH setup"<<endl;
    bool* send=new bool[padding_aes_length(100)];
    bool* recv=new bool[padding_aes_length(100)];
    for(int i=0;i<40;i++)
    {
        send[i] =p.rand()%2;
        cout<<int(send[i]);
    }
   /* cout<<""<<endl;
    if(prover==1)
    {
        DH->send(2,send,padding_aes_length(40));
        //DH->recv(2,recv,10);
    }
    if(prover==2)
    {
        DH->recv(1,recv,padding_aes_length(40));
        //DH->send(1,send,10);
    }

    for(int i=0;i<40;i++)
    {
        //recv[i] =p.rand()%2;
        cout<<int(recv[i]);
    }
    cout<<""<<endl;


    cout<<"initial....."<<endl;
    for(int i = 1;i<=prover_num;i++)//发
        {
            for(int j =1;j<=prover_num;j++)//收
            {
                if(i==j)continue;
                if(i==prover)
                { 
                    DH->send(j,send,padding_aes_length(40));  
                }
                if(j==prover)
                {
                    DH->recv(i,recv,padding_aes_length(40));
                }
            }
        }
        cout<<"recv"<<endl;
    //for(int i=1;i<n+1;i++)
    //{
        //cout<<"i is :"<<i<<endl;
        for(int j=0;j<40;j++)
        {
            cout<<(int)recv[j];
        }
        cout<<endl;
    //}




    delete send;
    delete recv;
    delete DH;
    delete io;*/
    //delete DH;
    //return 0;
}


