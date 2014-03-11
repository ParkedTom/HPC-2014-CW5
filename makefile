CPPFLAGS = -std=c++11 -Wall -I include -g 
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
	$(CXX) $(CPPFLAGS)  src/main.cpp src/processes.cpp -o $@ $(LDFLAGS) $(LDLIBS)
	
test : bin/original bin/process
	convert stars.png -depth 2 gray:- | ./bin/original 1632 1170 2  -2 > output.raw
	convert stars.png -depth 2 gray:- | ./bin/process 1632 1170  2 -2 > output_process.raw
	cmp --verbose output.raw output_process.raw
	
all : bin/process bin/original
	
