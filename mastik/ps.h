#ifndef __PS_H__
#define __PS_H__ 1

#include <mastik/lx.h>

#define PATTERN_CAPACITY 20
#define PATTERN_TARGET 0xFF
struct prime_pattern {
  uint8_t repeat;
  uint8_t stride;
  // The largest offset from i
  uint8_t width;
  uint8_t length;
  // 0xFF denotes the target, other values denote offset from i
  uint8_t pattern[PATTERN_CAPACITY];
};
typedef struct prime_pattern ppattern_t;
void dump_ppattern(ppattern_t pattern);
void access_pattern(vlist_t evset, int associativity, ppattern_t pattern);

typedef struct ps *ps_t;
ps_t ps_prepare(ppattern_t prime_pattern, int associativity, vlist_t evset);
int ps_prime_and_scope(ps_t ps);
void ps_prime(ps_t ps);
int ps_scope(ps_t ps);
void ps_release(ps_t ps);

#endif // __PS_H__
