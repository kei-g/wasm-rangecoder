#ifndef __include_rangecoder_h__
#define __include_rangecoder_h__

typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

extern void __flush(uint32_t id);
extern int __getbyte(uint32_t id);
extern void __putbyte(uint32_t id, uint8_t value);
extern void __rewind(uint32_t id);
extern uint32_t getdecompressedsize(uint32_t id);
extern uint16_t getfreq(uint32_t id, uint8_t symbol);
extern void putfreq(uint32_t id, uint8_t symbol, uint16_t count);

typedef struct _frequency freq_t;
typedef union _uint64 uint64_u;

void shift_left(uint64_u *dst, int length);

struct _frequency {
  uint32_t count;
  uint32_t cumulated;
};

union _uint64 {
  struct {
    uint32_t lo;
    uint32_t hi;
  };
  uint64_t value;
};

#endif /* __include_rangecoder_h__ */
