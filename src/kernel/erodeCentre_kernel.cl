__kernel void erodeCentre(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output)
{
	out(x,0)=vmax(in(x,0), in(x-1,0), in(x,1), in(x+1,0));
	for(unsigned y=1;y<h-1;y++){
		out(x,y)=vmax(in(x,y), in(x-1,y), in(x,y-1), in(x,y+1), in(x+1,y));
	}
	out(x,h-1)=vmax(in(x,h-1), in(x-1,h-1), in(x,h-2), in(x+1,h-1));
}
