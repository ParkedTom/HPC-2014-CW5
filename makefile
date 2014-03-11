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
	
test_stars : bin/original bin/process
	cat stars.raw | ./bin/original 1632 1170 2 15 > output.raw
	cat stars.raw | ./bin/process 1632 1170 2 15 > output_process.raw

test_lenna : bin/original bin/process
	cat lenna.raw | ./bin/original 512 512 2 15 > lenna_out.raw
	cat lenna.raw | ./bin/process 512 512 2 15 > lenna_out_proc.raw

compare_stars: test_lenna
	cmp --verbose output.raw output_process.raw 

compare_lenna: test_lenna
	cmp --verbose lenna_out.raw lenna_out_proc.raw 
	
all : bin/process bin/original
	
