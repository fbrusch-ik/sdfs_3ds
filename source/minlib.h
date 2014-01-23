#ifndef MINILIB_H
#define MINILIB_H

#include "3dstypes.h"

typedef unsigned int size_t;

void * memcpy(void *out, const void *in, int size);
int strlen(const char *text);
//bool strcmp(const char *text, const char *text1);
void write_word(u32 address, u32 word);
void write_color(u32 address, u8 r, u8 g, u8 b);
void write_byte(u32 address, u8 byte);
void * memset(void *ptr, int c, size_t size);
u8 read_byte(u32 address);
u32 read_word(u32 address);
uint64_t GetSystemTick(void);
void nsleep(uint64_t ns);
void usleep(uint64_t us);
void msleep(uint64_t ms);
void sleep(uint64_t s);

#endif