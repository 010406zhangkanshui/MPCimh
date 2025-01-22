
#include <emp-tool/emp-tool.h>
#include "emp-ot/emp-ot.h"
#include "RecIO.hpp" 
#include "mpio.hpp"
//#include "gmw.hpp"
#include <iostream>
#include <vector>
#include "constant.h"
#include "program.hpp"
#include "prg.hpp"
#include "circuit.hpp"
#include <time.h>
#include <thread>
//#include "ot.hpp"
#include "seed_binary_tree.hpp"
//#include "dh-keyexchange.hpp"
#include <algorithm>


using namespace std; 

int prover,port,secret;
int andgate_num,input_len,output_len;
char filename[25];

void broadcast_commit(char **& commit,MPIO<RecIO,prover_num>* &io)
{
    for(int i=1;i<=prover_num;i++)
    {
        for(int j=1;j<=prover_num;j++)
        {
            if(i==j) continue;
            if(i==prover)
            {
                for(int k=0;k<party_in_a_head;k++)
                {
                    io->send_data(j,commit[(prover-1)*party_in_a_head+k+1],32);
                }   
            }
            if(j==prover)
            {
                for(int k=0;k<party_in_a_head;k++)
                {
                    io->recv_data(i,commit[(i-1)*party_in_a_head+k+1],32);
                }

            }
        }
    }
    my::Hash h;
    h.reset();
    for(int i=1;i<=prover_num*party_in_a_head;i++)
    {
        h.put(commit[i],32);
    }
    h.digest(commit[0]);
}


//https://link.springer.com/chapter/10.1007/978-3-662-48797-6_29/figures/9
template <typename T>
void beaver_triple_send(T * ot, NetIO *io,bool* res,bool* y,int64_t length)
{
    //sender - Alice
    block *b0 = new block[length];
	block *b1 = new block[length];
    bool* s = new bool[length];
    ot->send_rot(b0,b1,length);
    //cout<<"rot sended"<<endl;
   
    for(int i=0;i<length;i++)
    {
        res[i] ^= getLSB(b0[i]);//res是v_0h(j,i)
        s[i]=y[i]^getLSB(b0[i])^getLSB(b1[i]);
    }
    //cout<<"begin to send bool"<<endl;
    io->send_bool(s,length);
    io->flush();
    //cout<<"exit triple send"<<endl;
    delete[] b0;
    delete[] b1;
    delete[] s;
}

template <typename T>
void beaver_triple_receive(T * ot, NetIO *io,bool* res,bool* x,int64_t length)
{   //x is choice bit
    //receiver - Bob
	block *r = new block[length];
    bool* s = new bool[length];
    
    ot->recv_rot(r,x,length);

    //cout<<"rot_recvd"<<endl;

    io->recv_bool(s,length);
    io->flush();
    //cout<<"bool recvd"<<endl;
    for(int i=0;i<length;i++)
    {
        //s[i]=getLSB(r)^(s[i]&x[i]);//get n
        res[i] ^= getLSB(r[i])^(s[i]&x[i]);//res是n_h(i,j)
    }
    //cout<<"exit triple receive"<<endl;
    delete[] r;
    delete[] s;
	

}

void send_to_TA(MPIO<NetIO,prover_num> *&io_pre,
FerretCOT<NetIO> * ferret_send[prover_num+1],
FerretCOT<NetIO> * ferret_recv[prover_num+1],
bool ** (&pre_all_a),
bool **(&pre_all_b),
bool **(&pre_all_ab),
bool **(& aux))
{
    cout<<"aux initial=========="<<endl;
    for(int j=0;j<10;j++)
    {
        cout<<aux[0][j];
    }
    for(int i=0;i<REP;i++)
    {
        for(int j=0;j<andgate_num;j++)
        {
            aux[i][j] = pre_all_a[i][j]&pre_all_b[i][j];
        }
    }
    cout<<""<<endl;
    for(int i=1;i<=prover_num;i++)//i，j循环，如果i是party则send，j是party则recv
        for(int j=1;j<=prover_num;j++)if(i!=j){
            if(i==prover){
                cout<<" send to "<<j<<endl;
                for(int k=0;k<REP;k++)
                {
                    
                    //test_ot<FerretCOT<emp::NetIO>>(ferret_send[j], io_pre->send_io[j], 1, 2000);//bob
                    //test_ot<IKNP<NetIO>>(ferret_send[j], io_online->send_io[j], 2, 20000);//bob
                    beaver_triple_send(ferret_send[j],io_pre->send_io[j],aux[k],pre_all_b[k],andgate_num); 
                }
            
            }
            if(j==prover){
                cout<<"received from "<<i<<endl;
                for(int k=0;k<REP;k++)
                {
                    //test_ot<FerretCOT<emp::NetIO>>(ferret_recv[i], io_pre->recv_io[i], 2, 2000);//alice
                    beaver_triple_receive(ferret_recv[i],io_pre->recv_io[i],aux[k],pre_all_a[k],andgate_num);
                }
        }
    }
    cout<<"aux before====》》》》》》"<<endl;
    for(int i=0;i<REP;i++)
    {
        cout<<"REP======"<<i<<endl;
        for(int j=0;j<10;j++)
        {
            cout<<aux[i][j];
        }
        cout<<""<<endl;
    }
    for(int i=0;i<REP;i++)
    {
        for( int j=0;j<andgate_num;j++)
        {
            aux[i][j] ^= pre_all_ab[i][j];
        }
    }

    if(prover != prover_num)
    {
        for(int i=0;i<REP;i++)
        {
            io_pre->send_io[prover_num]->send_bool(aux[i],andgate_num);
        }
        

    }else{
        bool tmp_aux[andgate_num];
        for(int i=1;i<prover_num;i++)
        {
            for(int j=0;j<REP;j++)
            {
                io_pre->recv_io[i]->recv_bool(tmp_aux,andgate_num);
                for(int k=0;k<andgate_num;k++)
                {
                    aux[j][k]^=tmp_aux[k];
                }
            }

        }

    }

        for(int i=0;i<REP;i++)
        {
            cout<<"REP is"<<i<<endl;
            cout<<"pre_a"<<endl;
            for(int j=0;j<10;j++)
            {
                cout<<pre_all_a[i][j];
            }
            cout<<""<<endl;
            cout<<"pre_b"<<endl;
            for(int j=0;j<10;j++)
            {
                cout<<pre_all_b[i][j];
            }
            cout<<""<<endl;
            cout<<"pre_ab"<<endl;
            for(int j=0;j<10;j++)
            {
                cout<<pre_all_ab[i][j];
            }
            cout<<""<<endl;
            if(prover==prover_num)
            {
                cout<<"aux"<<endl;
                for(int j=0;j<10;j++)
                {
                    cout<<aux[i][j];
                }
                cout<<""<<endl;
            }

        }
    
    


}
void gen_pre_commit(MPIO<RecIO,prover_num> *&io,bool** aux,vector<vector<vector<char>>> seed,char *** &pre_commit)
{
    my::Hash h_pre;
    //——precommit
    //最后一个用来放总的 pre_commit [REP+1][prover_num*party_in_a_head+1][32];
    // seed //[REP][party_in_a_head][32]
    for(int i=0;i<REP;i++)
    {
        cout<<"-=--=-=-REP"<<i<<endl;
        for(int j=0;j<party_in_a_head;j++)
        {
            h_pre.reset();
            h_pre.put(seed[i][j].data(),32);
            if(prover==prover_num&&j==(party_in_a_head-1))
            {
                h_pre.put(aux[i],andgate_num);
            }
            h_pre.digest(pre_commit[i][(prover-1)*party_in_a_head+j+1]);
        }
        broadcast_commit(pre_commit[i],io);
    }
    cout<<"broadcast commit----"<<endl;

    h_pre.reset();
    for(int i=0;i<REP;i++)
    {
        cout<<"REP is "<<i<<"pre_commit"<<endl;
        h_pre.put(pre_commit[i][0],32);
        for(int j=0;j<32;j++)
        {
            cout<<int(pre_commit[i][0][j]);
        }
        cout<<""<<endl;
    }
    h_pre.digest(pre_commit[REP][0]);//最终的pre_commit

}

