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
	convert testImage.png -depth 2 gray:- | ./bin/original 8192 8192 2 13 > output_erode.raw
	convert testImage.png -depth 2 gray:- | ./bin/process  8192 8192 2 13 > output_process_erode.raw 2> open.txt
	#cmp --verbose output.raw output_process.raw
	convert testImage.png -depth 2 gray:- | ./bin/original 8192 8192 2 -13 > output_dilate.raw
	convert testImage.png -depth 2 gray:- | ./bin/process  8192 8192 2 -13 > output_process_dilate.raw 2> close.txt
	convert -size 8192X8192 -depth 2 gray:output_dilate.raw output_dilate.png
	convert -size 8192X8192 -depth 2 gray:output_process_dilate.raw output_process_dilate.png
	convert -size 8192X8192 -depth 2 gray:output_erode.raw output_erode.png
	convert -size 8192X8192 -depth 2 gray:output_process_erode.raw output_process_erode.png
	#cmp --verbose open.txt close.txt
	cmp --verbose output_erode.raw output_process_erode.raw
	cmp --verbose output_dilate.raw output_process_dilate.raw
 #convert testImage.png -depth 8 gray:- | ./bin/original 8192 8192 8 15 > output.raw
 #convert testImage.png -depth 8 gray:- | ./bin/process  8192 8192 8 15 > output_process.raw
 #cmp --verbose output.raw output_process.raw
	
all : bin/process bin/original
	
