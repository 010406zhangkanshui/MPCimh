#ifndef CONSTANT_H_
#define CONSTANT_H_

typedef unsigned char boolean;
const int party_in_a_head = 3;
const int prover_num = 3;
const int run_num=81;//online的个数
const int parallel_num = 40;//同时并行做online阶段的个数
//const int all_party = 6;
const int n=prover_num*party_in_a_head+1;
//const int open_num=16;
//const int open_pre =10;//预处理打开的数量
const int REP=229;//pre的个数
const int MAX_SIZE=1024*1024*256;//max proof size , 256MB 
//const int NETWORK_BUFFER_SIZE=65536;
#define COMP_IMPL


#endif  
