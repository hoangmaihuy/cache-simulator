#include <iostream>
#include <fstream>
#include <argparse/argparse.hpp>
#include "config.h"
#include "cache.h"
#include "memory.h"

using namespace std;

string trace_path;
CacheConfig l1_config;
Memory mem;
Cache l1;

void parse_args(int argc, char *argv[]) {
  argparse::ArgumentParser parser("cache-simulator");

  parser.add_argument("trace-path")
      .help("Path to trace file");

  parser.add_argument("--size")
      .help("Cache size (bytes)")
      .default_value(DEFAULT_CACHE_SIZE);

  parser.add_argument("--setnum")
      .help("Number of cache sets")
      .default_value(DEFAULT_CACHE_SETS);

  parser.add_argument("--associativity")
      .help("Number of lines per cache set")
      .default_value(DEFAULT_CACHE_LINES);

  parser.add_argument("--write-through")
      .help("Enable write through, default write-back")
      .default_value(DEFAULT_WRITE_THROUGH)
      .implicit_value(!DEFAULT_WRITE_THROUGH);

  parser.add_argument("--write-allocate")
      .help("Enable write allocate, default no-write-allocate")
      .default_value(DEFAULT_WRITE_ALLOCATE)
      .implicit_value(!DEFAULT_WRITE_ALLOCATE);

  try {
    parser.parse_args(argc, argv);
  }
  catch (const runtime_error &err) {
    cerr << err.what() << endl;
    cerr << parser;
    exit(1);
  }

  trace_path = parser.get<string>("trace-path");
  l1_config.size = parser.get<int>("--size");
  l1_config.set_num = parser.get<int>("--setnum");
  l1_config.associativity = parser.get<int>("--associativity");
  l1_config.write_through = parser.get<bool>("--write-through");
  l1_config.write_allocate = parser.get<bool>("--write-allocate");
}

void init_cache() {
  StorageStats stats;
  memset(&stats, 0, sizeof(stats));

  // Init L1 cache
  l1.SetStats(stats);
  l1.SetLower(&mem);
  l1.SetConfig(l1_config);
  l1.SetLatency({L1_HIT_LATENCY, L1_BUS_LATENCY});

  // Init memory
  mem.SetStats(stats);
  mem.SetLatency({MEM_HIT_LATENCY, MEM_BUS_LATENCY});
}

void handle_trace() {
  ifstream fi;
  fi.open(trace_path);
  char op;
  uint64_t addr;
  while (fi >> op >> hex >> addr) {
  }
}

int main(int argc, char *argv[]) {
  parse_args(argc, argv);
  init_cache();
  handle_trace();
  return 0;
}
