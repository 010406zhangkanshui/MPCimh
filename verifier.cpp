//#include "RecIO.hpp" 
//#include "mpio.hpp"
#include <iostream>
#include <vector>
#include "constant.h"
#include "circuit.hpp"
#include <time.h>
#include "program.hpp"
#include "seed_binary_tree.hpp"

int andgate_num,input_len,output_len,num_gate,wire_num;
char filename[25];


void pre_process(char* file_name,char*** &pre_commit,vector<vector<vector<char>>> &seed,vector<vector<char>> &aux,bool**&open_party,vector<vector<vector<char>>> &online_msg,char** &online_commit)
{
    //seed [REP][n][32]
    //aux [REP][andgate]
    //pre_commit [REP+1][N+1]，用于online 且不打开的pre_commit已经读入
    //open_party[run_num][N+1]
    //online_msg [run_num][N+1][input+andgate_num+output]
    my::PRG p;
    my::Hash h_pre;
    my::Hash h_online;
    p.reseed((unsigned char*) pre_commit[REP][0]);
    bool* run_round=new bool[REP];
    get_online_round(p,run_num,run_round);
    bool * pre_all_a=new bool[andgate_num]();
    bool * pre_all_b = new bool[andgate_num]();
    int run_count=0; 
    char * new_pre_commit=new char[32];
    //char ** new_online_commit = new char*[run_num+1];

    //bool * masked_input = new bool[input_len];
    for(int i=0;i<REP;i++)
    {
        vector<Circuit*> pre_circuit;
        h_pre.reset();
        cout<<"REP is "<<i<<"-==-=-===="<<endl;
        if(run_round[i]==0)//全打开
        {
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                pre_circuit.push_back(new Circuit(filename));
                pre_circuit[j]->prg_n.reseed((unsigned char*)seed[i][j].data());
                pre_circuit[j]->pre_wire();
                if(j==0)
                {
                    for(int k=0;k<andgate_num;k++)
                    {
                        pre_all_a[k] = pre_circuit[j]->Inputwires_andgate_a[k];
                        pre_all_b[k] = pre_circuit[j]->Inputwires_andgate_b[k];
                        aux[i][k] = pre_circuit[j]->Outputwires_andgate_ab[k];
                    }   
                }
                else
                {
                    for(int k=0;k<andgate_num;k++)
                    {
                        pre_all_a[k] ^= pre_circuit[j]->Inputwires_andgate_a[k];
                        pre_all_b[k] ^= pre_circuit[j]->Inputwires_andgate_b[k];
                        aux[i][k] ^= pre_circuit[j]->Outputwires_andgate_ab[k];
                    }  
                }
            }
            for(int j=0;j<andgate_num;j++)
            {
                aux[i][j] ^=(pre_all_a[j]&pre_all_b[j]); 
            }
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                h_pre.reset();
                h_pre.put(seed[i][j].data(),32);
                if(j==(prover_num*party_in_a_head-1))
                {
                    h_pre.put(aux[i].data(),andgate_num);
                }
                h_pre.digest(pre_commit[i][j+1]);
            }
            h_pre.reset();
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                h_pre.put(pre_commit[i][j+1],32);
            }
            h_pre.digest(pre_commit[i][0]);

        }
        else//继续做online
        {
            //vector<bool> masked_input;
            //masked_input.resize(input_len,0);
            h_online.reset();
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                if(open_party[run_count][j+1]==1)//某方要打开
                {
                    pre_circuit.push_back(new Circuit(file_name));
                    pre_circuit[j]->prg_n.reseed((unsigned char*)seed[i][j].data());
                    pre_circuit[j]->pre_wire();
                    pre_circuit[j]->abshare_andgate.resize(andgate_num,0);
                    pre_circuit[j]->online_wires.resize(pre_circuit[j]->num_wire);
                    for(int k=0;k<andgate_num;k++)
                    {
                        pre_circuit[j]->abshare_andgate[k] = pre_circuit[j]->Outputwires_andgate_ab[k];
                    }
                    h_pre.reset();
                    h_pre.put(seed[i][j].data(),32);

                    if(j==(prover_num*party_in_a_head-1))
                    {
                        h_pre.put(aux[i].data(),andgate_num);
                        for(int k=0;k<andgate_num;k++)
                        {
                            pre_circuit[j]->abshare_andgate[k]=bool(aux[i][k])^pre_circuit[j]->abshare_andgate[k];
                        }

                    }
                    h_pre.digest(pre_commit[i][j+1]);
                }
                else
                {
                    pre_circuit.push_back(nullptr);
                    //已经从proof中读出了，无需操作
                }
                for(int k=0;k<input_len;k++)
                {
                    online_msg[run_count][0][k] ^= online_msg[run_count][j+1][k];
                }
            }
            for(int j=0;j<prover_num*party_in_a_head;j++)//放输入
            {
                if(open_party[run_count][j+1]==1)
                {
                    for(int k=0;k<input_len;k++)
                    {
                        pre_circuit[j]->online_wires[k] = online_msg[run_count][0][k];
                    }
                    
                }
            }
            int and_count=0;
            for(int j=0;j<num_gate;j++)
            {
                int and_flag=0;
                for(int k=0;k<prover_num*party_in_a_head;k++)
                {
                    if(open_party[run_count][k+1]==1)
                    {
                        if(pre_circuit[k]->gates[4*j+3]==0)
                        {
                            bool online_a = pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j]].val;
                            bool online_b = pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j+1]].val;
                            bool pre_a = pre_circuit[k]->pre_wires[pre_circuit[k]->gates[4*j]].val;
                            bool pre_b = pre_circuit[k]->pre_wires[pre_circuit[k]->gates[4*j+1]].val;
                            bool pre_c = pre_circuit[k]->pre_wires[pre_circuit[k]->gates[4*j+2]].val;
                            bool pre_ab = pre_circuit[k]->abshare_andgate[and_count];
                            and_flag = 1;
                            bool s_share = online_a&pre_b ^ online_b&pre_a ^ pre_ab ^ pre_c;

                            online_msg[run_count][k+1][input_len+and_count] = s_share;
                            pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j+2]] = online_a & online_b;
                       

                        }
                        else if(pre_circuit[k]->gates[4*j+3]==1)
                        {
                            bool online_a = pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j]].val;
                            bool online_b = pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j+1]].val;
                            pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j+2]]=online_a ^ online_b;
                        }
                        else
                        {
                            bool online_a = pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j]].val;
                            pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j+2]]=online_a ^ 1;
                    
                        }
                    }
                }
                if(and_flag==1)
                {
                    for(int k=0;k<party_in_a_head*prover_num;k++)
                    {
                        online_msg[run_count][0][input_len+and_count]^=online_msg[run_count][k+1][input_len+and_count];
                    }//获得xor结果
                    for(int k=0;k<party_in_a_head*prover_num;k++)
                    {
                        if(open_party[run_count][k+1]==1)
                        {
                            pre_circuit[k]->online_wires[pre_circuit[k]->gates[4*j+2]].val^=online_msg[run_count][0][input_len+and_count];
                        }
                    }//每个party获得and门的online的值；
                    and_count ++;
                }
            }
            //online线路已经计算完成,接下来开始做output_mask重构
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                if(open_party[run_count][j+1]==1)
                {
                    for(int k=0;k<output_len;k++)
                    {
                        online_msg[run_count][j+1][k+input_len+andgate_num] = pre_circuit[j]->pre_wires[wire_num-output_len+k].val;
                    }
                }
                for(int k=0;k<output_len;k++)
                {
                    online_msg[run_count][0][k+input_len+andgate_num] ^= online_msg[run_count][j+1][k+input_len+andgate_num];
                }
                h_online.put(online_msg[run_count][j+1].data(),andgate_num+input_len+output_len);

            }
            h_online.digest(online_commit[run_count]);
            //输出结果
            for(int j=0;j<party_in_a_head*prover_num;j++)
            {
                if(open_party[run_count][j+1]==1)
                {
                    cout<<"party_"<<j+1<<"real out:";
                    for(int k=0;k<output_len;k++)
                    {
                        bool real_out = (bool) online_msg[run_count][0][k+input_len+andgate_num]^bool(pre_circuit[j]->online_wires[wire_num-output_len+k].val);
                        cout<<int(real_out);
                    }
                    cout<<""<<endl;
                }
            }

            h_pre.reset();
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                h_pre.put(pre_commit[i][j+1],32);
            }
            h_pre.digest(pre_commit[i][0]);
            run_count++;
        }
        
        for(Circuit* ptr:pre_circuit)
        {
            delete ptr;
        }
        pre_circuit.clear();
    }
    h_pre.reset();
    for(int i=0;i<REP;i++)
    {
        //cout<<"REP is "<<i<<" ,pre_commit"<<endl;
        h_pre.put(pre_commit[i][0],32);

    }
    h_pre.digest(new_pre_commit);
    h_online.reset();
    for(int i=0;i<run_num;i++)
    {
        h_online.put(online_commit[i],32);
    }
    h_online.digest(online_commit[run_num]);
    int commit_flag=0;
    for(int i=0;i<32;i++)
    {
        if(new_pre_commit[i]!=pre_commit[REP][0][i])
        {
            cout<<"error precommit"<<endl;
            commit_flag++;
        }
        
    }
    if(commit_flag==0)
    {
        cout<<"success pre_commit"<<endl;
    }

    delete[] new_pre_commit;
    delete[] run_round;
    delete[] pre_all_a;
    delete[] pre_all_b;
}

