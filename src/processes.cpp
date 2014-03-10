#include "processes.hpp"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
#define MAX 255 //This is the max value a pixel can have, for up to 8 bits per pixel
//#include "tbb/parallel_for.h"

////////////////////////////////////////////
// Routines for bringing in binary images

/*! Reverse the orders of bits if necessary
	\note This is laborious and a bit pointless. I'm sure it could be removed, or at least moved...
*/
uint64_t shuffle64(unsigned bits, uint64_t x)
{
	if(bits==1){
		x=((x&0x0101010101010101ull)<<7)
			| ((x&0x0202020202020202ull)<<5)
			| ((x&0x0404040404040404ull)<<3)
			| ((x&0x0808080808080808ull)<<1)
			| ((x&0x1010101010101010ull)>>1)
			| ((x&0x2020202020202020ull)>>3)
			| ((x&0x4040404040404040ull)>>5)
			| ((x&0x8080808080808080ull)>>7);
	}else if(bits==2){
		x=((x&0x0303030303030303ull)<<6)
			| ((x&0x0c0c0c0c0c0c0c0cull)<<2)
			| ((x&0x3030303030303030ull)>>2)
			| ((x&0xc0c0c0c0c0c0c0c0ull)>>6);
	}else if(bits==4){
		x=((x&0x0f0f0f0f0f0f0f0full)<<4)
			| ((x&0xf0f0f0f0f0f0f0f0ull)>>4);
	}
	return x;
}



/*! Take data packed into incoming format, and exand to one integer per pixel */
void unpack_blob(unsigned w, unsigned h, unsigned bits, const uint64_t *pRaw, uint32_t *pUnpacked)
{
	uint64_t buffer=0;
	unsigned bufferedBits=0;
	
	const uint64_t MASK=0xFFFFFFFFFFFFFFFFULL>>(64-bits);
	
	for(unsigned i=0;i<w*h;i++){
		if(bufferedBits==0){
			buffer=shuffle64(bits, *pRaw++);
			bufferedBits=64;
		}
		
		pUnpacked[i]=buffer&MASK;
		buffer=buffer>>bits;
		bufferedBits-=bits;
	}
	
	assert(bufferedBits==0);
}

/*! Go back from one integer per pixel to packed format for output. */
void pack_blob(unsigned w, unsigned h, unsigned bits, const uint32_t *pUnpacked, uint64_t *pRaw)
{
	uint64_t buffer=0;
	unsigned bufferedBits=0;
	
	const uint64_t MASK=0xFFFFFFFFFFFFFFFFULL>>(64-bits);
	
	for(unsigned i=0;i<w*h;i++){
		buffer=buffer | (uint64_t(pUnpacked[i]&MASK)<< bufferedBits);
		bufferedBits+=bits;
		
		if(bufferedBits==64){
			*pRaw++ = shuffle64(bits, buffer);
			buffer=0;
			bufferedBits=0;
		}
	}
	
	assert(bufferedBits==0);
}

bool read_blob(int fd, uint64_t cbBlob, void *pBlob)
{
	uint8_t *pBytes=(uint8_t*)pBlob;
	
	uint64_t done=0;
	while(done<cbBlob){
		int todo=(int)std::min(uint64_t(1)<<30, cbBlob-done);		
		
		int got=read(fd, pBytes+done, todo);
		if(got==0 && done==0)
			return false;	// end of file
		if(got<=0)
			throw std::invalid_argument("Read failure.");
		done+=got;
	}
	
	return true;
}

void write_blob(int fd, uint64_t cbBlob, const void *pBlob)
{
	const uint8_t *pBytes=(const uint8_t*)pBlob;
	
	uint64_t done=0;
	while(done<cbBlob){
		int todo=(int)std::min(uint64_t(1)<<30, cbBlob-done);
		
		int got=write(fd, pBytes+done, todo);
		if(got<=0)
			throw std::invalid_argument("Write failure.");
		done+=got;
	}
}

///////////////////////////////////////////////////////////////////
// Basic image processing primitives

uint32_t vmin(uint32_t a, uint32_t b)
{ return std::min(a,b); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c)
{ 
    //if((a == 0) | (b == 0) | (c == 0)){
//	return (uint32_t) 0;
  //  }else{
	return std::min(a,std::min(b,c)); 
    //}
}

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{ 
   // if((a == 0) | (b == 0) | (c == 0) | (d == 0)){
//	return (uint32_t) 0;
  //  }else{
	return std::min(std::min(a,d),std::min(b,c)); 
    //}
}

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{ 
//    if((a == 0) | (b ==0) | (c == 0) | (d == 0) | (e == 0))
//	return (uint32_t) 0;
  //  else{
	return std::min(e, std::min(std::min(a,d),std::min(b,c))); 
    //} 
}

