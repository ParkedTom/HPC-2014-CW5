__kernel void erodeLeft(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output)
{
	out(0,0)=vmin(in(0,0), in(0,1), in(1,0));
	for(unsigned y=1;y<h-1;y++){
		out(0,y)=vmin(in(0,y), in(0,y-1), in(1,y), in(0,y+1));
	}
	out(0,h-1)=vmin(in(0,h-1), in(0,h-2), in(1,h-1));
}