void local_preprocess(vector<vector<vector<char>>> &seed,char * filename,bool ** (&pre_all_a),bool **(&pre_all_b),bool **(&pre_all_ab))
{   
    cout<<"enter the local preprocess"<<endl;
    //seed [REP][party_in_a_head]
    //pre_all_a 和 pre_all_b 和pre_all_ab [REP][andgate_num];
    for(int i=0;i<REP;i++)
    {
        //cout<<"REP is "<<i<<" :"<<endl;
        vector<Circuit*> circuit;
        for(int j=0;j<party_in_a_head;j++)
        {
        //    cout<<"party "<<j<<endl;
            circuit.push_back(new Circuit(filename));
            circuit[j]->prg_n.reseed((unsigned char*)seed[i][j].data());
            circuit[j]->pre_wire();
        /*    cout<<"pre_a:"<<endl;
            for(int k=0;k<10;k++)
            {
                cout<<int(pre_circuit[j]->Inputwires_andgate_a[k]);
            }
            cout<<""<<endl;
            cout<<"pre_b:"<<endl;
            for(int k=0;k<10;k++)
            {
                cout<<int(pre_circuit[j]->Inputwires_andgate_b[k]);
            }
            cout<<""<<endl;
            cout<<"pre_ab:"<<endl;
            for(int k=0;k<10;k++)
            {
                cout<<int(pre_circuit[j]->Outputwires_andgate_ab[k]);
            }*/
            if(j==0)//第一方
            {
                //copy(pre_circuit[j]->Inputwires_andgate_a,pre_circuit[j]->Inputwires_andgate_a+andgate_num,pre_all_a[i]);
                //copy(pre_circuit[j]->Inputwires_andgate_b,pre_circuit[j]->Inputwires_andgate_b+andgate_num,pre_all_b[i]);
                //copy(pre_circuit[j]->Outputwires_andgate_ab,pre_circuit[j]->Outputwires_andgate_ab+andgate_num,pre_all_ab[i]);
                for(int k=0;k<andgate_num;k++)
                {
                    pre_all_a[i][k]=circuit[j]->Inputwires_andgate_a[k];
                    pre_all_b[i][k]=circuit[j]->Inputwires_andgate_b[k];
                    pre_all_ab[i][k]=circuit[j]->Outputwires_andgate_ab[k];
                }
            }
            else
            {
                for(int k=0;k<andgate_num;k++)
                {
                    pre_all_a[i][k]^=circuit[j]->Inputwires_andgate_a[k];
                    pre_all_b[i][k]^=circuit[j]->Inputwires_andgate_b[k];
                    pre_all_ab[i][k]^=circuit[j]->Outputwires_andgate_ab[k];
                }
               //std::transform(pre_all_a[i],pre_all_a[i]+andgate_num , pre_circuit[j]->Inputwires_andgate_a, pre_all_a[i], std::bit_xor<bool>());
               // std::transform(pre_all_b[i],pre_all_b[i]+andgate_num , pre_circuit[j]->Inputwires_andgate_b, pre_all_b[i], std::bit_xor<bool>());
               // std::transform(pre_all_ab[i],pre_all_ab[i]+andgate_num , pre_circuit[j]->Outputwires_andgate_ab, pre_all_ab[i], std::bit_xor<bool>());
            }    
        }
    /*    cout<<""<<endl;
        cout<<"pre_all_a:"<<endl;
        for(int k=0;k<10;k++)
        {
            cout<<int(pre_all_a[i][k]);
        }
        cout<<""<<endl;
        cout<<"pre_all_b:"<<endl;
        for(int k=0;k<10;k++)
        {
            cout<<int(pre_all_b[i][k]);
        }
        cout<<""<<endl;
        cout<<"pre_all_ab:"<<endl;
        for(int k=0;k<10;k++)
        {
            cout<<int(pre_all_ab[i][k]);
        }
        cout<<""<<endl;
    */
        for(Circuit* ptr:circuit)
        {
            delete ptr;
        }
        circuit.clear();
    }
}


