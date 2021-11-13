#include "stdio.h"
#include "cache.h"
#include "memory.h"

int main(void) {
  Memory m;
  Cache l1;
  l1.SetLower(&m);

  StorageStats s;
  s.access_time = 0;
  m.SetStats(s);
  l1.SetStats(s);

  StorageLatency ml;
  ml.bus_latency = 6;
  ml.hit_latency = 100;
  m.SetLatency(ml);

  StorageLatency ll;
  ll.bus_latency = 3;
  ll.hit_latency = 10;
  l1.SetLatency(ll);

  int hit, time;
  char content[64];
  l1.HandleRequest(0, 0, 1, content, hit, time);
  printf("Request access time: %dns\n", time);
  l1.HandleRequest(1024, 0, 1, content, hit, time);
  printf("Request access time: %dns\n", time);

  l1.GetStats(s);
  printf("Total L1 access time: %dns\n", s.access_time);
  m.GetStats(s);
  printf("Total Memory access time: %dns\n", s.access_time);
  return 0;
}
