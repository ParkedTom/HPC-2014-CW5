CPPFLAGS = -std=c++11 -stdlib=libstdc++ -framework OpenCL -I include

original : src/original/original.cpp
	clang++ $(CPPFLAGS) $^ -o $@ 
	
test : original
	convert lenna.png -depth 2 gray:- | ./original 512 512 2 > output.raw
	