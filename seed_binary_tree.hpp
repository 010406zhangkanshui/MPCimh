#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prg.hpp"
#include <cmath>

// 结点结构
/*struct Node {
    char seed[32]; // 字符数组用于存储标签seed
    int Is_leave;
    struct Node* left;
    struct Node* right;
};*/


/* struct TreeNode {
    int val;
    char seed[32]; // 添加 char 数组
    int Is_leave;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};*/

class Seed_Tree{
    public:

    struct TreeNode {
    int val;
    char seed[32]; // 添加 char 数组
    int Is_leave;
    int send_flag=0;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    };

    int num_leaves;
    int layer;
    vector<TreeNode*> nodes;

    Seed_Tree(int N) {
        num_leaves = N;
        nodes.resize(N+1);
        generateCompleteBinaryTree();
        layer = floor(log2(num_leaves));
    }
    Seed_Tree(int N,vector<vector<char>> &read_seed,vector<int> &seed_idx)
    {
        num_leaves=N;
        nodes.resize(N+1);
        Restore_Tree(read_seed,seed_idx);
        layer = floor(log2(num_leaves));

    }
    void Restore_Tree(vector<vector<char>> &read_seed,vector<int> &seed_idx)
    {
        vector<int> flag;
        flag.resize(num_leaves+1,0);
        for (int i = 1; i <= num_leaves; ++i) {
            nodes[i] = new TreeNode(i);
        }
        
        for(int i=0;i<seed_idx.size();i++)
        {
            int idx = seed_idx[i];
            flag[idx]=1;
            for(int j =0;j<32;j++)
            {
                //nodes[idx] =new TreeNode(idx);
                nodes[idx]->seed[j]=read_seed[i][j];
            }
            //PRG p;
        }
        my::PRG prg;
        for(int i=1;i<=num_leaves/2;i++)
        {
            if(flag[i]==1)//刚才被放入了种子
            {
                prg.reseed((unsigned char*) nodes[i]->seed);
                if(i*2<=num_leaves)
                {
                    for(int j=0;j<32;j++)
                    {
                        nodes[2*i]->seed[j]=prg.rand();
                    }
                    flag[2*i]=1;
                }
                if(i*2<=num_leaves)
                {
                    for(int j=0;j<32;j++)
                    {
                        nodes[2*i+1]->seed[j]=prg.rand();
                    }
                    flag[2*i+1]=1;

                }
            }
        }//此时底层应该被重构完毕

    }
    void generateCompleteBinaryTree() 
    {
        //if (N <= 0)
       // return nullptr;
        //vector<TreeNode*> nodes(N + 1,nullptr); // 创建一个存储节点指针的数组
        // 生成每个节点，并存储到数组中
        for (int i = 1; i <= num_leaves; ++i) {
            nodes[i] = new TreeNode(i);
        }
    
        my::PRG prg;
        // nodes[1]->seed;
        strncpy(nodes[1]->seed,(char*) prg.key, 32);
        // 构建树的结构
        for (int i = 1; i <= num_leaves / 2; ++i) {
            if(i!=1)
            {
                prg.reseed((unsigned char*)nodes[i]->seed);
            }

            if (2 * i <= num_leaves)
            {
                nodes[i]->left = nodes[2 * i];
                for(int j=0;j<32;j++)
                {
                    nodes[2*i]->seed[j]=prg.rand();
                }
            }
            
            if (2 * i + 1 <= num_leaves)
            {
                nodes[i]->right = nodes[2 * i + 1];
                for(int j=0;j<32;j++)
                {
                    nodes[2*i+1]->seed[j]=prg.rand();
                }
            }
            
        }
    }

    void inorderTraversal(TreeNode* root) {
        if (root == nullptr)
            return;
        inorderTraversal(root->left);
        std::cout << root->val << " 's is leave "<<root->Is_leave<<endl;
        inorderTraversal(root->right);
    }
    ~Seed_Tree()
    {
        delete_tree();
    }

    int find_node(int * unopened_leaves, int unopened_size);
    void delete_tree();
    void set_seed(vector<vector<vector<char>>> &seed, int rep, int party_num);
    void set_seed(vector<vector<vector<char>>> &seed, int rep, int party_num,int prover);
    void set_master_seed(vector<vector<char>> &master_seed,int rep);
    //void Reconstruct(int idx,char* seed);
};


