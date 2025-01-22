#ifndef PRG_HPP__
#define PRG_HPP__

#include <memory>
#include <algorithm>
#include <random> 
#include "hash.hpp"
namespace my{
class PRG { public:
	uint64_t counter = 0;
	uint64_t key[2];
    my::Hash hash;

	PRG(const void * seed = nullptr) {	
		if (seed != nullptr) {
			reseed((const unsigned char *)seed);
		} else {
			unsigned int *v=(unsigned int*)key;
            std::random_device rand_div;
            for(int i=0;i<sizeof(key)/sizeof(unsigned int);i++)
                v[i]=rand_div();
			counter=0;
		}
	}
	void reseed(const unsigned char* seed) {
        memcpy(key,seed,sizeof(key));
		counter = 0;
	}

	void reseed(const block* seed) {
		memcpy(key,seed,sizeof(key));
		counter = 0;
	}    

	void random_data(void *data, int nbytes) { 
        unsigned char tmp[32];
        uint64_t ctr[3];
        memcpy(ctr,key,sizeof(key));
        int left=nbytes;
        int cur=0;
        while(left>0){
            ctr[2]=counter;
            my::Hash::hash(tmp,ctr,sizeof(ctr));
            memcpy(data+cur*32,tmp,std::min(left,32));//所有的32都是digest——size
            counter++;    
            cur++;
            left-=32;
        }
	}



	void random_bool(bool * data, int length) {
		uint8_t * uint_data = (uint8_t*)data;
		random_data(uint_data, length);
		for(int i = 0; i < length; ++i)
			data[i] = uint_data[i] & 1;
	}


    unsigned int rand(){
        unsigned int res;
        random_data(&res,sizeof(res));
        return res;
    } 
};}

#endif// PRP_H__