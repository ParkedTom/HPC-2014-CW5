# CPPFLAGS = -std=c++0x -stdlib=libstdc++ -I include
CPPFLAGS =  -I include -Wall -std=c++0x

bin/original : src/original/original.cpp
	$(CXX) $(CPPFLAGS) $^ -o $@ 
	
bin/process : src/main.cpp src/processes.cpp
	$(CXX) $(CPPFLAGS)  src/main.cpp src/processes.cpp -o $@ 
	
test: bin/process
	convert lenna.png -depth 2 gray:- | ./bin/original 512 512 2 > output_process.raw

time : bin/original bin/process
	time(convert lenna.png -depth 2 gray:- | ./bin/original 512 512 2 > output_original.raw) 
	time(convert lenna.png -depth 2 gray:- | ./bin/process 512 512 2 > output_process.raw) 
	diff output_original.raw output_process.raw
	
all : bin/process bin/original
	
