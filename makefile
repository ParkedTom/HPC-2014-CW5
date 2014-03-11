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
	convert testImage.png -depth 2 gray:- | ./bin/original 8192 8192 2 13 > output.raw
	convert testImage.png -depth 2 gray:- | ./bin/process  8192 8192 2 13 > output_process.raw
	cmp --verbose output.raw output_process.raw
	convert testImage.png -depth 2 gray:- | ./bin/original 8192 8192 2 -13 > output.raw
	convert testImage.png -depth 2 gray:- | ./bin/process  8192 8192 2 -13 > output_process.raw
	cmp --verbose output.raw output_process.raw
	convert testImage.png -depth 8 gray:- | ./bin/original 8192 8192 8 25 > output.raw
	convert testImage.png -depth 8 gray:- | ./bin/process  8192 8192 8 25 > output_process.raw
	cmp --verbose output.raw output_process.raw                      
	convert testImage.png -depth 8 gray:- | ./bin/original 8192 8192 8 -25 > output.raw
	convert testImage.png -depth 8 gray:- | ./bin/process  8192 8192 8 -25 > output_process.raw
	cmp --verbose output.raw output_process.raw
	convert stars.png -depth 8 gray:- | ./bin/original 1632 1170 8 -25 > output.raw 
	convert stars.png -depth 8 gray:- | ./bin/process  1632 1170 8 -25 > output_process.raw
	cmp --verbose output.raw output_process.raw                 
	#convert stars.png -depth 8 gray:- | ./bin/original 1632 1170 8 25 > output.raw
	#convert stars.png -depth 8 gray:- | ./bin/process  1632 1170 8 25 > output_process.raw 
	#cmp --verbose output.raw output_process.raw
	convert testImage.png -depth 32 gray:- | ./bin/original 8192 8192 32 64 > output.raw
	convert testImage.png -depth 32 gray:- | ./bin/process  8192 8192 32 64 > output_process.raw
	cmp --verbose output.raw output_process.raw
	
	
all : bin/process bin/original
	