CPPFLAGS = -std=c++11 -stdlib=libstdc++ -framework OpenCL -I include

bin/original : src/original/original.cpp
	clang++ $(CPPFLAGS) $^ -o $@ 
	
test : bin/original
	convert lenna.png -depth 2 gray:- | ./bin/original 512 512 2 > output.raw
	