void read_proof(char* &commit_all,bool**&open_party,vector<vector<vector<char>>>&master_seed,vector<vector<vector<char>>> &seed,vector<vector<vector<char>>> &online_msg,char*** &pre_commit,vector<vector<char>> &aux)
{
    cout<<"enter read_proof"<<endl;
    //online_msg  [run_num][n+1][input+andgate_num+output] online比较特殊
    //seed  [REP][n][32]
    //pre_commit [REP+1][N+1]
    //master_seed [prover_num][REP][32]
    //aux [REP][andgate]
    vector<vector<char>>read_seed ;
    vector<int> seed_idx;
    FILE *fp[party_in_a_head*prover_num];
    vector<Seed_Tree*> seed_tree;
    my::PRG p;
    int send_node_num;
    bool* run_round=new bool[REP];
    for(int i=0;i<party_in_a_head*prover_num;i++)
    {
        string name="view_"+to_string(i+1)+".bin";//打开
        fp[i]=fopen(name.c_str(),"rb");//建立指针
    }
    //将每个prover的commit和pre_commit_all读取出来,本来放在每个prover的第一个party中
    for(int i=0;i<prover_num;i++)
    {
        fread(pre_commit[REP][0],1,32,fp[i*party_in_a_head]);
        fread(commit_all,1,32,fp[i*party_in_a_head]);
        fread(&send_node_num,sizeof(int),1,fp[i*party_in_a_head]);
        cout<<"prover "<<i+1<<" send_node_num : "<<send_node_num<<endl;
        read_seed.resize(send_node_num);
        seed_idx.resize(send_node_num);
        for(int j=0;j<send_node_num;j++)
        {
            fread(&seed_idx[j],sizeof(int),1,fp[i*party_in_a_head]);
            read_seed[j].resize(32);
            fread(read_seed[j].data(),1,32,fp[i*party_in_a_head]);
        }
        int num_leaves=get_numleaves(REP);
        seed_tree.push_back(new Seed_Tree(num_leaves,read_seed,seed_idx));
        //
        cout<<"test1"<<endl;
        seed_tree[i]->set_master_seed(master_seed[i],REP);//这里把树里面的叶节点seed给vector seed
        for(int j=0;j<REP;j++)
        {
            p.reseed((unsigned char*)master_seed[i][j].data());
            for(int k=i*party_in_a_head;k<(i+1)*party_in_a_head;k++)
            {
                p.random_data(seed[j][k].data(),32);
            }

        }
    }
    p.reseed((unsigned char*) pre_commit[REP][0]);
    get_online_round(p,run_num,run_round);
    p.reseed((unsigned char*) commit_all);
    //bool* open_party = new bool[party_in_a_head*prover_num+1];
    char * file_in_pre = new char[(andgate_num+7)/8];
    char * file_in_online = new char[(andgate_num+input_len+output_len+7)/8];
    for(int i=0;i<run_num;i++)
    {
        get_openparty(p,open_party[i]);
    }
    int run_count=0;
    for(int i=0;i<REP;i++)
    {
        //cout<<"REP is "<<i<<endl;
        if(run_round[i]==0)//保存mater_seed
        {
            /*for(int j=0;j<prover_num;j++)
            {
                //fread(master_seed[j][i].data(),1,32,fp[j*party_in_a_head]);
                p.reseed((unsigned char*) master_seed[j][i].data());
                for(int k=j*party_in_a_head;k<(j+1)*party_in_a_head;k++)//012,345
                {
                    p.random_data(seed[i][k].data(),32);
                }
            }*/
        
        }
        else// online计算
        {
            
            for(int j=0;j<prover_num*party_in_a_head;j++)
            {
                //cout<<"party is "<<j+1<<endl;
                if(open_party[run_count][j+1]==1)//选择打开
                {
                   // cout<<"enter opened reading,party is "<<j+1<<endl;
                    fread(seed[i][j].data(),1,32,fp[j]);
                    if(j==(prover_num*party_in_a_head-1))
                    {
                        fread(file_in_online,1,(andgate_num+7)/8,fp[j]);
                        byte_to_bool(file_in_online,aux[i],andgate_num);
                    }
                    //cout<<"read pre_view"<<endl;
                    fread(file_in_online,1,(input_len+7)/8,fp[j]);
                    byte_to_bool(file_in_online,online_msg[run_count][j+1],input_len);
                    //cout<<"open reading is over"<<endl;
                }
                else
                {
                    //cout<<"enter unopened reading,party is "<<j+1<<endl;
                    fread(pre_commit[i][j+1],1,32,fp[j]);
                    //cout<<"1"<<endl;
                    fread(file_in_online,1,(input_len+andgate_num+output_len+7)/8,fp[j]);
                    //cout<<"2"<<endl;
                    byte_to_bool(file_in_online,online_msg[run_count][j+1],input_len+andgate_num+output_len);
                    //cout<<"unopened reading is over"<<endl;
                }

            }
        run_count++;
        }
 

    }
    cout<<"exit the read proof"<<endl;
    //cout<<"read proof is over"<<endl;
    for(int i=0;i<party_in_a_head;i++)
    {
        fclose(fp[i]);
        //delete[] fp[i];
    }

    //delete[] open_party;
    delete[] file_in_online;
    delete[] file_in_pre;
    delete[] run_round;

}
int main(int argc,char **argv)
{
    if(argc!=2){
        puts("./main <circuit_file> ");
        return 0;
    }

    sscanf(argv[1],"%s",&filename);
    Circuit circuit(filename);

    input_len = circuit.n1;
    output_len = circuit.n3;
    andgate_num = circuit.num_andgate; //全局变量
    num_gate = circuit.num_gate;
    wire_num = circuit.num_wire;
    char * commit_all = new char[32];
    bool** open_party = new bool*[run_num];
    char *** pre_commit = new char**[REP+1];//[REP+1][N+1][32]
    char ** online_commit = new char*[run_num+1];//[REP+1][32];
    vector<vector<char>> aux;
    vector<vector<vector<char>>> master_seed;// master_seed [prover_num][REP][32]
    aux.resize(REP);
    master_seed.resize(prover_num);
    for(int i=0;i<prover_num;i++)
    {
        master_seed[i].resize(REP);
        for(int j=0;j<REP;j++)
        {
            master_seed[i][j].resize(32);
        }
    }
    for(int i=0;i<REP;i++)
    {
        //open_party[i] =new bool[party_in_a_head*prover_num+1];
        pre_commit[i] = new char*[prover_num*party_in_a_head+1];
        //master_seed[i].resize(prover_num);
        //online_commit[i] =new char[32];
        aux[i].resize(andgate_num);
        for(int j=0;j<=prover_num*party_in_a_head;j++)
        {
            pre_commit[i][j] = new char[32];
        }

    }
    pre_commit[REP] = new char*[prover_num*party_in_a_head+1];
    for(int i=0;i<run_num;i++)
    {
        open_party[i] = new bool[party_in_a_head*prover_num+1];
    }
    for(int i=0;i<run_num+1;i++)
    {
        online_commit[i] = new char[32];
    }
    for(int j=0;j<=prover_num*party_in_a_head;j++)
    {
        pre_commit[REP][j] = new char[32];
    }
    //vector<vector<vector<vector<char>>>> pre_view;//[n][REP][party_in_a_head*prover_num][andgate_num]
    vector<vector<vector<char>>> online_msg;//[run_num][n+1][input+andgate_num+output]
    vector<vector<vector<char>>> seed;


    online_msg.resize(run_num);
    for(int i=0;i<run_num;i++)
    {
        online_msg[i].resize(prover_num*party_in_a_head+1);
        for(int j=0;j<prover_num*party_in_a_head+1;j++)
        {
            online_msg[i][j].resize(input_len+andgate_num+output_len,0);
        }
    }
    seed.resize(REP);
    for(int i=0;i<REP;i++)
    {
        //online_msg[i].resize(prover_num*party_in_a_head+1);
        seed[i].resize(prover_num*party_in_a_head);
        //online_msg[i][0].resize(input_len+andgate_num+output_len);
        for(int j=0;j<prover_num*party_in_a_head;j++)
        {
            //online_msg[i][j+1].resize(input_len+andgate_num+output_len,0);
            seed[i][j].resize(32);
        }
    }
    read_proof(commit_all,open_party,master_seed,seed,online_msg,pre_commit,aux);
    pre_process(filename,pre_commit,seed,aux,open_party,online_msg,online_commit);
    my::Hash h;
    h.reset();
    char*  new_final_commit=new char[32];
    h.put(pre_commit[REP][0],32);
    h.put(online_commit[run_num],32);
    h.digest(new_final_commit);
    int commit_error_flag=0;
    for(int i=0;i<32;i++)
    {
        if(new_final_commit[i]!=commit_all[i])
        {
            cout<<"wrong commit all"<<endl;
            commit_error_flag=1;
        }
    }
    if(commit_error_flag==0)
    cout<<"success commit"<<endl;
    //recompute(filename,open_party,seed,pre_view,online_msg,pre_commit,online_commit);
   /* char* v_commit_all=new char[32];
    Hash h;
    h.put(pre_commit[REP][0],32);
    h.put(online_commit[REP],32);
    h.digest(v_commit_all);
    int error_flag=0;
    for(int i=0;i<32;i++)
    {
        if(v_commit_all[i]!=commit_all[i])
        {
            error_flag = 1;
        }
    }
    if(error_flag==1)
    {
        cout<<"error commit"<<endl;
    }
    else{cout<<"commit is right"<<endl;}*/
    //////////
    delete[] commit_all;
    delete[] new_final_commit;
    //delete[] v_commit_all;
    delete_3Darray(pre_commit,REP+1,party_in_a_head*prover_num+1);
    for(int i=0;i<run_num;i++)
    {
        delete[] open_party[i];
        delete[] online_commit[i];
    }
    delete[] online_commit[run_num];
}