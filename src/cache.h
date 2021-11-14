#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"

typedef struct CacheConfig_ {
  int size;
  int associativity; // Number of cache lines per set
  int set_num; // Number of cache sets
  int block_size;
  bool write_through; // 0|1 for back|through
  bool write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

typedef struct CacheLine_ {
  bool valid;
  bool dirty;
  uint64_t tag;
  uint64_t *blocks;
} CacheLine;

typedef struct CacheSet_ {
  CacheLine *lines;
} CacheSet;

class Cache: public Storage {
 public:
  Cache() {}
  ~Cache() {}

  // Sets & Gets
  void SetConfig(CacheConfig cc);
  void GetConfig(CacheConfig cc);
  void SetLower(Storage *ll) { lower_ = ll; }
  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time);

 private:
  // Bypassing
  int BypassDecision();
  // Partitioning
  void PartitionAlgorithm();
  // Replacement
  int ReplaceDecision();
  void ReplaceAlgorithm();
  // Prefetching
  int PrefetchDecision();
  void PrefetchAlgorithm();

  CacheSet *sets;
  CacheConfig config_;
  Storage *lower_;
  DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
