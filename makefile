CPPFLAGS = -std=c++11 -stdlib=libstdc++ -I include

bin/original : src/original/original.cpp
	clang++ $(CPPFLAGS) $^ -o $@ 
	
bin/process : src/main.cpp src/processes.cpp
	clang++ $(CPPFLAGS)  src/main.cpp src/processes.cpp -o $@ 
	
test : bin/original
	time (convert lenna.png -depth 2 gray:- | ./bin/original 512 512 2 > output_original.raw)
	time (convert lenna.png -depth 2 gray:- | ./bin/process 512 512 2 > output_process.raw)
	diff output_original.raw output_process.raw
	
all : bin/process bin/original
	