#ifndef DH_HPP
#define DH_HPP

#include <vector>
#include <string> 
#include "group.hpp"
#include "hash.hpp"
#include "prg.hpp"
#include "openssl/aes.h"
#include "openssl/evp.h"

// 加密函数
void encrypt_aes_ecb(const unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *ciphertext) {
    AES_KEY enc_key;
    AES_set_encrypt_key(key, 128, &enc_key);

    int blocks = (plaintext_len + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE; // 计算需要加密的数据块数量

    for (int i = 0; i < blocks; i++) {
        AES_encrypt(plaintext + (i * AES_BLOCK_SIZE), ciphertext + (i * AES_BLOCK_SIZE), &enc_key);
    }
}

// 解密函数
void decrypt_aes_ecb(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *plaintext) {
    AES_KEY dec_key;
    AES_set_decrypt_key(key, 128, &dec_key);

    int blocks = ciphertext_len / AES_BLOCK_SIZE; // 计算加密数据块的数量

    for (int i = 0; i < blocks; i++) {
        AES_decrypt(ciphertext + (i * AES_BLOCK_SIZE), plaintext + (i * AES_BLOCK_SIZE), &dec_key);
    }
}


template<class MIO>
class Key_Ex{
public:
	MIO *io;

	Group *G = nullptr; 
    Key_Ex(MIO *io){
		this->io=io;
		G = new Group();
    }
 
	~Key_Ex() {
		delete G;
	}

	void send(int id,const bool *data0, int length) {

		BigInt a;
        Point A;
		unsigned char res[2][Hash::DIGEST_SIZE];
//		Point * B = new Point[length];
		Point  B ;
		bool send[length];


		G->get_rand_bn(a);
		A = G->mul_gen(a);
		io->send_pt(id,&A);

		io->recv_pt(id,G, &B);
		B = B.mul(a);//B的a次方
		io->flush();
		Hash::KDF(&res[0][0],B);


		//AES encrypt
		encrypt_aes_ecb((unsigned char *)data0,length,res[0],(unsigned char*)send);
	
	//	PRG prg;
	//	prg.reseed(res[0]);
	//	for(int i=0;i<length;i++){
	//	send[i]=(prg.rand()%2)^data0[i];
	//	}


		io->send_data(id,send, length*sizeof(bool));
		io->flush();
	//	delete[] B;
	}

	void recv(int id,bool *data, int length) {

		//cout<<"enter the recv function"<<endl;
		BigInt bb ;
//		Point * B = new Point[length],
		Point  B ,
//				* As = new Point[length],
				As,
				A;
		bool recv[length];

		G->get_rand_bn(bb);
 
		io->recv_pt(id,G, &A); 
		//cout<<"recv A"<<endl;
		B = G->mul_gen(bb);
		io->send_pt(id,&B);
		//cout<<"send B"<<endl;
		io->flush();

		unsigned char res[2][Hash::DIGEST_SIZE];

		As = A.mul(bb);//A的b次方
		Hash::KDF(&res[0][0],As);
	/*	PRG prg;
		prg.reseed(res[0]);
	*/ // cout<<"begin to recv data:"<<length<<endl;
		io->recv_data(id,recv, length*sizeof(bool));
		//cout<<"recv data"<<endl;
		io->flush();
		decrypt_aes_ecb((unsigned char *)recv,length,res[0],(unsigned char*)data);
	/*	for(int i=0;i<length;i++){
		data[i]=(prg.rand()%2)^recv[i];
		}
	*/

		
	//	delete[] bb;
	//	delete[] B;
	//	delete[] As;
	}
	
};



#endif