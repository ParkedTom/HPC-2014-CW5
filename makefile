CPPFLAGS = -std=c++11 -Wall -stdlib=libstdc++ -I include
LDFLAGS = 
LDLIBS = -lm -ltbb
CPPFLAGS += -O2

TBB_DIR = ~/tbb42_20140122oss

TBB_INC_DIR = $(TBB_DIR)/include

TBB_LIB_DIR = $(TBB_DIR)/lib

CPPFLAGS += -I $(TBB_INC_DIR)

LDFLAGS += -L $(TBB_LIB_DIR)

bin/original : src/original/original.cpp
	clang++ $(CPPFLAGS) $^ -o $@ 
	
bin/process : src/main.cpp src/processes.cpp
	clang++ $(CPPFLAGS)  src/main.cpp src/processes.cpp -o $@ $(LDFLAGS) $(LDLIBS)
	
test : bin/original bin/process
	convert lenna.png -depth 2 gray:- | ./bin/original 512 512 2 -1 > output_original.raw
	convert lenna.png -depth 2 gray:- | ./bin/process 512 512 2 -1 > output_process.raw
	time dd if=/dev/zero bs=4194304 count=1 | ./bin/original 4096 4096 2 -1 > /dev/null
	time dd if=/dev/zero bs=4194304 count=1 | ./bin/process 4096 4096 2 -1 > /dev/null
	time dd if=/dev/zero bs=16777216 count=1 | ./bin/original 8192 8192 2 -1 > /dev/null
	time dd if=/dev/zero bs=16777216 count=1 | ./bin/process 8192 8192 2 -1 > /dev/null
	diff output_original.raw output_process.raw
	
all : bin/process bin/original
	