#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <mastik/ps.h>
#include <mastik/l3.h>
#include <mastik/lx.h>
#include <mastik/impl.h>
#include <src/vlist.h> // FIXME: BAD
#include <src/mm-impl.h>

#define COMPILER_BARRIER() asm volatile("" ::: "memory");

struct shared_data {
  bool perform_action;
  bool do_access;
  bool should_exit;
  void* addr;
};
struct shared_data* shared_data;

pid_t spawn_child(void* addr) {
  pid_t child = fork();
  if(child == 0) {
    while(!shared_data->should_exit) {
      while(!shared_data->perform_action) {
        sched_yield();
      }
      COMPILER_BARRIER();
      if(shared_data->do_access)
        memaccesstime(shared_data->addr);
      COMPILER_BARRIER();
      shared_data->perform_action = false;
    }

    exit(0);
  } else {
    return child;
  }
}

void offset_evset(vlist_t evset, uintptr_t offset) {
  for(int i = 0; i < vl_len(evset); i++) {
    vl_set(evset, i, (uintptr_t)vl_get(evset, i) + offset);
  }
}

struct timespec time_diff(struct timespec start, struct timespec end){
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

int main() {
  shared_data = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  assert(shared_data);
  memset(shared_data, 0, sizeof(struct shared_data));

  l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
  l3i->associativity = 0;
  l3i->slices = 0;
  l3i->setsperslice = 0;
  l3i->bufsize = 0;
  l3i->flags = L3FLAG_LINEARMAP;
  l3i->progressNotification = NULL;
  l3i->progressNotificationData = NULL;
  l3pp_t l3 = l3_prepare(l3i, NULL);
  assert(l3_getSets(l3) == 4096); // FIXME: REMOVE
    
  ppattern_t best_pat;
  // best_pat.length = 8; Max: 277381 group: 0 set: 364
  // best_pat.repeat = 3;
  // best_pat.stride = 1;
  // best_pat.width = 2;
  // uint8_t _pat[20] = {0, 1, 2, PATTERN_TARGET, 0, 1, 1, 2};
  // best_pat.length = 6; // Max: 281430 group: 3 set: 450
  // best_pat.repeat = 4;
  // best_pat.stride = 3;
  // best_pat.width = 2;
  // uint8_t _pat[20] = {1, 2, 0, PATTERN_TARGET, PATTERN_TARGET, 2};
  // best_pat.length = 8; // Max: 264354 group: 3 set: 935
  // best_pat.repeat = 3;
  // best_pat.stride = 2;
  // best_pat.width = 2;
  // uint8_t _pat[20] = {1, PATTERN_TARGET, PATTERN_TARGET, 2, 0, 1, 0, 2};
  best_pat.length = 10; // Max: 282647 group: 3 set: 433
  best_pat.repeat = 4;
  best_pat.stride = 1;
  best_pat.width = 2;
  uint8_t _pat[20] = {0, PATTERN_TARGET, PATTERN_TARGET, 2, 2, PATTERN_TARGET, 0, 1, PATTERN_TARGET, 1};
  // best_pat.length = 2; // Max: 264359 group: 3 set: 824
  // best_pat.repeat = 16;
  // best_pat.stride = 1;
  // best_pat.width = 0;
  // uint8_t _pat[20] = {0, PATTERN_TARGET};
  
  // // R2_S4_P1S01
  // best_pat.length = 4;
  // best_pat.repeat = 2;
  // best_pat.stride = 4;
  // best_pat.width = 1;
  // uint8_t _pat[20] = {1, PATTERN_TARGET, 0, 1};

  memcpy(best_pat.pattern, _pat, sizeof(_pat));

  // vlist_t evset = l3_getGroups(l3)[1];
  // printf("before: %p %p\n", vl_get(evset, 0), getphysaddr(vl_get(evset, 0)));
  // offset_evset(evset, 16*LX_CACHELINE);
  // printf("after:  %p %p\n", vl_get(evset, 0), getphysaddr(vl_get(evset, 0)));
  // l3_verify_evset(l3, evset);
  int assoc = l3_getAssociativity(l3);


  // int max = 0;
  // int max_group = 0;
  // int max_set = 0;
  // for(int i = 0; i < l3_getNumGroups(l3); i++) {
  //   for(int j = 0; j < l3_getGroupSize(l3); j++) {
  //     vlist_t evset = l3_getGroups(l3)[i];
  //     if(j > 0) offset_evset(evset, LX_CACHELINE);
  //     l3_verify_evset(l3, evset);
  //     ps_t ps = ps_prepare(best_pat, assoc, evset);
  //     uint64_t sum = 0;
  //     for(int k = 0; k < 100; k++) {
  //       sum += ps_prime_and_scope(ps);
  //     }
  //     int avg = sum/100;
  //     if(avg > max) {
  //       max = avg;
  //       max_group = i;
  //       max_set = j;
  //     }
  //     printf("%d:%d %d\n", i, j, sum/100);
  //     ps_release(ps);
  //   }
  // }
  // printf("Max: %d group: %d set: %d\n", max, max_group, max_set);

  vlist_t evset = l3_getGroups(l3)[3];
  offset_evset(evset, 43*LX_CACHELINE);

  // {
  //   struct timespec res;
  //   clock_getres(CLOCK_PROCESS_CPUTIME_ID, &res);
  //   printf("Clock resolution is %lds:%ldns\n", res.tv_sec, res.tv_nsec);
  // }

  // printf("Priming...\n");
#if 1
  struct timespec t1, t2;
  for(int i = 0; i < 10; i++) {
    ps_t ps = ps_prepare(best_pat, assoc, evset);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    int iterations = ps_prime_and_scope(ps);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);
    long int diff = time_diff(t1, t2).tv_nsec;
    printf("Iterations: %6d ms: %ld ns: %ld\n", iterations, diff / 1000000, diff);
    ps_release(ps);

    offset_evset(evset, 3*LX_CACHELINE);
  }
#else
  // printf("Scoped!\n");

  pid_t child = spawn_child(vl_get(evset, assoc)); // FIXME: Better interface

  int num_passed = 0;
  for(int test = 0; test < 100; test++) {
    ps_prime(ps);

    ps_scope(ps);
    if(ps_scope(ps)) {
      printf("Sanity check %d failed, exiting...\n", test);
      goto exit;
    }
    printf("Sanity check %d passed\n", test);

    int test_type = test%2;
    ps_prime(ps);
    int c1 = ps_scope(ps);
    {
      COMPILER_BARRIER();
      shared_data->addr = vl_get(evset, assoc);
      shared_data->do_access = test_type;
      COMPILER_BARRIER(); 
      shared_data->perform_action = true;
      COMPILER_BARRIER();
      while(shared_data->perform_action) {
        sched_yield();
      }
      COMPILER_BARRIER();
    }
    int c2 = ps_scope(ps);
    if((test_type && !c1 && c2) || (!test_type && !c1 && !c2)) {
      printf("PS check %d passed\n", test);
      num_passed++;
    } else {
      printf("PS check %d failed (c1=%d, c2=%d)\n", test, c1, c2);
    }
  }
  printf("Passed %d/100 tests\n", num_passed);

exit:
  shared_data->should_exit = true;
  kill(child, SIGKILL);

#endif
  // ps_release(ps);


  return 0;
}