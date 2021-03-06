#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>

uint64_t shuffle64(unsigned bits, uint64_t x);

void unpack_blob(unsigned w, unsigned h, unsigned bits, const uint64_t *pRaw, uint32_t *pUnpacked);

void pack_blob(unsigned w, unsigned h, unsigned bits, const uint32_t *pUnpacked, uint64_t *pRaw);

bool read_blob(int fd, uint64_t cbBlob, void *pBlob);

void write_blob(int fd, uint64_t cbBlob, const void *pBlob);

uint32_t vmin(uint32_t a, uint32_t b);

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c);

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

void erode(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels, unsigned no_frames);

uint32_t vmax(uint32_t a, uint32_t b);

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c);

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

void dilate(unsigned w, unsigned h, const std::vector<uint32_t> &input, std::vector<uint32_t> &output, uint32_t count, unsigned levels, unsigned no_frames);

void process(int levels, unsigned w, unsigned h, unsigned no_frames, std::vector<uint32_t> &pixels, uint32_t count);

void invert(int levels, unsigned w, unsigned h, unsigned bits, std::vector<uint32_t> &pixels);