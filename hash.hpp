#ifndef HASH_HPP__
#define HASH_HPP__

#include <openssl/sha.h>
#include <cstdio>
#include <cstring>
#include "group.hpp"
#include <openssl/evp.h>
#include <immintrin.h>
#include <iostream>
using namespace std;
typedef __m128i block ;
namespace my{
class Hash { public:
	SHA256_CTX hsh;
	EVP_MD_CTX *mdctx;
    static const int HASH_BUFFER_SIZE=65536;
	char buffer[HASH_BUFFER_SIZE];
	int size = 0;
	static const int DIGEST_SIZE = 32;
	Hash() {
		SHA256_Init(&hsh);
	}
	~Hash() {
	}
	void put(const void * data, int nbyte) {//数据与数据大小
		if (nbyte > HASH_BUFFER_SIZE)
			SHA256_Update(&hsh, data, nbyte);
			//直接处理数据块，添加到哈希运算中
		else if(size + nbyte < HASH_BUFFER_SIZE) {
			memcpy(buffer+size, data, nbyte);
			size+=nbyte;
			//如果当前已经存储在缓冲区中的数据加上传入的数据仍然小于HASH_BUFFER_SIZE，那么就将这个新数据块复制到缓冲区的末尾，并更新缓冲区的大小。
		} else {
			SHA256_Update(&hsh, (char*)buffer, size);
			memcpy(buffer, data, nbyte);
			size = nbyte;
			//那么就先使用SHA256_Update函数处理当前缓冲区中的数据，然后将新数据块复制到缓冲区的前面，并更新缓冲区的大小
		}
	}
	void digest(char * a) {
		if(size > 0) {
			SHA256_Update(&hsh, (char*)buffer, size);
			size=0;
		}
		SHA256_Final((unsigned char *)a, &hsh);
	}
	
	void reset() {
		SHA256_Init(&hsh);
		size=0;
	}
	static void hash(void * digest, const void * data, int nbyte) {
		(void )SHA256((const unsigned char *)data, nbyte, (unsigned char *)digest);
	}
	/*#ifdef __x86_64__
	__attribute__((target("sse2")))
	#endif
	static block hash_for_block(const void * data, int nbyte) {
		char digest[DIGEST_SIZE];
		hash_once(digest, data, nbyte);
		return _mm_load_si128((__m128i*)&digest[0]);
	}*/

	static void KDF(unsigned char *out, Point &in) {
		size_t len = in.size();
	//	cout<<"iii"<<endl;
		in.group->resize_scratch(len);
		unsigned char * tmp = in.group->scratch;
		in.to_bin(tmp, len);
        hash(out,tmp,len);
	}
	static void hash_once(char * dgst, const void * data, int nbyte) {
		Hash hash;
		hash.put(data, nbyte);
		hash.digest(dgst);
	}
	#ifdef __x86_64__
	__attribute__((target("sse2")))
	#endif
	static block hash_for_block(const void * data, int nbyte) {
	//	cout<<"enter the block"<<endl;
		char digest[DIGEST_SIZE];
		hash_once(digest, data, nbyte);
		return _mm_load_si128((__m128i*)&digest[0]);
	}
	static block KDF(Point &in, uint64_t id = 1) {
		//cout<<"enter KDF"<<endl;
		size_t len = in.size();
	//	cout<<"kkk"<<endl;
		in.group->resize_scratch(len+8);
		
		unsigned char * tmp = in.group->scratch;
		in.to_bin(tmp, len);
		memcpy(tmp+len, &id, 8);
		
		block ret = hash_for_block(tmp, len+8);
		return ret;
	}	
};}

#endif// HASH_H__
