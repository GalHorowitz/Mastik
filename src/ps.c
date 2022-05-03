#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <mastik/ps.h>
#include <mastik/low.h>
#include <mastik/l3.h>
#include <mastik/mm.h>
#include "vlist.h"
#include "mm-impl.h"

void dump_ppattern(ppattern_t pat) {
  printf("R%d_S%d_P", pat.repeat, pat.stride);
  for(int i = 0; i < pat.length; i++) {
    if(pat.pattern[i] == PATTERN_TARGET) {
      printf("S");
    } else {
      printf("%d", pat.pattern[i]);
    }
  }
}

void access_pattern(vlist_t evset, int assoc, ppattern_t pattern) {
  // TODO: Flatten out the list
  for(int r = 0; r < pattern.repeat; r++) {
    for(int i = 0; i < assoc - pattern.width; i++) {
      for(int j = 0; j < pattern.length; j++) {
        int a = pattern.pattern[j];
        if(a == PATTERN_TARGET) {
          // memaccesstime(vl_get(evset, 0)); // FIXME: CHANGED TO SERIAL
          memaccess(vl_get(evset, 0));
        } else {
          assert(a <= pattern.width);
          // memaccesstime(vl_get(evset, i + a)); // FIXME: CHANGED TO SERIAL
          memaccess(vl_get(evset, i + a));
        }
      }
    }
  }
}


struct ps { 
  ppattern_t prime_pattern;
  int associativity;
  vlist_t evset;
};

ps_t ps_prepare(ppattern_t prime_pattern, int associativity, vlist_t evset) {
  ps_t rv = malloc(sizeof(struct ps));
  assert(vl_len(evset) > associativity + 1);
  rv->prime_pattern = prime_pattern;
  rv->associativity = associativity;
  rv->evset = evset;
  return rv;
}

void ps_prime(ps_t ps) {
  access_pattern(ps->evset, ps->associativity, ps->prime_pattern);
}

int ps_scope(ps_t ps) {
  void* target = vl_get(ps->evset, 0);
  uint32_t t = memaccesstime(target);
  return t > L3_THRESHOLD;
}

struct debug_dat {
  int acc;
  void* addr;
};
struct debug_dat* debug_shared;

#include <unistd.h>
pid_t debug_proc() {
    pid_t pid = fork();
    if(pid == 0) {
      while(!debug_shared->acc) {
        printf(".");
      }
      printf("Accessing %p\n", debug_shared->addr);
      memaccesstime(debug_shared->addr);
      exit(0);
    }

    return pid;
}

#include <sys/mman.h>
#include <signal.h>
#include <sched.h>
int ps_prime_and_scope(ps_t ps) {
  debug_shared = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  debug_shared->acc = 0;
  debug_shared->addr = vl_get(ps->evset, ps->associativity);
  pid_t child = debug_proc();

  void* target = vl_get(ps->evset, 0);
  access_pattern(ps->evset, ps->associativity, ps->prime_pattern);

  // int num_out = 0;
  int i = 0;
  while(1) {
    if(i > 1) {
      debug_shared->acc = 1;
    }

    uint32_t t = memaccesstime(target);
    if(t > L3_THRESHOLD) {
      // printf("scope %d t=%d\n", i, t);
      // if(num_out > 0) printf("(%d) %d ? %d\n", num_out, t, L3_THRESHOLD);
      // num_out++;
      // if(num_out > 5) {
      kill(child, SIGKILL);
      munmap(debug_shared, 0x1000);
      return i;
      // }
    } else {
      // num_out = 0;
      sched_yield();
    }
    i++;
    // delayloop(10);
  }
}

void ps_release(ps_t ps) {
  ps->evset = NULL;
  free(ps);
}
