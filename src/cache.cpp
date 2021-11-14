#include <cstdlib>
#include <cassert>
#include <cstring>
#include "cache.h"

bool is_power_of_two(uint64_t x) {
  return x && !(x & (x - 1));
}

void Cache::SetConfig(CacheConfig cc) {
  // Check if config is valid
  assert(is_power_of_two(cc.size));
  assert(is_power_of_two(cc.set_num));
  assert(is_power_of_two(cc.associativity));
  assert(cc.set_num * cc.associativity < cc.size);
  cc.block_size = cc.size / (cc.set_num * cc.associativity);

  config_ = cc;
  sets = static_cast<CacheSet *>(malloc(sizeof(CacheSet) * cc.set_num));
  for (int i = 0; i < cc.set_num; i++) {
    sets[i].lines = static_cast<CacheLine *>(malloc(sizeof(CacheLine) * cc.associativity));
    for (int j = 0; j < cc.associativity; j++) {
      CacheLine &line = sets[i].lines[j];
      line.valid = false;
      line.dirty = false;
      line.blocks = static_cast<uint64_t *>(malloc(sizeof(uint64_t) * cc.block_size));
      memset(line.blocks, 0, sizeof(line.blocks) * cc.block_size);
    }
  }
}

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
  return false;
}

void Cache::PartitionAlgorithm() {
}

int Cache::ReplaceDecision() {
  return true;
  //return FALSE;
}

void Cache::ReplaceAlgorithm() {
}

int Cache::PrefetchDecision() {
  return false;
}

void Cache::PrefetchAlgorithm() {
}

