#include "processes.hpp"
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
#include "tbb/parallel_for.h"

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
                return false;   // end of file
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

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)
{ return std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g)
{ return std::min(g,std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c)))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h)
{ return std::min(std::min(g,h),std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c)))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i)
{ return std::min(i,std::min(std::min(g,h),std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c))))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j)
{ return std::min(std::min(i,j),std::min(std::min(g,h),std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c))))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j, uint32_t k)
{ return std::min(k,std::min(std::min(i,j),std::min(std::min(g,h),std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c)))))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j, uint32_t k, uint32_t l)
{ return std::min(std::min(l,k),std::min(std::min(i,j),std::min(std::min(g,h),std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c)))))); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j, uint32_t k, uint32_t l, uint32_t m)
{ return std::min(m,std::min(std::min(l,k),std::min(std::min(i,j),std::min(std::min(g,h),std::min(std::min(e,f), std::min(std::min(a,d),std::min(b,c))))))); }

//erode function with parallel for outer loop vmin OpenCL Kernels
void erode(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned level, unsigned no_frames)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };;
	if(level == 0)
	{
		for(unsigned y=0; y<h; y++)
		{
			if(y==0){ //First line should only be _processed_ for the first block
				if(count == 0){
					out(0,0) = vmin(in(0,0), in(0,1), in(1,0));
					auto loop_body = [=](size_t x){
						out(x,0)=vmin(in(x,0), in(x-1,0), in(x+1,0), in(x,1));
                    };
                    tbb::parallel_for(1u, w-1, loop_body);
					/*for(unsigned x=1;x<w-1;x++)
					{
						out(x,0)=vmin(in(x,0), in(x-1,0), in(x+1,0), in(x,1));
					}*/
					out(w-1,0)=vmin(in(w-1,0), in(w-2,0), in(w-1,1));
				} //else do nothing
			} else if((y == (h-1)) && (count==(no_frames-1))){ //Last lines should only be _processed_ for final block - otherwise only read.
				out(0, y) = vmin(in(0,y), in(0, y-1), in(1, y));
				auto loop_body = [=](size_t x){
					out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1));
                };
                tbb::parallel_for(1u, w-1, loop_body);
				/*for(unsigned x=1;x<w-1; x++)
				{
					out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1));
				}*/
				out(w-1, y) = vmin(in(w-1,y), in(w-2, y), in(w-1, y-1));
			} else if (y < (h-1)){
				
				out(0,y)=vmin(in(0, y-1), in(0, y+1), in(0,y), in(1,y)); //Left hand side edge
				auto loop_body = [=](size_t x){
					out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1), in(x,y+1));
                };
                tbb::parallel_for(1u, w-1, loop_body);
				/*for(unsigned x=1; x<w-1; x++)
				{
					out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1), in(x,y+1));
				}*/
				out(w-1, y) = vmin(in(w-1, y-1), in(w-1, y+1), in(w-1,y), in(w-2,y)); //Right hand side edge
			}			
		}
	}else{
		for(unsigned y=0; y<h; y++)
		{
			if(y==0){ //First line should only be _processed_ for the first block
				if(count == 0){
					out(0,0) = vmin(in(0,0), in(0,1), in(1,0), in(1,1), in(0,2), in(2,0));
					out(1,0) = vmin(in(0,0), in(1,0), in(2,0), in(3,0), in(0,1), in(1,1), in(2,1), in(1,2));
					for(unsigned x=2;x<w-2;x++)
					{
						out(x,0) = vmin(in(x,0), in(x-2,0), in(x-1,0), in(x+1,0), in(x+2,0), in(x-1,1), in(x,1), in(x+1,1), in(x,2));
					}
					out(w-2,0) = vmin(in(w-1,0), in(w-2,0), in(w-3,0), in(w-4,0), in(w-1,1), in(w-2,1), in(w-3,1), in(w-2,2));
					out(w-1,0)=vmin(in(w-1,0), in(w-2,0), in(w-3,0), in(w-1,1), in(w-2,1), in(w-1,2));
				} //else do nothing
			}else if(y==1){
//				std::cerr<<"Frame: "<<count<<"y = "<<y<<"\n";
				out(0,1) = vmin(in(0,0), in(1,0), in(0,1), in(0,2), in(1,1), in(1,2), in(0,3), in(2,1));
				out(1,1) = vmin(in(0,0), in(1,0), in(2,0), in(1,0), in(1,1), in(2,1), in(3,1), in(0,2), in(1,2), in(2,2), in(1,3));
				for(unsigned x=2;x<w-2;x++)
				{
					out(x,1) = vmin(in(x-1,0), in(x,0), in(x+1,0), in(x,1), in(x-2,1), in(x-1,1), in(x+1,1), in(x+2,1), in(x-1,2), in(x,2), in(x+1,2), in(x,3));
				}
				out(w-2,0) = vmin(in(w-1,0), in(w-2,0), in(w-3,1), in(w-2,1), in(w-1,1), in(w-1,2), in(w-2,2), in(w-1,3));
				out(w-1,0) = vmin(in(w-1,0), in(w-2,0), in(w-3,0), in(w-1,1), in(w-2,1), in(w-1,2));
			}else if((y >= (h-2)) && (count==(no_frames-1))){ //Last lines should only be _processed_ for final block - otherwise only read.
//				std::cerr<<"Frame: "<<count<<"y = "<<y<<"\n";
				out(0,h-2) = vmin(in(0,h-1), in(1,h-1), in(0,h-2), in(1,h-2), in(2,h-2), in(0,h-3), in(1,h-3), in(0,h-4));
				out(0,h-1) = vmin(in(0,h-1), in(1,h-1), in(2,h-1), in(0,h-2), in(1,h-2), in(0,h-3));
				out(1,h-2) = vmin(in(0,h-1), in(0,h-2), in(0,h-3), in(1,h-1), in(2,h-1), in(1,h-2), in(2,h-2), in(3,h-2), in(1,h-3), in(2,h-3), in(1,h-4));
				out(1,h-1) = vmin(in(0,h-1), in(0,h-2), in(1,h-1), in(2,h-1), in(3,h-1), in(1,h-2), in(2,h-2), in(1,h-3));
				for(unsigned x=2;x<w-2; x++)
				{
					out(x, h-2) = vmin(in(x,h-2), in(x+1, h-2), in(x+2,h-2), in(x,h-3), in(x+1,h-3), in(x, h-4), in(x-1,h-2), in(x-2, h-2), in(x-1, h-3), in(x-1,h-1), in(x,h-1), in(x+1,h-1));
					out(x, h-1) = vmin(in(x,h-1), in(x+1, h-1), in(x+2,h-1), in(x,h-2), in(x+1,h-2), in(x, h-3), in(x-1,h-1), in(x-2, h-1), in(x-1, h-2));
				}
				out(w-2,h-2) = vmin(in(w-1,h-1), in(w-2,h-1), in(w-3, h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-4,h-2), in(w-1,h-3), in(w-2,h-3), in(w-3,h-3), in(w-2,h-4));
				out(w-1,h-2) = vmin(in(w-1,h-1), in(w-2,h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-1,h-3), in(w-2,h-3), in(w-1,h-4));
				out(w-2,h-1) = vmin(in(w-1,h-1), in(w-2,h-1), in(w-3,h-1), in(w-4,h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-2,h-3));
				out(w-1,h-1) = vmin(in(w-1,h-1), in(w-2,h-1), in(w-3,h-1), in(w-1,h-2), in(w-2,h-2), in(w-1,h-3));
				break;
			}else	if (y < (h-2)){
				out(0,y)=vmin(in(0,y-2), in(0,y-1), in(0,y), in(0,y+1), in(0,y+2), in(1,y-1), in(1,y), in(1,y+1), in(2,y));
				out(1,y)=vmin(in(0, y-1), in(0, y+1), in(0,y), in(1,y-2), in(1,y-1), in(1,y), in(1,y+1), in(1,y+2), in(2,y-1), in(2,y), in(2,y+1), in(3,y)); //Left hand side edge
				for(unsigned x=2; x<w-2; x++)
				{
					out(x, y) = vmin(in(x,y), in(x-1,y), in(x+1,y), in(x-2,y), in(x+2,y), in(x,y-1), in(x,y+1), in(x,y-2), in(x,y+2), in(x-1,y-1), in(x-1,y+1), in(x+1,y-1), in(x+1,y+1));
				}
				out(w-2, y) = vmin(in(w-1, y-1), in(w-1, y+1), in(w-1,y), in(w-2,y), in(w-2,y-1), in(w-2,y-2), in(w-2,y+1), in(w-2,y+2), in(w-3, y-1), in(w-3, y+1), in(w-3,y), in(w-4,y)); //Right hand side edge
				out(w-1, y) = vmin(in(w-1,y), in(w-1,y-1), in(w-1,y-2), in(w-1,y+1), in(w-1,y+2), in(w-2, y-1), in(w-2, y+1), in(w-2,y), in(w-3,y));
			}			
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

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)
{ return std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g)
{ return std::max(g,std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c)))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h)
{ return std::max(std::max(g,h),std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c)))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i)
{ return std::max(i,std::max(std::max(g,h),std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c))))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j)
{ return std::max(std::max(i,j),std::max(std::max(g,h),std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c))))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j, uint32_t k)
{ return std::max(k,std::max(std::max(i,j),std::max(std::max(g,h),std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c)))))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j, uint32_t k, uint32_t l)
{ return std::max(std::max(l,k),std::max(std::max(i,j),std::max(std::max(g,h),std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c)))))); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h, uint32_t i, uint32_t j, uint32_t k, uint32_t l, uint32_t m)
{ return std::max(m,std::max(std::max(l,k),std::max(std::max(i,j),std::max(std::max(g,h),std::max(std::max(e,f), std::max(std::max(a,d),std::max(b,c))))))); }

