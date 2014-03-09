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
		std::cerr<<"w: "<<w<<"\n";
		std::cerr<<"h: "<<h<<"\n";
		
		unsigned bits=8;
		if(argc>3){
			bits=atoi(argv[3]);
		}
		std::cerr<<"bits: "<<bits<<"\n";
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
		if(abslevels > vmin(h/4,w/4,64))
		{
			abslevels = vmin(h/4,w/4,64);
			
		}
		std::cerr<<"levels: "<<levels<<"\n";
		std::cerr<<"abslevels: "<<abslevels<<"\n";
		unsigned k = 2;
		while(((h/k)*w*bits)/8 > 1000000)
		{
			if((h/(2*k)) < 1+2*abslevels)
			{
				break;
			}
			k *= 2;
		}
		unsigned data = h/k;
		std::cerr<<"data: "<<data<<"\n";
		
		std::cerr<<"k: "<<k<<"\n";
		
		fprintf(stderr, "Processing %d x %d image with %d bits per pixel.\n", w, h, bits);
		
		

		uint64_t cbRaw_out=uint64_t(w)*data*bits/8; //take four rows at a time
        uint64_t cbRaw_final=uint64_t(w)*(data + 2*abslevels)*bits/8;
		uint64_t cbRaw_init=uint64_t(w)*(data - 2*abslevels)*bits/8;
		
		
		std::cerr<<"cbRaw_out: "<<cbRaw_out<<"\n";
		uint32_t count = 0;
		std::vector<uint64_t> raw_final(cbRaw_final/8);
		
		uint64_t cbPixels = uint64_t(w)*(data + 4*abslevels);
		uint64_t cbStore = uint64_t(w)*4*abslevels;
		uint64_t cbRaw_in=uint64_t(w)*data*bits/8;
		std::cerr<<"cbRaw_in: "<<cbRaw_in<<"\n";
		std::cerr<<"cbPixels: "<<cbPixels<<"\n";
		std::cerr<<"cbStore: "<<cbStore<<"\n";
				
		std::vector<uint64_t> raw_in(cbRaw_in/8);
		std::vector<uint64_t> raw_init(cbRaw_init/8);
		std::vector<uint64_t> raw_out(cbRaw_out/8);
		
		std::vector<uint32_t> pixels(cbPixels);
		std::vector<uint32_t> store(cbStore);
		
		uint64_t TopGot = 0;
		uint64_t store_p;
		
		while(1){
			if(count==0)
			{
				if(!read_blob(STDIN_FILENO, cbRaw_in, &raw_in[0]))
      			{		
      				break;	// No more images   
      			}
				std::cerr<<"Read Frame: "<<count<<"\n\n";
				TopGot += cbRaw_in;
				std::cerr<<"Total Read: "<<TopGot<<"\n\n";
				unpack_blob(w, data, bits, &raw_in[0], &pixels[0]);
				
				//store bottom rows of pixels
				if(data < 4*abslevels){
					std::copy(pixels.begin(), pixels.begin()+data*w, store.begin());
                    store_p = data*w;
				}else{
					std::copy(pixels.end() - 8*abslevels*w, pixels.end()-4*w*abslevels, store.begin());
                    store_p = store.size();
				}
				
				process(levels, w, data, k, pixels, count);
				//invert(levels, w, h, bits, pixels);
				
	      		count++;//increment frame count
                
				pack_blob(w, data - 2*levels, bits, &pixels[0], &raw_init[0]);
				write_blob(STDOUT_FILENO, cbRaw_init, &raw_init[0]);
			}else if(count == k-1){
				if(!read_blob(STDIN_FILENO, cbRaw_in, &raw_in[0]))
      			{
      				break;	// No more images   
      			}
				std::cerr<<"Read Frame: "<<count<<"\n\n";
				TopGot += cbRaw_in;
				std::cerr<<"Total Read: "<<TopGot<<"\n\n";
                
                
				std::copy(store.begin(), store.begin()+store_p, pixels.begin());
				
				unpack_blob(w, data, bits, &raw_in[0], &pixels[store_p]);

				process(levels, w, data + 4*abslevels, k, pixels, count);
				//process 
				
				count++; // increment frame count
				
				pack_blob(w, data + 2*abslevels, bits, &pixels[2*w*abslevels], &raw_final[0]);
				//repack the bottom data rows to the raw output buffer
				write_blob(STDOUT_FILENO, cbRaw_final, &raw_final[0]);
				//write raw buffer to stdout
			}else{
				if(!read_blob(STDIN_FILENO, cbRaw_in, &raw_in[0]))
      			{
      				break;	// No more images   
      			}
				std::cerr<<"Read Frame: "<<count<<"\n\n";
				TopGot += cbRaw_in;
				std::cerr<<"Total Read: "<<TopGot<<"\n\n";
				
				std::copy(store.begin(), store.begin()+store_p,pixels.begin());
				unpack_blob(w, data, bits, &raw_in[0], &pixels[store_p]);
				
				if(store_p != store.size())
				{
					std::copy(pixels.end() - 8*abslevels*w + store_p, pixels.end() - 4*w*abslevels + store_p, store.begin());	
				}else{
					std::copy(pixels.end() - 4*abslevels*w, pixels.end(), store.begin());
				}			

				process(levels, w, data+store_p/w, k, pixels, count);
				//process 
				
				count++; // increment frame count
				
				pack_blob(w, data, bits, &pixels[2*abslevels*w], &raw_out[0]);
				//repack the bottom data rows to the raw output buffer
				write_blob(STDOUT_FILENO, cbRaw_out, &raw_out[0]);
				//write raw buffer to stdout
			}
		}
		
		return 0;
	}catch(std::exception &e){
		std::cerr<<"Caught exception : "<<e.what()<<"\n";
		return 1;
	}
}