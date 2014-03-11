#include "processes.hpp"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
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

		if(bits>32){
			throw std::invalid_argument("Bits must be <= 32.");
        }
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
		unsigned k = 2;
        uint64_t frameSize =uint64_t(h/k)*uint64_t(w)*uint64_t(bits)/8; //measure frame size for k = 2
        while(frameSize > 10000) 	//if frame > 10kB then double the number of frames per image until each frame
		{							//is less than 10kB
			if((h/(2*k)) < 1+2*abslevels)
			{
				break;
			}
			k *= 2;
			frameSize =uint64_t(h/k)*uint64_t(w)*uint64_t(bits)/8;
		}
		while (h%k != 0) {
            k += 1;
            if((h/(k)) < 1+2*abslevels)
			{
				break;
			}
            std::cerr<<"k = "<<k<<"\n";
		}
        if((h/(k)) < 1+2*abslevels)
        {
            while (h%k != 0)
            {
                k -= 1;
				if(k==1)
				{
					break;
				}
            }
        }
		unsigned data = h/k; // each frame is h/k rows each

		fprintf(stderr, "Processing %d x %d image with %d bits per pixel.\n", w, h, bits);
    	uint64_t cbRaw_out=uint64_t(w)*data*bits/8; //Intermedite output buffer size
        uint64_t cbRaw_final=uint64_t(w)*(data + 2*abslevels)*bits/8; //final frame output buffer size
		uint64_t cbRaw_init=uint64_t(w)*(data - 2*abslevels)*bits/8; //initial frame output buffer size

		uint32_t count = 0; // frame count

		uint64_t cbPixels = uint64_t(w)*(data + 4*abslevels); // processing container size
		uint64_t cbPixels_init = uint64_t(w)*(data); // processing container size
		uint64_t cbStore = uint64_t(w)*4*abslevels; //interframe store size
		uint64_t cbRaw_in=uint64_t(w)*data*bits/8; 	//input frame size

		//raw image containers		
		std::vector<uint64_t> raw_in(cbRaw_in/8);
		std::vector<uint64_t> raw_init(cbRaw_init/8);
		std::vector<uint64_t> raw_out(cbRaw_out/8);
		std::vector<uint64_t> raw_final(cbRaw_final/8);

		std::vector<uint32_t> pixels(cbPixels); // processing container
		std::vector<uint32_t> pixels_init(cbPixels_init); // processing container
		std::vector<uint32_t> store(cbStore); 	//interframe store
		
        uint64_t store_p; //store pointer

		while(1){
			if(count==0)
			{
				if(!read_blob(STDIN_FILENO, cbRaw_in, &raw_in[0]))
      			{		
      				break;	// No more images   
      			}
				unpack_blob(w, data, bits, &raw_in[0], &pixels_init[0]);

				//store bottom rows of pixels
				if(data < 4*abslevels){ // if first frame is smaller than 4*abslevels rows then store all frame
					std::copy(pixels_init.begin(), pixels_init.end(), store.begin());
                    store_p = data*w;
				}else{
					std::copy(pixels_init.end() - 4*abslevels*w, pixels_init.end(), store.begin());
         	        store_p = store.size(); //else store the bottom 4*abslevels rows of the first frame
				}

				process(levels, w, data, k, pixels_init, count, divide);
				//invert(levels, w, h, bits, pixels);

	      		count++;//increment frame count
                
				pack_blob(w, data - 2*abslevels, bits, &pixels_init[0], &raw_init[0]);
				write_blob(STDOUT_FILENO, cbRaw_init, &raw_init[0]);
			}else if(count == k-1){
				if(!read_blob(STDIN_FILENO, cbRaw_in, &raw_in[0]))
      			{
      				break;	// No more images   
      			}

				std::copy(store.begin(), store.begin()+store_p, pixels.begin()); //put the stored rows at the top 
				//of the processing frame

				unpack_blob(w, data, bits, &raw_in[0], &pixels[store_p]);
				//write the new frame below the rows from the previous frame

				process(levels, w, data + 4*abslevels, k, pixels, count, divide);
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
				
                std::copy(store.begin(), store.begin()+store_p,pixels.begin());//put the stored rows at the top 
				//of the processing frame

				unpack_blob(w, data, bits, &raw_in[0], &pixels[store_p]);
				//write the new frame below the rows from the previous frame

				if(store_p != store.size()) //if store not full copy only the filled rows to the processing vector
				{
					std::copy(pixels.end() - 8*abslevels*w + store_p, pixels.end() - 4*w*abslevels + store_p, store.begin());	
				}else{
					std::copy(pixels.end() - 4*abslevels*w, pixels.end(), store.begin());
				//	else write whole store to pixels
				}			
				process(levels, w, data+store_p/w, k, pixels, count, divide);
				//process 
				store_p = store.size(); //store now full

				count++; // increment frame count

				pack_blob(w, data, bits, &pixels[2*abslevels*w], &raw_out[0]);
				//repack the bottom data rows to the raw output buffer
				write_blob(STDOUT_FILENO, cbRaw_out, &raw_out[0]);
				//write raw buffer to stdout
			}
		}
        std::cerr<<"End of program - happy processing! \n";		
		return 0;
	}catch(std::exception &e){
		std::cerr<<"Caught exception : "<<e.what()<<"\n";
		return 1;
	}
}
