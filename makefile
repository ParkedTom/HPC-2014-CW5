CPPFLAGS =  -I include -Wall -std=c++0x
LDFLAGS = 
LDLIBS = -lm -ltbb
CPPFLAGS += -O2

TBB_DIR = ~/tbb42_20140122oss

TBB_INC_DIR = $(TBB_DIR)/include

TBB_LIB_DIR = $(TBB_DIR)/lib

CPPFLAGS += -I $(TBB_INC_DIR)

LDFLAGS += -L $(TBB_LIB_DIR)

bin/original : src/original/original.cpp
	$(CXX) $(CPPFLAGS) $^ -o $@ 
	
bin/process : src/main.cpp src/processes.cpp
	$(CXX) $(CPPFLAGS)  src/main.cpp src/processes.cpp -o $@ 

test : bin/original bin/process
	convert lenna.png -depth 2 gray:- | ./bin/original 512 512 2 1 > output.raw
	convert lenna.png -depth 2 gray:- | ./bin/process 512 512 2 1 > output_process.raw
	cmp --verbose output.raw output_process.raw

	
all : bin/process bin/original
	
