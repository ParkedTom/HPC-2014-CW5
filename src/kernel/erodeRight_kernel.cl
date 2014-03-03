__kernel void erodeRight(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output)
{
	out(w-1,0)=vmax(in(w-1,0), in(w-1,1), in(w-2,0));
	for(unsigned y=1;y<h-1;y++){
		out(w-1,y)=vmax(in(w-1,y), in(w-1,y-1), in(w-2,y), in(w-1,y+1));
	}
	out(w-1,h-1)=vmax(in(w-1,h-1), in(w-1,h-2), in(w-2,h-1));
}