void broadcast_get(bool ***&broadcast_msg,MPIO<RecIO,prover_num>* &io,int input_size)
{   //cout<<"enter the broadcast"<<endl;
    //broadcast_msg[run_num][party][input_size]
    for(int i=1;i<=prover_num;i++)
    {
        for(int j=1;j<=prover_num;j++)
        {
            if(i==j) continue;
            if(i==prover)
            {
                for(int l=0;l<run_num;l++)
                {
                    for(int k=0;k<party_in_a_head;k++)
                    {
                        io->send_data(j,broadcast_msg[l][(prover-1)*party_in_a_head+k+1],input_size);
                    }
                    
                }   
            }
            if(j==prover)
            {
                for(int l=0;l<run_num;l++)
                {
                    for(int k=0;k<party_in_a_head;k++)
                    {
                        io->recv_data(i,broadcast_msg[l][(i-1)*party_in_a_head+k+1],input_size);
                    }
                    
                }

            }
        }
    }

    for(int k=0;k<run_num;k++)
    {
       for(int i=0;i<input_size;i++)
       {
            broadcast_msg[k][0][i]=0;
            for(int j=1;j<=prover_num*party_in_a_head;j++)
            {
                broadcast_msg[k][0][i] ^= broadcast_msg[k][j][i];
            }
        } 
    }
    //cout<<"broadcast is over"<<endl;
}



void msg_put_hash(my::Hash &view_h,bool **&broadcast_msg,int msg_size)
{
    for(int i=1;i<prover_num*party_in_a_head+1;i++)
    {
        view_h.put(broadcast_msg[i],msg_size);
    }
}

