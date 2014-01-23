#include "minlib.h"

void * memcpy(void *out, const void *in, int size)
{
  byte *pout = out;
  byte *pin = (byte *)in;
  int i;
  for(i = 0; i < size; i++) {
    *pout = *pin;
    pout++;
    pin++;
  }
    return out;
}

int strlen(const char *text)
{
  int i = 0;
  while(*text) 
  {
    text++;
    i++;
  }
  return i;
}

void * memset(void *ptr, int c, size_t size)
{
  byte *p1 = ptr;
  byte *p2 = ptr + size;
  while(p1 != p2) {
    *p1 = c;
    p1++;
  }
    return ptr;
}

void write_word(u32 address, u32 word){
  u32 *a = (u32*) address;
  *a = word;
}

void write_color(u32 address, u8 r, u8 g, u8 b){
  write_byte(address, b);
  write_byte(address+1, g);
  write_byte(address+2, r);
}

void write_byte(u32 address, u8 byte){
  char *a = (char*) address;
  *a = byte;
}

u8 read_byte(u32 address){
  return *(u8*)address;
}

u32 read_word(u32 address){
  return *(u32*)address;
}


/*
* GetSystemTick and sleep functions by xerpi
*
*/
uint64_t GetSystemTick(void)
{
    register unsigned long lo64 asm ("r0");
    register unsigned long hi64 asm ("r1");
    asm volatile ( "SVC 0x28" : "=r"(lo64), "=r"(hi64) );
    return ((uint64_t)hi64<<32) | (uint64_t)lo64;
}

void nsleep(uint64_t ns)
{
    uint64_t ticks = ns;
    uint64_t start = GetSystemTick();
    while ((GetSystemTick() - start) < ticks);
}

void usleep(uint64_t us)
{
    uint64_t ticks = us * TICKS_PER_USEC;
    uint64_t start = GetSystemTick();
    while ((GetSystemTick() - start) < ticks);
}

void msleep(uint64_t ms)
{
    uint64_t ticks = ms * TICKS_PER_MSEC;
    uint64_t start = GetSystemTick();
    while ((GetSystemTick() - start) < ticks);  
}

void sleep(uint64_t s)
{
    uint64_t ticks = s * TICKS_PER_SEC;
    uint64_t start = GetSystemTick();
    while ((GetSystemTick() - start) < ticks); 
}