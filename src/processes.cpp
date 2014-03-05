#include "processes.hpp"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
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
{ return std::min(a,std::min(b,c)); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{ return std::min(std::min(a,d),std::min(b,c)); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{ return std::min(e, std::min(std::min(a,d),std::min(b,c))); }

//erode function with parallel for outer loop vmin OpenCL Kernels
void erode(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };
	unsigned end = h + 2*levels;

	for(unsigned y=0; y<end; y++)
	{
		std::cerr<<"Erode for "<<y<<"\n";
		if(y==0){ //First line should only be _processed_ for the first block
			if(count == 0){
				out(0,0) = vmin(in(0,0), in(0,1), in(1,0));
				for(unsigned x=1;x<w-1;x++)
				{
					out(x,0)=vmin(in(x,0), in(x-1,0), in(x+1,0), in(x,1));
				}
				out(w-1,0)=vmin(in(w-1,0), in(w-2,0), in(w-1,1));
			} //else do nothing
		} else if(y==end-1 && count==LAST){ //Last line should only be _processed_ for final block - otherwise only read.
			out(0, y) = vmin(in(0,y), in(0, y-1), in(1, y));
			for(unsigned x=1;x<w-1; x++)
			{
				out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1));
			}
			out(w-1, y) = vmin(in(w-1,y), in(w-2, y), in(w-1, y-1));
		} else if (y <= (end-levels)){
			
			out(0,y)=vmin(in(0, y-1), in(0, y+1), in(0,y), in(1,y)); //Left hand side edge
			for(unsigned x=1; x<w-1; x++)
			{
				out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1), in(x,y+1));
			}
			out(w-1, y) = vmin(in(w-1, y-1), in(w-1, y+1), in(w-1,y), in(w-2,y)); //Right hand side edge
		}			
	}
}

uint32_t vmax(uint32_t a, uint32_t b)
{ return std::max(a,b); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c)
{ return std::max(a,std::max(b,c)); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{ return std::max(std::max(a,d),std::max(b,c)); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{ return std::max(e, std::max(std::max(a,d),std::max(b,c))); }

void dilate(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };
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
		} else if(y==end-1 && count==LAST){ //Last line should only be _processed_ for final block - otherwise only read.
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
	}//end
}

///////////////////////////////////////////////////////////////////
// Composite image processing

void process(int levels, unsigned w, unsigned h, unsigned /*bits*/, std::vector<uint32_t> &pixels, uint32_t count)
{
	std::vector<uint32_t> buffer(pixels.size());
	
	// Depending on whether levels is positive or negative,
	// we flip the order round.
	auto fwd=levels < 0 ? erode : dilate;
	auto rev=levels < 0 ? dilate : erode;
	std::cerr<<"Enter Process\n";
	unsigned abslevels = std::abs(levels);
	
	for(int i=0;i<abslevels;i++){
		std::cerr<<"Enter for\n";
		fwd(w, h, pixels, buffer, count, abslevels);
		std::cerr<<"Forward\n";
		std::swap(pixels, buffer);
	}
	for(int i=0;i<std::abs(levels);i++){
		rev(w,h,pixels, buffer, count, abslevels);
		std::cerr<<"Create cbRaw\n";
		std::swap(pixels, buffer);
	}
}

// You may want to play with this to check you understand what is going on
void invert(int levels, unsigned w, unsigned h, unsigned bits, std::vector<uint32_t> &pixels)
{
	uint32_t mask=0xFFFFFFFFul>>bits;
	
	for(unsigned i=0;i<w*h;i++){
		pixels[i]=mask-pixels[i];
	}
}