/*
distribute the witness to the virtual party in his head
*/
void SecretInput_share(int secret,int share_num,int input_size,vector<vector<bool>> & secret_share)
{
    secret_share.resize(share_num);

    vector<bool> inputs;
    inputs.resize(input_size,0);
    cout<<"prover_input is "<<endl;
    for(int i=0;i<32;i++)
    {
        inputs[i] =(secret>>i&1);
        cout<<int(inputs[i]);
    }
    cout<<""<<endl;
    for(int i=0;i<share_num;i++)
    {
        secret_share[i].resize(input_size,0);
        if(i==(share_num-1))
        {
            for(int j=0;j<input_size;j++)
            {
                secret_share[i][j]=inputs[j];
            }
        }


    }
    my::PRG p;
    for(int i=0;i<share_num;i++)
    {
        cout<<"party "<<(i+1)<<" input is ";
        if(i!=(share_num-1))
        {
            for(int j=0;j<input_size;j++)
            {
                secret_share[i][j]=p.rand()%2;
                secret_share[share_num-1][j]=secret_share[share_num-1][j]^secret_share[i][j];
                cout<<int(secret_share[i][j]);
            } 
        }
        else
        {
            for(int j=0;j<input_size;j++)
            {
                cout<<int(secret_share[i][j]);
            } 
        }
        cout<<""<<endl;

    }

}
void bool_copy_to_char(vector<char> &dst,bool* &src,int position,int copy_size)
{
    memcpy(&dst[position], src, copy_size);
}
void compute_online(int secret,vector<vector<vector<char>>> &seed,char* filename,char **&online_commit,vector<vector<bool>> & secret_share, MPIO<RecIO,prover_num>* &io, vector<vector<vector<char>>> &online_msg,bool* run_round,bool** &aux)
{   cout<<"enter the compute online function"<<endl;
    
    bool *** masked_input =new bool**[run_num];//[prover_num*party_in_a_head+1][secret_share[0].size()];
    bool *** output_mask = new bool**[run_num];
    bool *** broadcast_msg = new bool**[run_num];   
    for(int i=0;i<run_num;i++)
    {
        masked_input[i] = new bool*[prover_num*party_in_a_head+1];
        output_mask[i] = new bool*[prover_num*party_in_a_head+1];
        broadcast_msg[i] = new bool*[prover_num*party_in_a_head+1];
        for(int j=0;j<prover_num*party_in_a_head+1;j++)
        {
            masked_input[i][j] = new bool[input_len];
            output_mask[i][j] = new bool[output_len];
            broadcast_msg[i][j] = new bool[1];
        }
    }
    cout<<"compute online function have allocate the memory"<<endl;
    
    //记得把online_msg删掉！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    //Hash view_h[party_in_a_head];
    my::Hash view_h1;
    //char commit[party_in_a_head][32];
    //aux [REP][padding(andgate_num)];
    // online_msg [run_num][party*prover+1][input+and+output]
    vector<vector<bool>> real_output;
    real_output.resize(party_in_a_head);

    int run_count =0;
   // int parallel_count=0;
    vector<vector<Circuit*>> online_circuit;
    for(int i=0;i<REP;i++)
    {    //seed [REP][party_in_a_head][32]
        if(run_round[i]==1)//需要做online
        {
            cout<<"=-=-=-=-=-=-=-=-=-=-=REP is "<<i<<" : =-=-=-=-=-=-=-=-==-=-=-=-=="<<endl;
            SecretInput_share(secret,party_in_a_head,input_len,secret_share);
            vector<Circuit*> online_circuit_round;
            view_h1.reset();
            for(int j=0;j<party_in_a_head;j++)
            {
                //view_h[j].reset();
                online_circuit_round.push_back(new Circuit(filename));
                online_circuit_round[j]->prg_n.reseed((unsigned char*) seed[i][j].data());
                online_circuit_round[j]->pre_wire();
                online_circuit_round[j]->abshare_andgate.resize(andgate_num,0);
                online_circuit_round[j]->online_wires.resize(online_circuit_round[j]->num_wire);
                for(int k=0;k<online_circuit_round[j]->n1;k++)
                {
                    //online_circuit[j]-> online_wires[k]=secret_share[j][k]^online_circuit[j]->pre_wires[k].val;
                    masked_input[run_count][(prover-1)*party_in_a_head+j+1][k] =online_circuit_round[j]->pre_wires[k].val^secret_share[j][k];
  
                }//masked_input 
                //cout<<"======ab share:  ";
                if(prover==prover_num&&j==(party_in_a_head-1))
                {
                    for(int k=0;k<andgate_num;k++)
                    {
                        online_circuit_round[j]->abshare_andgate[k]=online_circuit_round[j]->Outputwires_andgate_ab[k];
                        online_circuit_round[j]->abshare_andgate[k]=online_circuit_round[j]->abshare_andgate[k]^aux[i][k];
                    } 
                }
                else
                {
                    for(int k=0;k<andgate_num;k++)
                    {
                        online_circuit_round[j]->abshare_andgate[k]=online_circuit_round[j]->Outputwires_andgate_ab[k];
                    } 

                }
            }//得到预处理的ab share
            online_circuit.push_back(online_circuit_round);
            run_count++;
        }
    }  

    broadcast_get(masked_input,io,online_circuit[0][0]->n1);//masked_input[][0]储存了最后的xor结果
    
    for(int i=0;i<run_num;i++)
    {
        for(int j=0;j<prover_num*party_in_a_head+1;j++)
        {
            bool_copy_to_char(online_msg[i][j],masked_input[i][j],0,input_len);//把收到的mask_input放进online_msg
        }
        for(int j=0;j<party_in_a_head;j++)
        {
            for(int k=0;k<online_circuit[0][0]->n1;k++)
            {
                online_circuit[i][j]->online_wires[k] = masked_input[i][0][k];//masked_input放入电路
            }
        }
    }

    int num_gate = online_circuit[0][0]->num_gate;
    int and_count = 0;
    //开始online工作
    for(int i=0;i<num_gate;i++)
    {
        if(online_circuit[0][0]->gates[4*i+3] == AND_GATE)
        {
            for(int j=0;j<run_num;j++)
            {   
                for(int k=0;k<party_in_a_head;k++)
                {
                    bool online_a = online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i]].val;
                    bool online_b = online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i+1]].val;
                    bool pre_a = online_circuit[j][k]->pre_wires[online_circuit[j][k]->gates[4*i]].val;
                    bool pre_b = online_circuit[j][k]->pre_wires[online_circuit[j][k]->gates[4*i+1]].val;
                    bool pre_c = online_circuit[j][k]->pre_wires[online_circuit[j][k]->gates[4*i+2]].val;
                    bool pre_ab = online_circuit[j][k]->abshare_andgate[and_count];
                    broadcast_msg[j][(prover-1)*party_in_a_head+k+1][0] = online_a&pre_b ^ online_b&pre_a ^ pre_ab ^ pre_c;
                    online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i+2]]=online_a &online_b;
                }
            }
            broadcast_get(broadcast_msg,io,1);

            for(int j=0;j<run_num;j++)
            {
                for(int k=1;k<prover_num*party_in_a_head+1;k++)
                {
                    online_msg[j][k][input_len+and_count] = broadcast_msg[j][k][0];  
                }
            }
            for(int j=0;j<run_num;j++)
            {
                for(int k=0;k<party_in_a_head;k++)
                {
                    online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i+2]].val^=broadcast_msg[j][0][0];//0代表着重构结果 
                //msg_put_hash(view_h[k],broadcast_msg,1);
                //以上是 andgate_view commit;
                }
            }

            and_count++;
        }
        else if(online_circuit[0][0]->gates[4*i+3] == XOR_GATE)
        {
            for(int j=0;j<run_num;j++)
            {
                for(int k=0;k<party_in_a_head;k++)
                {
                    bool online_a = online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i]].val;
                    bool online_b = online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i+1]].val;
                    online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i+2]]=online_a ^ online_b;
                }
            }
        }
        else  
		{
            for(int j=0;j<run_num;j++)
            {
                for(int k=0;k<party_in_a_head;k++)
                {
                    bool online_a = online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i]].val;
                    online_circuit[j][k]->online_wires[online_circuit[j][k]->gates[4*i+2]]=online_a ^ 1;
                } 
            }
	    }	

    }
    //online过程结束,开始处理output
    for(int i=0;i<run_num;i++)
    {   
        for(int j=0;j<party_in_a_head;j++)//output
        {
            int wire_num =online_circuit[i][j]->num_wire;
            for(int k=0;k<online_circuit[i][j]->n3;k++)
            {        
                output_mask[i][(prover-1)*party_in_a_head+j+1][k] = online_circuit[i][j]->pre_wires[wire_num-output_len+k].val;
            }
        }
    }

    broadcast_get(output_mask,io,output_len);
    for(int i=0;i<run_num;i++)//将output阶段的消息复制下来
    {
        for(int j=1;j<prover_num*party_in_a_head+1;j++)
        {
            bool_copy_to_char(online_msg[i][j],output_mask[i][j],input_len+andgate_num,output_len);
        }
    }

    for(int i=0;i<run_num;i++)
    {
        cout<<"====run count is :"<<i<<endl;
        for(int j=0;j<party_in_a_head;j++)
        {
            //msg_put_hash(view_h[j],output_mask,output_len);
            //view_h[j].digest(commit[i][j+1+(prover-1)*party_in_a_head]);//获得当前轮commit
            real_output[j].resize(output_len,0);
            int wire_num =online_circuit[i][j]->num_wire;
   
            cout<<"party "<<(j+1)<<" real out: ";
            for(int k=0;k<online_circuit[i][j]->n3;k++)
            {
                real_output[j][k] = online_circuit[i][j]->online_wires[wire_num-output_len+k].val^output_mask[i][0][k];
                cout<<int(real_output[j][k]);
            }
            cout<<""<<endl;
        }
    }

    for(auto &ptr_row:online_circuit)
    {
        for(auto &ptr_col:ptr_row)
        {
            delete ptr_col;
        }
        
    }

    for(int i=0;i<run_num;i++)
    {
        view_h1.reset();
        for(int j=1;j<prover_num*party_in_a_head+1;j++)
        {
            view_h1.put(online_msg[i][j].data(),andgate_num+input_len+output_len);
        }
        view_h1.digest(online_commit[i]);//这个无需广播，每个prover都是一样的
    }
            //if(run_count==parallel_num)//
 /*           for(int j=0;j<prover_num*party_in_a_head+1;j++)//这里是从0开始，把input_hat放进online_msg[i][0]，后面是从1开始
            {
                bool_copy_to_char(online_msg[run_count][j],masked_input[j],0,input_len);
            }

            for(int j=0;j<party_in_a_head;j++)
            {
                for(int k=0;k<online_circuit[j]->n1;k++)
                {
                    online_circuit[j]->online_wires[k] =masked_input[0][k];
                }
            }
            int num_gate =online_circuit[0]->num_gate;
            int and_count=0;
            for(int j=0;j<num_gate;j++)
            {
                if(online_circuit[0]->gates[4*j+3] == AND_GATE) {
				///////加入andgate的两个输入////////////
                //bool broadcast_msg [party_in_a_head*prover_num+1];
                //bool broadcast_msg
                    for(int k=0;k<party_in_a_head;k++)
                    {
                        bool online_a = online_circuit[k]->online_wires[online_circuit[k]->gates[4*j]].val;
                        bool online_b = online_circuit[k]->online_wires[online_circuit[k]->gates[4*j+1]].val;
                        bool pre_a = online_circuit[k]->pre_wires[online_circuit[k]->gates[4*j]].val;
                        bool pre_b = online_circuit[k]->pre_wires[online_circuit[k]->gates[4*j+1]].val;
                        bool pre_c = online_circuit[k]->pre_wires[online_circuit[k]->gates[4*j+2]].val;
                        bool pre_ab = online_circuit[k]->abshare_andgate[and_count];
                        broadcast_msg[(prover-1)*party_in_a_head+k+1][0] = online_a&pre_b ^ online_b&pre_a ^ pre_ab ^ pre_c;
                        online_circuit[k]->online_wires[online_circuit[k]->gates[4*j+2]]=online_a &online_b;
                    }

                    broadcast_get(broadcast_msg,io,1);

                    for(int k=1;k<prover_num*party_in_a_head+1;k++)
                    {
                        online_msg[run_count][k][input_len+and_count] = broadcast_msg[k][0];
                        //bool_copy_to_char(online_msg[i][k],broadcast_msg[k],input_len+and_count,1);
                    }

                    for(int k=0;k<party_in_a_head;k++)
                    {
                        online_circuit[k]->online_wires[online_circuit[k]->gates[4*j+2]].val^=broadcast_msg[0][0];
                    //msg_put_hash(view_h[k],broadcast_msg,1);
                    //以上是 andgate_view commit;
                    }
				    and_count++;
			    }
			    else if(online_circuit[0]->gates[4*j+3] == XOR_GATE) {
                    for(int k=0;k<party_in_a_head;k++)
                    {
                        bool online_a = online_circuit[k]->online_wires[online_circuit[k]->gates[4*j]].val;
                        bool online_b = online_circuit[k]->online_wires[online_circuit[k]->gates[4*j+1]].val;
                        online_circuit[k]->online_wires[online_circuit[k]->gates[4*j+2]]=online_a ^ online_b;
                    }
				
			    }
			    else  
			    {
                    for(int k=0;k<party_in_a_head;k++)
                    {
                        bool online_a = online_circuit[k]->online_wires[online_circuit[k]->gates[4*j]].val;
                        online_circuit[k]->online_wires[online_circuit[k]->gates[4*j+2]]=online_a ^ 1;
                    }
			    }	
            }
            
            for(int j=0;j<party_in_a_head;j++)//output
            {
                int wire_num =online_circuit[j]->num_wire;
                for(int k=0;k<online_circuit[j]->n3;k++)
                {        
                    output_mask[(prover-1)*party_in_a_head+j+1][k]=online_circuit[j]->pre_wires[wire_num-output_len+k].val;
                //online_circuit[j]->online_wires[k] =masked_input[0][k];
                }
            }
            broadcast_get(output_mask,io,output_len);
            for(int j=1;j<prover_num*party_in_a_head+1;j++)
            {
                bool_copy_to_char(online_msg[run_count][j],output_mask[j],input_len+andgate_num,output_len);
            }

            for(int j=0;j<party_in_a_head;j++)
            {
            //msg_put_hash(view_h[j],output_mask,output_len);
            //view_h[j].digest(commit[i][j+1+(prover-1)*party_in_a_head]);//获得当前轮commit
                real_output[j].resize(output_len,0);
                int wire_num =online_circuit[j]->num_wire;
   
                cout<<"party "<<(j+1)<<" real out: ";
                for(int k=0;k<online_circuit[j]->n3;k++)
                {
                    real_output[j][k] = online_circuit[j]->online_wires[wire_num-output_len+k].val^output_mask[0][k];
                    cout<<int(real_output[j][k]);
                }
                cout<<""<<endl;
            }
        
            //broadcast_commit(pre_commit[i],io);//广播每一方的commit，得到当前轮的总commit
        
            for(auto &ptr:online_circuit)
            {
                delete ptr;
            }

            for(int j=1;j<prover_num*party_in_a_head+1;j++)
            {
                view_h1.put(online_msg[run_count][j].data(),andgate_num+input_len+output_len);
            }
            view_h1.digest(online_commit[run_count]);//这个无需广播，每个prover都是一样的
            
            run_count++;
            parallel_count++;
        }
    */
        
    //view_h[0].reset();
    view_h1.reset();
    for(int i=0;i<run_num;i++)
    {
        //cout<<"pre_commit "<<i<<" : "<<endl;
        /////view_h[0].put(pre_commit[i][0],32);
       /* for(int j=0;j<32;j++)
        {
            cout<<int(pre_commit[i][0][j]);
        }*/
       // cout<<""<<endl;
       // cout<<"online_commit "<<i<<" : "<<endl;
        view_h1.put(online_commit[i],32);
       /* for(int j=0;j<32;j++)
        {
            cout<<int(online_commit[i][j]);
        }
        cout<<""<<endl;*/
    }//得到所有轮的总pre_commit
    //view_h[0].digest(pre_commit[REP][0]);
    view_h1.digest(online_commit[run_num]);


    //删除内存
    for(int i=0;i<run_num;i++)
    {
        for (int j = 0; j < prover_num*party_in_a_head+1; ++j) 
        {
            delete[] masked_input[i][j];
            delete[] output_mask[i][j];
            delete[] broadcast_msg[i][j];
    //    delete[] commit[i];
        }
        delete[] masked_input[i];
        delete[] output_mask[i];
        delete[] broadcast_msg[i];
    }

    delete[] masked_input;
    delete[] output_mask;
    delete[] broadcast_msg;
   // delete[] commit;
}