/*void generateCompleteBinaryTree(int N) {
    if (N <= 0)
        return nullptr;

    //vector<TreeNode*> nodes(N + 1,nullptr); // 创建一个存储节点指针的数组

    // 生成每个节点，并存储到数组中
    for (int i = 1; i <= N; ++i) {
        nodes[i] = new TreeNode(i);
    }
    
    PRG prg;
    // nodes[1]->seed;
    // strncpy(nodes[1]->seed,(char*) prg.key, 32);
    // 构建树的结构
    for (int i = 1; i <= N / 2; ++i) {
        if(i!=1)
        {
            prg.reseed((unsigned char*)nodes[i]->seed);
        }
        

        if (2 * i <= N)
        {
            nodes[i]->left = nodes[2 * i];
            for(int j=0;j<32;j++)
            {
                nodes[2*i]->seed[j]=prg.rand();
            }
        }
            
        if (2 * i + 1 <= N)
        {
            nodes[i]->right = nodes[2 * i + 1];
            for(int j=0;j<32;j++)
            {
                nodes[2*i+1]->seed[j]=prg.rand();
            }
        }
            
    }

    return nodes[1]; // 返回根节点
}*/

/*void inorderTraversal(TreeNode* root) {
    if (root == nullptr)
        return;
    inorderTraversal(root->left);
    std::cout << root->val << " 's is leave "<<root->Is_leave<<endl;
    inorderTraversal(root->right);
}*/
int Seed_Tree:: find_node(int * unopened_leaves, int unopened_size)
{
    //vector<int> nodes;
    //nodes.resize(num_leaves+1,0);
    vector<vector<char>> send_node;
    int send_num=0;

    for(int i=0;i<unopened_size;i++)
    {
        int father=unopened_leaves[i]+pow(2,layer)-1;//unopened_leaves从1开始
        while(father>=1)
        {
            nodes[father]->send_flag=1;
            father=father/2;
        }
    }
    int remain_last_node = pow(2,layer+1)-1;
    cout<<"remain_last_node : "<<remain_last_node<<endl;
    for(int node=num_leaves+1;node<=remain_last_node;node++)//对于不存在的叶子节点
    {
        int father=node/2;
        while(father>=1)
        {
            nodes[father]->send_flag=1;
            father=father/2;
        }
    }
    //int layer=floor(log2(num_leaves));
    cout<<"layer is "<<layer<<endl;
    int underlying_first =pow(2,layer);
    cout<<"underlying first is : "<<underlying_first<<endl;
    for(int i=1;i<=num_leaves;i++)
    {
        if((i%2)==1&&i>1&&nodes[i]->send_flag==1)//奇数并在路径上
        {
            if(nodes[i-1]->send_flag==0)//兄弟节点不在路径上
            {
                nodes[i-1]->send_flag=2;   
            }
        }
        else if((i%2)==0&&i>1&&nodes[i]->send_flag==1)//偶数并在路径上
        {
            if(nodes[i+1]->send_flag==0)//兄弟节点不在路径上
            {
                nodes[i+1]->send_flag=2;
               /* if((i+1)<underlying_first&&(2*(i+1))>num_leaves)//兄弟节点是非底层的叶子节点
                {

                }
                else
                {
                    nodes[i+1]->send_flag=2;
                }*/
            }
        }
    }
    //cout<<" send_flag : "<<endl;
    for(int i=1;i<=num_leaves;i++)//0-不在路径上且也无需发送,1-在路径上,2-兄弟节点
    {
        //cout<<nodes[i]->send_flag;
        if(nodes[i]->send_flag==2)
        {
            //send_node.push_back(nodes[i]->seed);
            send_num++;
        }
    }
    cout<<"exit find node"<<endl;
    return send_num;

}


void Seed_Tree:: delete_tree()
{
    for(int i=1;i<=num_leaves;i++)
        delete nodes[i];
}
void Seed_Tree:: set_seed(vector<vector<vector<char>>> &seed, int rep, int party_num)
{
    int underlying_first =pow(2,layer);
    cout<<"underlying first is : "<<underlying_first<<endl;

    for(int i = 0;i<party_num;i++)
    {
        for(int j=0;j<rep;j++)
        {
            for(int k=0;k<32;k++)
            {
                seed[j][i][k]=nodes[underlying_first+i*rep+j]->seed[k];
            }
            
        }
    }
}

void Seed_Tree:: set_seed(vector<vector<vector<char>>> &seed, int rep, int party_num,int prover)
{
    int underlying_first = pow(2,layer);
    cout<<"underlying first is : "<<underlying_first<<endl;

    for(int i = 0;i<party_num;i++)
    {
        for(int j=0;j<rep;j++)
        {
            for(int k=0;k<32;k++)
            {
                seed[j][i+prover*party_in_a_head][k]=nodes[underlying_first+i*rep+j]->seed[k];
            } 
        }
    }
}
void Seed_Tree:: set_master_seed(vector<vector<char>> &master_seed,int rep)
{
    int underlying_first =pow(2,layer);
    cout<<"underlying first is : "<<underlying_first<<endl;


        for(int j=0;j<rep;j++)
        {
            for(int k=0;k<32;k++)
            {
                master_seed[j][k]=nodes[underlying_first+j]->seed[k];
            }
            
        }
}