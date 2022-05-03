#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <mastik/ps.h>
#include <mastik/pt.h>
#include <mastik/l3.h>

int main() {
  l3info_t l3i = (l3info_t)malloc(sizeof(struct l3info));
  l3i->associativity = 0;
  l3i->slices = 0;
  l3i->setsperslice = 0;
  l3i->bufsize = 0;
  l3i->flags = 0;
  l3i->progressNotification = NULL;
  l3i->progressNotificationData = NULL;
  l3pp_t l3 = l3_prepare(l3i, NULL);
  assert(l3_getSets(l3) == 4096); // FIXME: REMOVE

  pt_t patterns = pt_prepare(l3_getAssociativity(l3), l3_getGroups(l3)[2]);
  pt_results_t results = generate_prime_patterns(patterns);

  printf("EVCr    | Cycles | Pattern\n");
  for(int i = 0; i < PT_RESULTS_COUNT; i++){
    printf("%7.3f%% | %6d | ", results->evcrs[i]*100, results->cycles[i]);
    dump_ppattern(results->patterns[i]);
    printf("\n");
  }
}