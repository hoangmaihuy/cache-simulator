#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.hpp"
#include <vector>
#include <string>

using namespace std;

#define ADDR_LEN 64

typedef struct CacheConfig_ {
  int size;
  int associativity; // Number of cache lines per set
  int set_num; // Number of cache sets
  int block_size;
  bool write_through; // 0|1 for back|through
  bool write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

typedef struct CacheLine_ {
  int access_counter;
  bool valid;
  bool dirty;
  uint64_t tag;
  vector<char> blocks;
} CacheLine;

typedef struct CacheSet_ {
  vector<CacheLine> lines;
} CacheSet;

class Cache : public Storage {
public:
  Cache() { }

  ~Cache() {}

  // Sets & Gets
  bool SetConfig(CacheConfig cc);

  void GetConfig(CacheConfig cc);

  void SetLower(Storage *ll) { lower_ = ll; }

  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time);


private:
  // Bypassing
  bool BypassDecision();

  // Partitioning
  void PartitionAlgorithm(uint64_t addr, uint64_t &set_idx, uint64_t &tag, uint64_t &block_offset);

  int GetLine(uint64_t set_idx, uint64_t tag);

  void ReadRequest(uint64_t set_idx, uint64_t line_idx, uint64_t block_offset, int bytes, char *content);

  void WriteRequest(uint64_t set_idx, uint64_t line_idx, uint64_t block_offset, uint64_t tag, int bytes, char *content, bool dirty);

  // Replacement
  bool ReplaceDecision(int line_idx, int read);

  int ReplaceAlgorithm(uint64_t set_idx, int &time);

  // Prefetching
  bool PrefetchDecision();

  void PrefetchAlgorithm();

  int t, s, b; // Number of tag/set/block bits

  vector<CacheSet> sets;
  CacheConfig config_;
  Storage *lower_;
  DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
