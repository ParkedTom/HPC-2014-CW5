#include "processes.hpp"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
//remove before merging to master
//#define XCODE

int main(int argc, char *argv[])
{
		//remove before merging to master
    #ifdef XCODE
		freopen("input.raw", "r", stdin);
    freopen("output.raw", "w", stdout);
		#endif
	try{
		if(argc<3){
			fprintf(stderr, "Usage: process width height [bits] [levels]\n");
			fprintf(stderr, "   bits=8 by default\n");
			fprintf(stderr, "   levels=1 by default\n");
			exit(1);
		}
		
		unsigned w=atoi(argv[1]);
		unsigned h=atoi(argv[2]);
		
		unsigned bits=8;
		if(argc>3){
			bits=atoi(argv[3]);
		}
		
		if(bits>32)
			throw std::invalid_argument("Bits must be <= 32.");
		
		unsigned tmp=bits;
		while(tmp!=1){
			tmp>>=1;
			if(tmp==0)
				throw std::invalid_argument("Bits must be a binary power.");
		}
		
		if( ((w*bits)%64) != 0){
			throw std::invalid_argument(" width*bits must be divisible by 64.");
		}
		
		int levels=1;
		if(argc>4){
			levels=atoi(argv[4]);
		}
		unsigned abslevels = std::abs(levels);
		
		fprintf(stderr, "Processing %d x %d image with %d bits per pixel.\n", w, h, bits);
		
		
		uint64_t cbRaw=uint64_t(w)*(h/2)*bits/8; //take four rows at a time
		uint32_t count = 0;
		std::vector<uint64_t> raw(cbRaw/8);
		
		
		
		//values for initial frame
		uint64_t cbRaw_init=uint64_t(w)*((h/2)+(abslevels*2))*bits/8;
		std::vector<uint64_t> raw_init(cbRaw_init/8);
		
		//values for final frame
		uint64_t cbRaw_final=uint64_t(w)*((h/2)-(abslevels*2))*bits/8;
		std::vector<uint64_t> raw_final(cbRaw_final/8);
		
		std::vector<uint32_t> pixels(w*((h/2)+2*abslevels));
		std::vector<uint32_t> store(w*4*abslevels);
		
		while(1){
			if(count==0)
			{
				if(!read_blob(STDIN_FILENO, cbRaw_init, &raw_init[0]))
      	{
      		break;	// No more images   
      	}
				unpack_blob(w, ((h/2)+abslevels*2), bits, &raw_init[0], &pixels[0]);
				
				//store bottom rows of pixels
				std::copy(pixels.end()-(4*abslevels*w), pixels.end(), store.begin());
				
				process(levels, w, (h/2), bits, pixels, count);
				//invert(levels, w, h, bits, pixels);
				
	      count++;
				pack_blob(w, h/2, bits, &pixels[0], &raw[0]);
				write_blob(STDOUT_FILENO, cbRaw, &raw[0]);
			}else{
				if(!read_blob(STDIN_FILENO, cbRaw_final, &raw_final[0]))
      	{
      		break;	// No more images   
      	}
				unpack_blob(w, ((h/2)+abslevels*2), bits, &raw_final[0], &pixels[4*abslevels*w]);
				std::swap_ranges(pixels.begin(),pixels.begin()+4*abslevels*w,store.begin());
				process(levels, w, (h/2), bits, pixels, count);
				
				pack_blob(w, h/2, bits, &pixels[2*w*abslevels], &raw[0]);
				write_blob(STDOUT_FILENO, cbRaw, &raw[0]);
			}
		}
		
		return 0;
	}catch(std::exception &e){
		std::cerr<<"Caught exception : "<<e.what()<<"\n";
		return 1;
	}
}