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
	convert lenna.png -resize 2048x2048 -depth 2 gray:- | ./bin/original 2048 2048  2 1 > output.raw
	convert lenna.png -resize 2048x2048 -depth 2 gray:- | ./bin/process  2048 2048 2 1 > output_process.raw
	cmp --verbose output.raw output_process.raw
	
all : bin/process bin/original
	