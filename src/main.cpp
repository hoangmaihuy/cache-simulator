#include <iostream>
#include <fstream>
#include <argparse/argparse.hpp>
#include "config.hpp"
#include "cache.hpp"
#include "memory.hpp"

using namespace std;

string trace_path;
CacheConfig l1_config, l2_config;
Memory *mem;
Cache *l1;
Cache *l2;
int total_hit, total_time, total_request, iter;

bool verbose = false;
bool optimize = false;

void parse_args(int argc, char *argv[]) {
  argparse::ArgumentParser parser("cache-simulator");

  parser.add_argument("trace-path")
      .help("Path to trace file");

  parser.add_argument("--verbose")
      .help("Verbose mode")
      .default_value(false)
      .implicit_value(true);

  parser.add_argument("--optimized")
      .help("Use optimized config")
      .default_value(false)
      .implicit_value(true);

  parser.add_argument("--iter")
      .help("Trace iteration count")
      .default_value(10)
      .scan<'i', int>();;

  try {
    parser.parse_args(argc, argv);
  }
  catch (const runtime_error &err) {
    cerr << err.what() << endl;
    cerr << parser;
    exit(1);
  }

  trace_path = parser.get<string>("trace-path");
  verbose = parser.get<bool>("--verbose");
  optimize = parser.get<bool>("--optimized");
  iter = parser.get<int>("--iter");

  l1_config.size = L1_CACHE_SIZE;
  l1_config.block_size = L1_BLOCK_SIZE;
  l1_config.associativity = L1_CACHE_LINES;
  l1_config.write_through = L1_WRITE_THROUGH;
  l1_config.write_allocate = L1_WRITE_ALLOCATE;
  l1_config.prefetch = 3;
  l1_config.replacement = "lru";
  l1_config.mct = 0;
  l1_config.bypass = false;


  l2_config.size = L2_CACHE_SIZE;
  l2_config.block_size = L2_BLOCK_SIZE;
  l2_config.associativity = L2_CACHE_LINES;
  l2_config.write_through = L2_WRITE_THROUGH;
  l2_config.write_allocate = L2_WRITE_ALLOCATE;
  l2_config.prefetch = 3;
  l2_config.replacement = "plru";
  l2_config.mct = 1;
  l2_config.bypass = true;
}

void init_cache() {
  StorageStats stats;
  memset(&stats, 0, sizeof(stats));

  l1 = new Cache();

  l2 = new Cache();
  mem = new Memory();

  // Init L1 cache
  l1->SetStats(stats);
  l1->SetLower(l2);
  l1->SetConfig(l1_config);
  l1->SetLatency({L1_HIT_LATENCY, L1_BUS_LATENCY});

  // Init L2 cache
  l2->SetStats(stats);
  l2->SetLower(mem);
  l2->SetConfig(l2_config);
  l2->SetLatency({L2_HIT_LATENCY, L2_BUS_LATENCY});

  // Init memory
  mem->SetStats(stats);
  mem->SetLatency({MEM_HIT_LATENCY, MEM_BUS_LATENCY});
}

void handle_trace() {
  total_hit = 0;
  total_time = 0;
  total_request = 0;
  char *buf = static_cast<char *>(malloc(sizeof(char) * l1_config.block_size));
  while (iter--) {
    ifstream fi;
    fi.open(trace_path);
    char op;
    uint64_t addr;
    int hit, time;
    while (fi >> op >> hex >> addr) {
      total_request++;
      if (op == 'r') {
        l1->HandleRequest(addr, 1, 1, buf, hit, time);
      } else {
        l1->HandleRequest(addr, 1, 0, buf, hit, time);
      }
      total_hit += hit;
      total_time += time;
      if (verbose) {
        cerr << op << " " << hex << addr << ": " << hit << " " << time << "\n";
      }
    }
    fi.close();
  }
  free(buf);
}

void print_stats() {
  StorageStats l1_stats;
  StorageStats l2_stats;
  StorageStats mem_stats;
  l1->GetStats(l1_stats);
  l2->GetStats(l2_stats);
  mem->GetStats(mem_stats);

  double l1_mr = (double)l1_stats.miss_num / l1_stats.access_counter;
  double l2_mr = (double)l2_stats.miss_num / l2_stats.access_counter;

  double amat = L1_BUS_LATENCY + L1_HIT_LATENCY + l1_mr * (L2_BUS_LATENCY + L2_HIT_LATENCY + l2_mr * MEM_HIT_LATENCY);

  printf("Global stats:\n");
  printf("  Total request   :     %d\n", total_request);
  printf("  Total time      :     %d\n", total_time);
  printf("  Miss number     :     %d\n", total_request - total_hit);
  printf("  Miss rate       :     %f\n", (double)(total_request - total_hit) / total_request);
  printf("  AMAT            :     %f (cycles)\n", amat);

  printf("L1 Cache stats:\n");
  printf("  Access counter  :     %d\n", l1_stats.access_counter);
  printf("  Access time     :     %d\n", l1_stats.access_time);
  printf("  Miss number     :     %d\n", l1_stats.miss_num);
  printf("  Miss rate       :     %f\n", (double) l1_stats.miss_num / l1_stats.access_counter);
  printf("  Replace number  :     %d\n", l1_stats.replace_num);
  printf("  Prefetch number :     %d\n", l1_stats.prefetch_num);

  printf("L2 Cache stats:\n");
  printf("  Access counter  :     %d\n", l2_stats.access_counter);
  printf("  Access time     :     %d\n", l2_stats.access_time);
  printf("  Miss number     :     %d\n", l2_stats.miss_num);
  printf("  Miss rate       :     %f\n", (double) l2_stats.miss_num / l2_stats.access_counter);
  printf("  Replace number  :     %d\n", l2_stats.replace_num);
  printf("  Prefetch number :     %d\n", l2_stats.prefetch_num);

  printf("Memory stats:\n");
  printf("  Access counter  :     %d\n", mem_stats.access_counter);
  printf("  Access time     :     %d\n", mem_stats.access_time);
}

int main(int argc, char *argv[]) {
  parse_args(argc, argv);
  init_cache();
  handle_trace();
  print_stats();
  return 0;
}