uint32_t kernel_min(std::vector<uint32_t> &square, int levels, unsigned x, unsigned y)
{
	int w = sqrt(square.size()); //This should always return an integer
	uint32_t minimum;
	auto tKernel = [&](int x, int y) -> uint32_t { return square[y*w+x]; };
	auto subSet = [&](int x1, int y1, int x2, int y2) -> std::vector<uint32_t> 
	{
		std::vector<uint32_t> sub_kernel;
		for(int i=x1; i<=x2; i++)
		{
		  for(int j=y1; j<=y2; j++)
		  {
			sub_kernel.push_back(tKernel(i,j));
		  }
		}
		return sub_kernel;
	}; 

	if(levels == 1) { 
	//Note that even if it includes pixels that aren't needed, pass a full 3x3 kernel
	switch(x)
		{ 
		 case 0:switch(y)
			{
			 case 0: minimum = vmin(tKernel(0,0), tKernel(0,1), tKernel(1,0)); 
				 break;
			 case 1: minimum = vmin(tKernel(0,1), tKernel(0,0), tKernel(0,2), tKernel(1,1)); 	
				 break;
			 case 2: minimum = vmin(tKernel(0,2), tKernel(0,1), tKernel(1,2)); 
				 break;
			}
			break;
		 case 1:switch(y)
			{
			 case 0: minimum = vmin(tKernel(1,0), tKernel(1,1), tKernel(0,0), tKernel(2,0)); 
				 break;
			 case 1: minimum = vmin(tKernel(1,0), tKernel(0,1), tKernel(1,1), tKernel(2,1), tKernel(1,2));
				 break;
			 case 2: minimum = vmin(tKernel(1,2), tKernel(1,1), tKernel(0,2), tKernel(2,2) );
				 break;
			}
			break;
		 case 2:switch(y)
			{
			 case 0: minimum = vmin(tKernel(1,0), tKernel(2,0), tKernel(2,1));
				 break;
			 case 1: minimum = vmin(tKernel(1,1), tKernel(2,1), tKernel(2,0), tKernel(2,2));
				 break;
			 case 2: minimum = vmin(tKernel(2,2), tKernel(1,2), tKernel(2,1));
				 break;
			}
			break;
			
		} //End switch(x)
	}else if(levels == 2) { 
		uint32_t min_minor;
		std::vector<uint32_t> subKernel, subKernel2;
		switch(x)
		{
		 case 0:switch(y) 
			{
			 case 0: subKernel = subSet(0,0, 2,2);
				 minimum = vmin(kernel_min(subKernel, 1, 0, 0),
						tKernel(0,2), tKernel(1,1), tKernel(2,0));				 
				 break;
			 case 1: subKernel = subSet(0, 0, 2, 2);
				 minimum = vmin(kernel_min(subKernel, 1, 0, 1), tKernel(1,0), tKernel(1,2), 
						tKernel(2,1), tKernel(0,3));
				 break;
			 case 2: subKernel = subSet(0,1, 2,3);
				 min_minor = vmin(kernel_min(subKernel, 1, 0, 1), tKernel(1,1), tKernel(1,3), tKernel(2,2));
				 minimum = vmin(min_minor, tKernel(0,0), tKernel(0,4));
				 break;
			 case 3: subKernel = subSet(0,2, 2,4);
				 minimum = vmin(kernel_min(subKernel, 1, 0,1),
						tKernel(0,1), tKernel(1,2), tKernel(1,4), tKernel(2,3)); 
 				 break;
			 case 4: subKernel = subSet(0,2, 2,4);
				 minimum = vmin(kernel_min(subKernel, 1, 0,2), tKernel(0,2), tKernel(1,3), tKernel(2,4));
				 break;
			}
			break;
		 case 1:
			switch(y)
			{
			 case 0: subKernel = subSet(0,0, 2,2);
				 minimum = vmin(kernel_min(subKernel, 1, 1,0), tKernel(3,0), tKernel(0,1), 
						tKernel(2,1), tKernel(1,2));
				 break;
			 case 1: subKernel = subSet(0,0, 2,2);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,1), tKernel(0,0), tKernel(2,0),
						  tKernel(0,2), tKernel(2,2));
				 minimum = vmin(min_minor, tKernel(3,1), tKernel(1,3));
				 break;
			 case 2: subKernel = subSet(0,1, 2,3);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,1), tKernel(0,1), tKernel(2,1),
						  tKernel(0,3), tKernel(2,3));
				 minimum = vmin(min_minor, tKernel(3,2), tKernel(1, 4));
				 break;
			 case 3: subKernel = subSet(0,2, 2,4);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,1), tKernel(0,2), tKernel(2,2),
						  tKernel(0,4), tKernel(2,4));
				 minimum = vmin(min_minor, tKernel(1,1), tKernel(3,2));
				 break;
			 case 4: subKernel = subSet(0,2, 2,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,2), tKernel(0,3), tKernel(1,2), 
						tKernel(2,3), tKernel(3,4));
				 break;
			}
			break;
		 case 2:
			switch(y)
			{
			 case 0: subKernel = subSet(1,0, 3,2);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,0), tKernel(1,1), tKernel(3,1));
				 minimum = vmin(min_minor, tKernel(0,0), tKernel(4,0), tKernel(2,2));
				 break;
			 case 1: subKernel = subSet(1,0, 3,2);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,1), tKernel(1,0), tKernel(3,0),
						  tKernel(1,2), tKernel(3,2));
				 minimum = vmin(min_minor, tKernel(0,1), tKernel(4,1), tKernel(2,3));
				 break;
			 case 2: subKernel = subSet(0,1, 2,3);
				 subKernel2 = subSet(2,1, 4,3);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,1), kernel_min(subKernel, 1, 1,1));
				 minimum = vmin(min_minor, tKernel(2,0), tKernel(2,1), tKernel(2,3), tKernel(2,4));
				 break;
			 case 3: subKernel = subSet(0,2, 2,4);
				 subKernel2 = subSet(2,2, 4,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,1), kernel_min(subKernel2, 1, 1,1), 
						tKernel(2,1), tKernel(2,2), tKernel(2,4));
				 break;
			 case 4: subKernel = subSet(0,2, 2,4);
				 subKernel2 = subSet(2,2, 2,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,2), kernel_min(subKernel, 1, 1,2), 
						tKernel(2,2), tKernel(2,3));
				 break;

			}
			break;
		 case 3:
			switch(y)
			{
			 case 0: subKernel = subSet(2,0, 4,2);
				 minimum = vmin(kernel_min(subKernel, 1, 1, 0), tKernel(1,0), tKernel(2,1), 
						tKernel(4,1), tKernel(3,2));
				 break;
			 case 1: subKernel = subSet(1,0, 3,2);
				 subKernel2 = subSet(2,0, 4,2);
				 minimum = vmin(kernel_min(subKernel, 1, 1,1), kernel_min(subKernel2, 1, 2,1),
						tKernel(3,0), tKernel(3,2), tKernel(3,3)); 
				 break;
			 case 2: subKernel = subSet(1,1, 3,3);
				 subKernel2 = subSet(2,1, 4,3);
				 min_minor = vmin(kernel_min(subKernel, 1, 1,1), kernel_min(subKernel, 1, 2,1)); 
				 minimum = vmin(min_minor, tKernel(3,0), tKernel(3,1), tKernel(3,3), tKernel(3,4));
				 break;
			 case 3: subKernel = subSet(1,2, 3,4);
				 subKernel2 = subSet(2,2, 4,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,1), kernel_min(subKernel2, 1, 2,1), 
				 		  tKernel(3,1), tKernel(3,2), tKernel(3,4));
				 break;
			 case 4: subKernel = subSet(1,2, 3,4); 
				 subKernel2 = subSet(2,2, 4,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,2), kernel_min(subKernel2, 1, 2,2),
						tKernel(3,2), tKernel(3,3));
				 break; 
			}
			break;
		 case 4: //Review here
			switch(y)
			{
			 case 0: subKernel = subSet(2,0, 4,2);
				 minimum = vmin(kernel_min(subKernel, 1, 1,0), tKernel(4,1), tKernel(4,2));
				 break;
			 case 1: subKernel = subSet(2,0, 4,2);
				 minimum = vmin(kernel_min(subKernel, 1, 1,1), tKernel(4,0), tKernel(4,2), tKernel(4,3));
				 break;
			 case 2: subKernel = subSet(2,1, 4,3);
				 minimum = vmin(kernel_min(subKernel, 1, 1,1), tKernel(4,0), tKernel(4,1),
						tKernel(4,3), tKernel(4,4));
				 break;
			 case 3: subKernel = subSet(2,2, 4,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,1), tKernel(4,1), tKernel(4,2), tKernel(4,4));
				 break;
			 case 4: subKernel = subSet(2,2, 4,4);
				 minimum = vmin(kernel_min(subKernel, 1, 1,2), tKernel(4,2), tKernel(4,3));
				 break;
			}
			break;
		} // End switch(x)
	}else{
		std::cerr<<"Cannot deal with more than 2 levels";
	}

	return minimum;
}

