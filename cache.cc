#include "cache.h"
#include "def.h"

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
  hit = 0;
  time = 0;
  // Bypass?
  if (!BypassDecision()) {
    PartitionAlgorithm();
    // Miss?
    if (ReplaceDecision()) {
      // Choose victim
      ReplaceAlgorithm();
    } else {
      // return hit & time
      hit = 1;
      time += latency_.bus_latency + latency_.hit_latency;
      stats_.access_time += time;
      return;
    }
  }
  // Prefetch?
  if (PrefetchDecision()) {
    PrefetchAlgorithm();
  } else {
    // Fetch from lower layer
    int lower_hit, lower_time;
    lower_->HandleRequest(addr, bytes, read, content,
                          lower_hit, lower_time);
    hit = 0;
    time += latency_.bus_latency + lower_time;
    stats_.access_time += latency_.bus_latency;
  }
}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

int Cache::ReplaceDecision() {
  return TRUE;
  //return FALSE;
}

void Cache::ReplaceAlgorithm(){
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