void delete_4Darray(bool **** & array,int dim1,int dim2,int dim3)
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
void delete_3Darray(bool *** & array,int dim1,int dim2)
{

    for (int i = 1; i < dim1; ++i) {
        for (int j = 0; j < dim2; ++j) {
            delete[] array[i][j];
        }
        delete[] array[i];
    }
    delete[] array;
    cout<<"delete 3D is over"<<endl;
}


void write_proof(Seed_Tree & seed_tree,bool*&run_round, vector<char> &final_commit,vector<vector<vector<char>>> &seed, vector<vector<char>> master_seed,vector<vector<vector<char>>> &online_msg,char*** pre_commit,bool** &aux)
{
    //online_msg [REP][n+1][]
    //master_seed [REP][32]
    //seed[rep][party_in_a_head][32];
    //pre_commit [REP+1][prover_num*party_in_a_head+1][32];
    //aux[REP][padding(andgate_num)]
    FILE *fp[party_in_a_head];
    for(int i=1;i<=party_in_a_head;i++)
    {
        string name="view_"+to_string((prover-1)*party_in_a_head+i)+".bin";
        fp[i-1]=fopen(name.c_str(),"wb");//建立指针
    }
    my::PRG p;
    p.reseed((unsigned char*) final_commit.data());
    fwrite(pre_commit[REP][0],1,32,fp[0]);//写入pre_commit
    fwrite(final_commit.data(),1,32,fp[0]);
    cout<<"write final_commit:"<<endl;
    for(int i=0;i<32;i++)
    {
        cout<<int(final_commit[i]);
    }
    int* unopened_round= new int[run_num];
    int unopened_count=0;
    for(int i=0;i<REP;i++)
    {
        if(run_round[i]==1)//1代表要继续做online，那就不用把masterseed送过去
        {
            unopened_round[unopened_count]=i+1;
            unopened_count++;
        }
    }
    int send_master_node_num=seed_tree.find_node(unopened_round,run_num);
    fwrite(&send_master_node_num,sizeof(int),1,fp[0]);
    cout<<"send_node_num is :"<<send_master_node_num<<endl;
    cout<<"seed tree open:";
    for(int i=1;i<=seed_tree.num_leaves;i++)
    {
        if(seed_tree.nodes[i]->send_flag==2)
        {

            fwrite(&i,sizeof(int),1,fp[0]);
            cout<<" "<<i;
            fwrite(seed_tree.nodes[i]->seed,1,32,fp[0]);
        }
    }
    cout<<""<<endl;

    //p1.reseed((unsigned char*) commit_all);
    bool ** open_party=new bool*[run_num];//[party_in_a_head*prover_num+1];
    char * file_out_pre=new char[andgate_num];
    char * file_out_online =new char[andgate_num+input_len+output_len];
    //int *unopened_leaves=new int[REP];
    for(int i=0;i<run_num;i++)
    {
        open_party[i] = new bool[party_in_a_head*prover_num+1];
    }
   // char* send_seed = new char[];
    int run_count=0; 
    for(int i=0;i<REP;i++)
    {
        cout<<"REP is"<<i<<" "<<endl;
        if(run_round[i]==0)//全打开
        {
            //fwrite(master_seed[i].data(),1,32,fp[0]);
        }
        else//运行online
        {
            get_openparty(p,open_party[run_count]);
            for(int j=0;j<party_in_a_head;j++)
            {
                if(open_party[run_count][(prover-1)*party_in_a_head+j+1]==1)//打开
                {
                    cout<<"enter opened writing"<<endl;
                    fwrite(seed[i][j].data(),1,32,fp[j]);
                    if(j==(party_in_a_head-1)&&prover==prover_num)
                    {
                        cout<<"enter the aux writing"<<endl;
                        int byte = compress_bool_to_byte(file_out_pre,aux[i],andgate_num);//写入state=seed||aux；
                        fwrite(file_out_pre,1,byte,fp[j]);
                        cout<<"aux writing is overs"<<endl;
                    }
                    int byte = compress_bool_to_byte(file_out_online,online_msg[run_count][j+(prover-1)*party_in_a_head+1],input_len);
                    fwrite(file_out_online,1,byte,fp[j]);
                    cout<<"opened writing is over"<<endl;
                }
                else//不打开
                {
                    cout<<"enter unopened writing"<<endl;
                    fwrite(pre_commit[i][j+1+(prover-1)*party_in_a_head],1,32,fp[j]);
                    int byte =compress_bool_to_byte(file_out_online,online_msg[run_count][j+(prover-1)*party_in_a_head+1],input_len+andgate_num+output_len);
                    fwrite(file_out_online,1,byte,fp[j]);
                    cout<<"unopened writing is over"<<endl;
                }

            }
            //cout<<"write over"<<endl;
            run_count++;
        }
        
    }
    cout<<"write proof is over========><==============="<<endl;
    //int send_node_num=seed_tree.find_node(unopened_leaves,REP);
    // fwrite(unopened_leaves,1,REP,fp[0]);//写入
    // fwrite(&send_node_num,1,1,fp[0]);
    //cout<<"send_node_num is :"<<send_node_num<<endl;
    //cout<<"seed tree open:";
    //for(int i=1;i<=seed_tree.num_leaves;i++)
    //{
    //    if(seed_tree.nodes[i]->send_flag==2)
    //    {

    //        fwrite(&i,1,1,fp[0]);
    //        cout<<" "<<i;
    //        fwrite(seed_tree.nodes[i]->seed,1,32,fp[0]);
    //    }
    //}
    //cout<<""<<endl;
   /*for(int i=0;i<REP;i++)
    {

        for(int j=1;j<=party_in_a_head;j++)//自
        {    
            if(open_party[i][j+(prover-1)*party_in_a_head]==1)//选择打开,这一方预处理的seed和剩下不打开的发给这个party的消息
            {
                //cout<<"party "<<j;
                //********fwrite(seed[i][j-1].data(),1,32,fp[j-1]);
 
                for(int k=1;k<=party_in_a_head*prover_num;k++)
                {
                    if(open_party[i][k]==0)//不打开的party发给他的view要保存起来
                    {
                        int byte_len=compress_bool_to_byte(file_out_pre,pre_view[j][i][k],andgate_num);
                        fwrite(file_out_pre,1,byte_len,fp[j-1]);
                    }
                }
                int byte_len=compress_bool_to_byte(file_out_online,online_msg[i][j+(prover-1)*party_in_a_head],input_len);
                fwrite(file_out_online,1,byte_len,fp[j-1]);//
            }
            else//不打开,online_msg放进去
            {
                fwrite(pre_commit[i][j+(prover-1)*party_in_a_head],1,32,fp[j-1]);
                int byte_len=compress_bool_to_byte(file_out_online,online_msg[i][j+(prover-1)*party_in_a_head],andgate_num+input_len+output_len);
                fwrite(file_out_online,1,byte_len,fp[j-1]);
            }
        }
       // for(int j=0;j<)

    }*/ 

    for(int i=0;i<party_in_a_head;i++)
    {
        fclose(fp[i]);
        //delete[] fp[i];

    }
    for(int i=0;i<run_num;i++)
    {
        delete [] open_party[i];
    }

    delete [] file_out_online;
    delete [] file_out_pre;
    delete [] unopened_round;
}
int main(int argc,char **argv){
    if(argc!=5){
        puts("./main <prover> <port> <circuit_file> <secret input>");
        return 0;
    }
    int secret_input;
    sscanf(argv[1],"%d",&prover);
    sscanf(argv[2],"%d",&port);
    sscanf(argv[3],"%s",&filename);
    sscanf(argv[4],"%d",&secret_input);

    Circuit circuit(filename);
    andgate_num=circuit.num_andgate;
    input_len = circuit.n1;
    output_len = circuit.n3;

    my::PRG prg;
    my::PRG prg_seed;
    vector<vector<char>> master_seed;
    vector<vector<vector<char>>> seed;//[REP][party_in_a_head][32]
    vector<vector<bool>> secret_share;

    int num_leaves=get_numleaves(REP);
    Seed_Tree seed_tree(num_leaves);

    master_seed.resize(REP);
    seed.resize(REP);

    for(int i=0;i<REP;i++)
    {
        master_seed[i].resize(32);
        //prg.random_data(master_seed[i].data(),32);
       // prg_seed.reseed((unsigned char*) master_seed[i].data());//master_seed 生成
       // seed[i].resize(party_in_a_head);
       // for(int j=0;j<party_in_a_head;j++)
        //{
        //    seed[i][j].resize(32);
        //    prg_seed.random_data(seed[i][j].data(),32);
        //}
    }
    seed_tree.set_master_seed(master_seed,REP);
    //prg_seed.reseed((unsigned char*) master_seed[i].data());
    for(int i=0;i<REP;i++)
    {
        prg_seed.reseed((unsigned char*) master_seed[i].data());//master_seed 生成
        seed[i].resize(party_in_a_head);
        for(int j=0;j<party_in_a_head;j++)
        {
            seed[i][j].resize(32);
            prg_seed.random_data(seed[i][j].data(),32);
        }

    }
    
    
    
/*    vector<vector<vector<char>>> seed;
    //二叉树结构
    int num_leaves=get_numleaves(REP*party_in_a_head);
    Seed_Tree seed_tree(num_leaves);
    

    seed.resize(REP);
    vector<vector<bool>> secret_share;
    for(int i=0;i<REP;i++)
    {
        seed[i].resize(party_in_a_head);
        for(int j=0;j<party_in_a_head;j++)
        {
            seed[i][j].resize(32);
        }
    }*/
    //seed_tree.set_seed(seed,REP,party_in_a_head);

   /* for (int i = 0; i < REP; ++i) {
        for (int j = 0; j < party_in_a_head; ++j) {
            for (int k = 0; k < 32; ++k) {
                seed[i][j][k] = prg.rand();
            }
        }
    }*/
   // vector<vector<Circuit*>> circuit;
    vector<string>ip;
    for(int i=0;i<=prover_num+1;i++)
        ip.push_back(string("127.0.0.1"));

   // MPIO<RecIO,prover_num+1> *io=new MPIO<RecIO,prover_num+1>(prover,ip,port,false);//party是用户输入，假设n=3*8
   // cout<<"MPIO connected"<<endl;
   // Key_Ex<MPIO<RecIO,prover_num+1>> * DH = new Key_Ex<MPIO<RecIO,prover_num+1>> (io);
   // cout<<"DH setup"<<endl;

    //////////////////////////////////////////////////
    //建立ferret_COT
    /////////////////////
    cout<<"begin to connect io"<<endl;
    MPIO<NetIO,prover_num> *io_pre=new MPIO<NetIO,prover_num>(prover,ip,port,true);
    cout<<"io_pre is set up"<<endl;
    FerretCOT<NetIO> * ferret_send[prover_num+1];
    FerretCOT<NetIO> * ferret_recv[prover_num+1];
    for(int i=1;i<=prover_num;i++)//i，j循环，如果i是party则send，j是party则recv
        for(int j=1;j<=prover_num;j++)
            {if(i!=j){
            if(i==prover){
                ferret_send[j] = new FerretCOT<NetIO>(1, 1, &io_pre->send_io[j], true);//alice,拥有delta
        }
        if(j==prover){
            ferret_recv[i] = new FerretCOT<NetIO>(2, 1, &io_pre->recv_io[i], true);//bob
        }}
    }//ferret建立
    cout<<"ferret is over"<<endl;
    ///////////////////////////////////////////////

    //
    time_t begin = time(NULL);
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    //
    
    bool ** pre_all_a = new bool*[REP];
    bool ** pre_all_b = new bool*[REP];
    bool ** pre_all_ab = new bool*[REP];
    bool ** aux = new bool*[REP];
    //if(prover==prover_num)
    //{
        //aux= new bool*[REP];
    //}
    char *** pre_commit = new char**[REP+1];//最后一个用来放总的commit [REP+1][prover_num*party_in_a_head+1][32];
    char ** online_commit = new char*[run_num+1];
    for(int i=0;i<REP;i++)
    {
        pre_all_a[i]=new bool[andgate_num];
        pre_all_b[i]=new bool[andgate_num];
        pre_all_ab[i]=new bool[andgate_num];
        //if(prover==prover_num)
        //{
            aux[i]=new bool[andgate_num]();//现在每个prover都要有自己的aux
        //}
        
        pre_commit[i] =new char*[prover_num*party_in_a_head+1];
        //online_commit[i] =new char[32];
        for(int j=0;j<=prover_num*party_in_a_head;j++)
        {
            pre_commit[i][j] =new char[32];
        }
    }
    pre_commit[REP]=new char*[prover_num*party_in_a_head+1];
    for(int j=0;j<=prover_num*party_in_a_head;j++)
    {
        pre_commit[REP][j] =new char[32];
    }
    for(int i=0;i<run_num+1;i++)
    {
        online_commit[i]=new char[32];
    }

    local_preprocess(seed,filename,pre_all_a,pre_all_b,pre_all_ab);
    cout<<"local preprocessing is over."<<endl;
    /*开始和第三方交互*/

    
    send_to_TA(io_pre,ferret_send,ferret_recv,pre_all_a,pre_all_b,pre_all_ab,aux);//做预处理阶段的承诺
    delete io_pre; 
    struct timespec end_pre;
    clock_gettime(CLOCK_MONOTONIC, &end_pre);
    //delete DH;
    MPIO<RecIO,prover_num> *online_io=new MPIO<RecIO,prover_num>(prover,ip,port,false);//party是用户输入，假设n=3*8
    cout<<"online MPIO connected===>"<<endl;

    gen_pre_commit(online_io,aux,seed,pre_commit);//最后一个用来放总的commit
    cout<<"pre-commit is over"<<endl;
    my::PRG p;
    bool * run_round = new bool[REP];
    p.reseed((unsigned char*) pre_commit[REP][0]);
    get_online_round(p,run_num,run_round);
    cout<<"have get online round"<<endl;
    for(int i=0;i<REP;i++)
    {
        delete[] pre_all_a[i];
        delete[] pre_all_b[i];
        delete[] pre_all_ab[i];
    }


    vector<vector<vector<char>>> online_msg;//
    online_msg.resize(run_num);
    for(int i=0;i<run_num;i++)
    {
        online_msg[i].resize(prover_num*party_in_a_head+1);
        for(int j=0;j<prover_num*party_in_a_head+1;j++)
        {
            online_msg[i][j].resize(input_len+andgate_num+output_len);
        }
    }

    compute_online(secret_input,seed,filename,online_commit,secret_share,online_io,online_msg,run_round,aux);//online_commit 最后一个放了总的commit
    cout<<"online_compute is over"<<endl;

    my::Hash h;
    vector<char> final_commit;
    final_commit.resize(32);
    h.reset();
    h.put(pre_commit[REP][0],32);
    h.put(online_commit[run_num],32);
    h.digest(final_commit.data());//得到最终的commit，用于挑选哪些方写入proof
    write_proof(seed_tree,run_round,final_commit,seed,master_seed,online_msg,pre_commit,aux);
    cout<<"back to main function"<<endl;
    for(int i=0;i<REP;i++)
    {
        //if(prover==prover_num)
        //{
            delete[] aux[i];
        //}
    }
    delete_3Darray(pre_commit,REP+1,prover_num*party_in_a_head+1);
    delete online_io;
    delete[] run_round;
    //delete online_io;
    /* vector<vector<vector<vector<char>>>> pre_view;
    pre_view.resize(party_in_a_head+1);
    for(int i=1;i<=party_in_a_head;i++)
    {
        pre_view[i].resize(REP);
        for(int j=0;j<REP;j++)
        {
            pre_view[i][j].resize(party_in_a_head*prover_num+1);
            for(int k=1;k<=party_in_a_head*prover_num;k++)
            {
                pre_view[i][j][k].resize(andgate_num);
            }
        }
    }
    
    for (int j = 1; j <= party_in_a_head; j++) {
        threads.push_back(std::thread(thread_lambda_ab_generation, ref(pre_view), ref(send_r),ref(send_r_xor_a),ref(choice_b),ref(io),ref(iknp),j));
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    threads.clear();
    cout<<"lambda ab generating is over."<<endl;
    //预处理结束
    delete_4Darray(send_r_xor_a,party_in_a_head+1,REP,party_in_a_head*prover_num+1);
    delete_3Darray(choice_b,party_in_a_head+1,REP);

    vector<vector<vector<char>>> online_msg;//后面再push具体元素
    online_msg.resize(REP);
    for(int i=0;i<REP;i++)
    {
        online_msg[i].resize(prover_num*party_in_a_head+1);
        //online_msg[i][0].resize(input_len);
        for(int j=0;j<prover_num*party_in_a_head+1;j++)
        {
            online_msg[i][j].resize(input_len+andgate_num+output_len);
        }
    }

    for(auto &io_connect:io)
    {
        delete io_connect;
    }
    //cout<<"lambda ab generating is over."<<endl;
    //开始online
    struct timespec end_pre;
    clock_gettime(CLOCK_MONOTONIC, &end_pre);

    MPIO<RecIO,prover_num> *io_online=new MPIO<RecIO,prover_num>(prover,ip,port,true);
    cout<<"online connected"<<endl;
    
    compute_online(secret_input,seed,filename,send_r,pre_commit,online_commit,pre_view,secret_share,io_online,online_msg);
   // bool *open_party = new bool[party_in_a_head*prover_num+1];
    delete io_online;
    Hash h;
    char* commit_all=new char[32];
    h.put(pre_commit[REP][0],32);
    h.put(online_commit[REP],32);
    h.digest(commit_all);
    //得到最终的commit

    
    write_proof(seed_tree,commit_all,seed,online_msg,pre_view,pre_commit);
    delete [] commit_all;
    delete_4Darray(send_r,party_in_a_head+1,REP,party_in_a_head*prover_num+1);

    delete_3Darray(pre_commit,REP+1,prover_num*party_in_a_head+1);
    for(int i=0;i<REP+1;i++)
    {
        delete[] online_commit[i];
    }
    delete[] online_commit;


*/

    time_t over_all = time(NULL); 
  //结束时间
    cout<<""<<endl;
    cout<<"time = "<<double(over_all-begin)<<"s"<<endl;
 
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    double duration_pre = (double)(end_pre.tv_nsec-start.tv_nsec)/((double) 1e9) + (double)(end_pre.tv_sec-start.tv_sec);
    double duration = (double)(end.tv_nsec-start.tv_nsec)/((double) 1e9) + (double)(end.tv_sec-start.tv_sec);
    cout<<"pre time is "<<duration_pre<<endl;
    cout<<"total time is "<<duration<<endl;

    return 0;
}