//erode function with parallel for outer loop vmin OpenCL Kernels
void erode(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels, unsigned no_frames)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };

	auto subSet = [&](int x1, int y1, int x2, int y2) -> std::vector<uint32_t> 
	{
		std::vector<uint32_t> sub_kernel;
		for(int i=x1; i<=x2; i++)
		{
		  for(int j=y1; j<=y2; j++)
		  {
			sub_kernel.push_back(in(i,j));
		  }
		}
		return sub_kernel;
	};
/*
	auto img_k = [&](int x, int y) -> std::vector<uint32_t> 
	{
		std::vector<uint32_t> img_kernel;
		if(x-levels<0)
			img_kernel = subSet(0,y, 2*levels, (y+2*levels));
		else if(x+levels>(w-1)) 
			img_kernel = subSet((w-1-2*levels), y, w-1, (y+2*levels));
		else						
			img_kernel = subSet(x-levels,y, x+levels,(y+2*levels));
		return img_kernel;

	} ;*/
std::vector<uint32_t> img_kernel;
	for(unsigned y=0; y<h; y++)
	{
		if(y<levels){
			if (y==0){ //Only look at first row if first block
				std::cerr<<"Checking y==0 \n";
				if(count==0){
				   for(int x=0; x<w; x++){
					std::cerr<<"Entering first x loop\n";
					 if(x<levels){
						std::cerr<<"Entering first if clause\n";
						img_kernel = subSet(0,y, 2*levels, (y+2*levels));
						out(x,y) = kernel_min(img_kernel, levels, x, y);
					 }else	if((x+levels)>(w-1)){
						std::cerr<<"Entering second if clause\n";
						img_kernel = subSet((w-1-2*levels), y, w-1, (y+2*levels));
						out(x,y) = kernel_min(img_kernel, levels, (2*levels)-((w-1)-x), y);
					 }else{
						std::cerr<<"Entering third if clause\n";
						img_kernel = subSet(x-levels,y, x+levels,(y+2*levels));
						out(x,y) = kernel_min(img_kernel, levels, levels/*x-coord*/ ,y);
					}
					 
				  }
				} // else do nothing
			}else{
			  for(int x=0; x<w; x++)
			  {
			    if(x<levels){
					img_kernel = subSet(0,0, 2*levels, (2*levels));
					out(x,y) = kernel_min(img_kernel, levels, x, levels);
			    }else if(x+levels>(w-1)) {
					img_kernel = subSet((w-1-2*levels), 0, w-1, (2*levels));
					out(x,y) = kernel_min(img_kernel, levels, (2*levels)-((w-1)-x), y);
			    }else{						
					img_kernel = subSet(x-levels,0, x+levels,(0+2*levels));
					out(x,y) = kernel_min(img_kernel, levels, levels ,y);
				} 
			   }
			 }
		}else if(y<h){
			if(y<h-levels){
			   for(int x=0; x<w; x++)
			   {
			    if(x<levels){
					img_kernel = subSet(0,(y-levels), 2*levels, (y+levels));
					out(x,y) = kernel_min(img_kernel, levels, x ,levels);
			    }else if(x+levels>(w-1)){ 
					img_kernel = subSet((w-1-2*levels), (y-levels), w-1, (y+levels));
					out(x,y) = kernel_min(img_kernel, levels, (2*levels)-((w-1)-x) ,levels);
			    }else{						
					img_kernel = subSet(x-levels, (y-levels), x+levels,(y+levels));
					out(x,y) = kernel_min(img_kernel, levels, levels ,levels);
				}
			    
			   }
			}else{
			   if(y==h-1){
			      if(count == no_frames){
					for(int x=0; x<w; x++)
						{
						 if(x>levels)
						img_kernel = subSet(0,(h-1-2*levels), 2*levels, (h-1));
						 else if(x+levels>(w-1)) 
						img_kernel = subSet((w-1-2*levels), (h-1-2*levels), w-1, (h-1));
						 else						
						img_kernel = subSet(x-levels, (h-1-2*levels), x+levels,(h-1));

						 out(x,y) = kernel_min(img_kernel, levels, x%(2*levels+1) ,y%(2*levels+1));
						}
			      } //else do nothing
			   }else{
				for(int x=0; x<w; x++)
			      	{
			      	 if(x<levels)
			 		img_kernel = subSet(0,(h-1-2*levels), 2*levels, (h-1));
			      	 else if(x+levels>(w-1)) 
					img_kernel = subSet((w-1-2*levels), (h-1-2*levels), w-1, (h-1));
			      	 else						
					img_kernel = subSet(x-levels, (h-1-2*levels), x+levels,(h-1));

			      	 out(x,y) = kernel_min(img_kernel, levels, x%(2*levels+1) ,y%(2*levels+1));
			      	}

			   }
			}
		}

	} //End for
}