void dilate(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels, unsigned no_frames)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };
	
	if(levels == 0)
	{
		for(unsigned y=0; y<h; y++)
		{
			if(y==0){ //First line should only be _processed_ for the first block
				if(count == 0){
					out(0,0) = vmax(in(0,0), in(0,1), in(1,0));
					auto loop_body = [=](size_t x){
						out(x,0)=vmax(in(x,0), in(x-1,0), in(x+1,0), in(x,1));
                    };
                    tbb::parallel_for(1u, w-1, loop_body);
                    
					out(w-1,0)=vmax(in(w-1,0), in(w-2,0), in(w-1,1));
				} //else do nothing
			} else if((y >= (h-1)) && (count==(no_frames-1))){ //Last lines should only be _processed_ for final block - otherwise only read.
				out(0, y) = vmax(in(0,y), in(0, y-1), in(1, y));
				auto loop_body = [=](size_t x){
					out(x, y) = vmax(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1));
                };
                tbb::parallel_for(1u, w-1, loop_body);
				/*for(unsigned x=1;x<w-1; x++)
				{
				}*/
				out(w-1, y) = vmax(in(w-1,y), in(w-2, y), in(w-1, y-1));
			} else if (y < (h-1)){
				
				out(0,y)=vmax(in(0, y-1), in(0, y+1), in(0,y), in(1,y)); //Left hand side edge
				auto loop_body = [=](size_t x){
					out(x, y) = vmax(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1), in(x,y+1));
                };
                tbb::parallel_for(1u, w-1, loop_body);
				/*for(unsigned x=1; x<w-1; x++)
				{
					out(x, y) = vmax(in(x,y), in(x-1,y), in(x+1,y), in(x,y-1), in(x,y+1));
				}*/
				out(w-1, y) = vmax(in(w-1, y-1), in(w-1, y+1), in(w-1,y), in(w-2,y)); //Right hand side edge
			}			
		}
	}else{ //levels == 1 ie process 2 levels
		for(unsigned y=0; y<h; y++)
		{
			if(y==0){ //First line should only be _processed_ for the first block
				if(count == 0){
					out(0,0) = vmax(in(0,0), in(0,1), in(1,0), in(1,1), in(0,2), in(2,0));
					out(1,0) = vmax(in(0,0), in(1,0), in(2,0), in(3,0), in(0,1), in(1,1), in(2,1), in(1,2));
					for(unsigned x=2;x<w-2;x++)
					{
						out(x,0) = vmax(in(x,0), in(x-2,0), in(x-1,0), in(x+1,0), in(x+2,0), in(x-1,1), in(x,1), in(x+1,1), in(x,2));
					}
					out(w-2,0) = vmax(in(w-1,0), in(w-2,0), in(w-3,0), in(w-4,0), in(w-1,1), in(w-2,1), in(w-3,1), in(w-2,2));
					out(w-1,0)=vmax(in(w-1,0), in(w-2,0), in(w-3,0), in(w-1,1), in(w-2,1), in(w-1,2));
				} //else do nothing
			}else if(y==1){
				out(0,1) = vmax(in(0,0), in(1,0), in(0,1), in(0,2), in(1,1), in(1,2), in(0,3), in(2,1));
				out(1,1) = vmax(in(0,0), in(1,0), in(2,0), in(1,0), in(1,1), in(2,1), in(3,1), in(0,2), in(1,2), in(2,2), in(1,3));
				for(unsigned x=2;x<w-2;x++)
				{
					out(x,1) = vmax(in(x-1,0), in(x,0), in(x+1,0), in(x,1), in(x-2,1), in(x-1,1), in(x+1,1), in(x+2,1), in(x-1,2), in(x,2), in(x+1,2), in(x,3));
				}
				out(w-2,0) = vmax(in(w-1,0), in(w-2,0), in(w-3,1), in(w-2,1), in(w-1,1), in(w-1,2), in(w-2,2), in(w-1,3));
				out(w-1,0) = vmax(in(w-1,0), in(w-2,0), in(w-3,0), in(w-1,1), in(w-2,1), in(w-1,2));
			}else if((y >= (h-2)) && (count==(no_frames-1))){ //Last lines should only be _processed_ for final block - otherwise only read.
				out(0,h-2) = vmax(in(0,h-1), in(1,h-1), in(0,h-2), in(1,h-2), in(2,h-2), in(0,h-3), in(1,h-3), in(0,h-4));
				out(0,h-1) = vmax(in(0,h-1), in(1,h-1), in(2,h-1), in(0,h-2), in(1,h-2), in(0,h-3));
				out(1,h-2) = vmax(in(0,h-1), in(0,h-2), in(0,h-3), in(1,h-1), in(2,h-1), in(1,h-2), in(2,h-2), in(3,h-2), in(1,h-3), in(2,h-3), in(1,h-4));
				out(1,h-1) = vmax(in(0,h-1), in(0,h-2), in(1,h-1), in(2,h-1), in(3,h-1), in(1,h-2), in(2,h-2), in(1,h-3));
				for(unsigned x=2;x<w-2; x++)
				{
					out(x, h-2) = vmax(in(x,h-2), in(x+1, h-2), in(x+2,h-2), in(x,h-3), in(x+1,h-3), in(x, h-4), in(x-1,h-2), in(x-2, h-2), in(x-1, h-3), in(x-1,h-1), in(x,h-1), in(x+1,h-1));
					out(x, h-1) = vmax(in(x,h-1), in(x+1, h-1), in(x+2,h-1), in(x,h-2), in(x+1,h-2), in(x, h-3), in(x-1,h-1), in(x-2, h-1), in(x-1, h-2));
				}
				out(w-2,h-2) = vmax(in(w-1,h-1), in(w-2,h-1), in(w-3, h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-4,h-2), in(w-1,h-3), in(w-2,h-3), in(w-3,h-3), in(w-2,h-4));
				out(w-1,h-2) = vmax(in(w-1,h-1), in(w-2,h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-1,h-3), in(w-2,h-3), in(w-1,h-4));
				out(w-2,h-1) = vmax(in(w-1,h-1), in(w-2,h-1), in(w-3,h-1), in(w-4,h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-2,h-3));
				out(w-1,h-1) = vmax(in(w-1,h-1), in(w-2,h-1), in(w-3,h-1), in(w-1,h-2), in(w-2,h-2), in(w-1,h-3));
				break;
			}/*else if((y==h-2) && (count!=(no_frames-1))){
				out(0,h-2) = vmax(in(0,h-1), in(1,h-1), in(0,h-2), in(1,h-2), in(2,h-2), in(0,h-3), in(1,h-3), in(0,h-4));
				out(1,h-2) = vmax(in(0,h-1), in(0,h-2), in(0,h-3), in(1,h-1), in(2,h-1), in(1,h-2), in(2,h-2), in(3,h-2), in(1,h-3), in(2,h-3), in(1,h-4));
				for(unsigned x=2;x<w-2; x++)
				{
					out(x, h-2) = vmax(in(x,h-2), in(x+1, h-2), in(x+2,h-2), in(x,h-3), in(x+1,h-3), in(x, h-4), in(x-1,h-2), in(x-2, h-2), in(x-1, h-3), in(x-1,h-1), in(x,h-1), in(x+1,h-1));
				}				
				out(w-2,h-2) = vmax(in(w-1,h-1), in(w-2,h-1), in(w-3, h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-4,h-2), in(w-1,h-3), in(w-2,h-3), in(w-3,h-3), in(w-2,h-4));
				out(w-1,h-2) = vmax(in(w-1,h-1), in(w-2,h-1), in(w-1,h-2), in(w-2,h-2), in(w-3,h-2), in(w-1,h-3), in(w-2,h-3), in(w-1,h-4));
			}*/else	if (y < (h-2)){
				out(0,y)=vmax(in(0,y-2), in(0,y-1), in(0,y), in(0,y+1), in(0,y+2), in(1,y-1), in(1,y), in(1,y+1), in(2,y));
				out(1,y)=vmax(in(0, y-1), in(0, y+1), in(0,y), in(1,y-2), in(1,y-1), in(1,y), in(1,y+1), in(1,y+2), in(2,y-1), in(2,y), in(2,y+1), in(3,y)); //Left hand side edge
				for(unsigned x=2; x<w-2; x++)
				{
					out(x, y) = vmax(in(x,y), in(x-1,y), in(x+1,y), in(x-2,y), in(x+2,y), in(x,y-1), in(x,y+1), in(x,y-2), in(x,y+2), in(x-1,y-1), in(x-1,y+1), in(x+1,y-1), in(x+1,y+1));
				}
				out(w-2, y) = vmax(in(w-1, y-1), in(w-1, y+1), in(w-1,y), in(w-2,y), in(w-2,y-1), in(w-2,y-2), in(w-2,y+1), in(w-2,y+2), in(w-3, y-1), in(w-3, y+1), in(w-3,y), in(w-4,y)); //Right hand side edge
				out(w-1, y) = vmax(in(w-1,y), in(w-1,y-1), in(w-1,y-2), in(w-1,y+1), in(w-1,y+2), in(w-2, y-1), in(w-2, y+1), in(w-2,y), in(w-3,y));
			}			
		}
	}
}

///////////////////////////////////////////////////////////////////
// Composite image processing

void process(int levels, unsigned w, unsigned h, unsigned no_frames, std::vector<uint32_t> &pixels, uint32_t count, unsigned divide)
{
	std::vector<uint32_t> buffer(pixels.size());
	
	// Depending on whether levels is positive or negative,
	// we flip the order round.
	auto fwd=levels < 0 ? erode : dilate;
	auto rev=levels < 0 ? dilate : erode;
	unsigned abslevels = std::abs(levels);
	
	
	for(int i=0;i<abslevels;i++){
		fwd(w, h, pixels, buffer, count, 0, no_frames);
		std::swap(pixels, buffer);
	}
	for(int i=0;i<abslevels;i++){
		rev(w, h, pixels, buffer, count, 0, no_frames);
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