uint32_t vmax(uint32_t a, uint32_t b)
{ return std::max(a,b); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c)
{ return std::max(a,std::max(b,c)); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{ return std::max(std::max(a,d),std::max(b,c)); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{ return std::max(e, std::max(std::max(a,d),std::max(b,c))); }

void dilate(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels, unsigned no_frames)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & { return output[y*w+x]; };
	unsigned end = h + 2*levels;

	for(unsigned y=0; y<end; y++)
	{
		if(y==0){ //First line should only be _processed_ for the first block
			if(count == 0)
			{
				out(0,0) = vmax(in(0,0), in(0,1), in(1,0));
				for(unsigned x=1;x<w-1;x++)
				{
					out(x,0)=vmax(in(x,0), in(x-1,0), in(x+1,0), in(x,1));
				}
				out(w-1,0)=vmax(in(w-1,0), in(w-2,0), in(w-1,1));
			}//else do nothing
		} else if(y==end-1 && count==no_frames){ //Last line should only be _processed_ for final block - otherwise only read.
			out(0, y) = vmax(in(0,y), in(0, y-1), in(1, y));
			for(unsigned x=1;x<w-1; x++)
			{
				out(x, y) = vmax(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1));
			}
			out(w-1, y) = vmax(in(w-1,y), in(w-2, y), in(w-1, y-1));
		} else if(y < (end-levels)){
			out(0,y)=vmax(in(0, y-1), in(0, y+1), in(0,y), in(1,y)); //Left hand side edge
			for(unsigned x=1; x<w-1; x++)
			{
				out(x, y) = vmax(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1), in(x,y+1));
			}
			out(w-1, y) = vmax(in(w-1, y-1), in(w-1, y+1), in(w-1,y), in(w-2,y)); //Right hand side edge
		}			
	}//end for
}

///////////////////////////////////////////////////////////////////
// Composite image processing

void process(int levels, unsigned w, unsigned h, unsigned no_frames, std::vector<uint32_t> &pixels, uint32_t count)
{
	std::vector<uint32_t> buffer(pixels.size());
	
	// Depending on whether levels is positive or negative,
	// we flip the order round.
	auto fwd=levels < 0 ? erode : dilate;
	auto rev=levels < 0 ? dilate : erode;
	unsigned abslevels = std::abs(levels);
	//unsigned loop_bound = floor(abslevels/2);
	//for(int i=0;i<loop_bound;i++){
	for(int i=0;i<abslevels;i++){
		fwd(w, h, pixels, buffer, count, 1, no_frames);
		std::swap(pixels, buffer);
	}
	//if(abslevels%2 == 1) { fwd(w,h, pixels, buffer, count, 1, no_frames); std::swap(pixels, buffer); }
	//for(int i=0;i<loop_bound;i++){
	for(int i=0;i<abslevels;i++){
		rev(w,h,pixels, buffer, count, 1, no_frames);
		std::swap(pixels,buffer);
	}
/*	
	for(int i=0;i<abslevels;i++){
		fwd(w, h, pixels, buffer, count, abslevels, no_frames);
		std::swap(pixels, buffer);
	}
	for(int i=0;i<std::abs(levels);i++){
		rev(w,h,pixels, buffer, count, abslevels, no_frames);
		std::swap(pixels, buffer);
	}*/
	//if(abslevels%2 == 1) {rev(w,h,pixels,buffer,count,1, no_frames); std::swap(pixels,buffer);}
}

// You may want to play with this to check you understand what is going on
void invert(int levels, unsigned w, unsigned h, unsigned bits, std::vector<uint32_t> &pixels)
{
	uint32_t mask=0xFFFFFFFFul>>bits;
	
	for(unsigned i=0;i<w*h;i++){
		pixels[i]=mask-pixels[i];
	}
}


