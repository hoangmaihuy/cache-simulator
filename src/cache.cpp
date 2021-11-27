#include <cstdlib>
#include <cassert>
#include <math.h>
#include "cache.hpp"

extern bool verbose;

bool is_power_of_two(uint64_t x) {
  return x && !(x & (x - 1));
}

uint64_t get_bits(uint64_t x, unsigned int lo, unsigned int hi) {
  return x << (63 - hi) >> (63 - hi + lo);
}

bool Cache::SetConfig(CacheConfig cc) {
  // Check if config is valid
  if (!is_power_of_two(cc.size)) return false;
  if (!is_power_of_two(cc.block_size)) return false;
  if (!is_power_of_two(cc.associativity)) return false;
  if (cc.block_size * cc.associativity >= cc.size) return false;
  cc.set_num = cc.size / (cc.block_size * cc.associativity);
  if (!is_power_of_two(cc.set_num)) return false;

  config_ = cc;

  s = log2(config_.set_num);
  b = log2(config_.block_size);
  t = ADDR_LEN - s - b;

  sets = vector<CacheSet>(cc.set_num);
  for (int i = 0; i < cc.set_num; i++) {
    sets[i].lines = vector<CacheLine>(cc.associativity);
    for (int j = 0; j < cc.associativity; j++) {
      CacheLine &line = sets[i].lines[j];
      line.access_counter = 0;
      line.valid = false;
      line.dirty = false;
      line.blocks = vector<char>(cc.block_size);
    }
  }
  return true;
}

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
  assert(bytes > 0 && bytes <= config_.block_size);
  if (verbose)
    fprintf(stderr, "cache handle: addr = 0x%llx, size = %d\n", addr, bytes);
  stats_.access_counter++;
  hit = 0;
  time = 0;
  uint64_t set_idx, tag, block_offset;
  int line_idx;
  int lower_hit = 0, lower_time = 0;

  // Bypass?
  if (!BypassDecision()) {
    PartitionAlgorithm(addr, set_idx, tag, block_offset);
    assert(block_offset + bytes <= config_.block_size);
    line_idx = GetLine(set_idx, tag);
    if (ReplaceDecision(line_idx, read)) {
      // Choose victim
      line_idx = ReplaceAlgorithm(set_idx, time);
    } else {
      // return hit & time
      if (read) { // read hit
        ReadRequest(set_idx, line_idx, block_offset, bytes, content);
      } else { // write hit
        WriteRequest(set_idx, line_idx, block_offset, tag, bytes, content, !config_.write_through);
        if (config_.write_through) { // write through
          lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
        }
      }
      hit = 1;
      time += latency_.bus_latency + latency_.hit_latency + lower_time;
      stats_.access_time += latency_.bus_latency + latency_.hit_latency;
      return;
    }
  }
  // Prefetch?
  if (PrefetchDecision()) {
    PrefetchAlgorithm();
  } else {
    // Fetch from lower layer
    hit = 0;
    stats_.miss_num++;
    if (read) {
      auto lower_addr = addr & ~((uint64_t) config_.block_size - 1);
      lower_->HandleRequest(lower_addr, config_.block_size, read, content, lower_hit, lower_time);
    } else {
      lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
    }
    // Replacement
    if (line_idx != -1 && (read || config_.write_allocate)) {
      stats_.fetch_num++;
      WriteRequest(set_idx, line_idx, 0, tag, config_.block_size, content, false);
      time += latency_.bus_latency + latency_.hit_latency + lower_time;
      stats_.access_time += latency_.bus_latency + latency_.hit_latency;
    } else {
      time += latency_.bus_latency + lower_time;
      stats_.access_time += latency_.bus_latency;
    }
  }
}

void Cache::ReadRequest(uint64_t set_idx, uint64_t line_idx, uint64_t block_offset, int bytes, char *content) {
  auto &line = sets[set_idx].lines[line_idx];
  assert(line.valid);
  for (uint64_t i = block_offset; i < block_offset + bytes; i++) {
    content[i] = line.blocks[i];
  }
  line.access_counter = stats_.access_counter;
}

void
Cache::WriteRequest(uint64_t set_idx, uint64_t line_idx, uint64_t block_offset, uint64_t tag, int bytes, char *content,
                    bool dirty) {
  if (verbose) {
    fprintf(stderr, "cache write: set = %lld, line = %lld, offset = %lld, tag = %lld\n", set_idx, line_idx,
            block_offset, tag);
  }
  auto &line = sets[set_idx].lines[line_idx];
  for (uint64_t i = block_offset; i < block_offset + bytes; i++) {
    line.blocks[i] = content[i];
  }
  line.access_counter = stats_.access_counter;
  line.tag = tag;
  line.valid = true;
  line.dirty = dirty;
}

bool Cache::BypassDecision() {
  return false;
}

void Cache::PartitionAlgorithm(uint64_t addr, uint64_t &set_idx, uint64_t &tag, uint64_t &block_offset) {

  block_offset = get_bits(addr, 0, b - 1);
  set_idx = get_bits(addr, b, b + s - 1);
  tag = get_bits(addr, b + s, ADDR_LEN - 1);

}

int Cache::GetLine(uint64_t set_idx, uint64_t tag) {

  auto &lines = sets[set_idx].lines;

  for (int i = 0; i < config_.associativity; i++) {
    if (lines[i].valid && lines[i].tag == tag)
      return i;
  }

  return -1;
}

bool Cache::ReplaceDecision(int line_idx, int read) {
  return line_idx == -1;
}

int Cache::ReplaceAlgorithm(uint64_t set_idx, int &time) {
  int line_idx = -1;
  char *buf = static_cast<char *>(malloc(sizeof(char) * config_.block_size));
  auto &lines = sets[set_idx].lines;
  for (int i = 0; i < lines.size(); i++) {
    if (!lines[i].valid) {
      line_idx = i;
      break;
    };
    if (line_idx == -1 || lines[i].access_counter < lines[line_idx].access_counter)
      line_idx = i;
  }

  auto &line = lines[line_idx];
  stats_.replace_num++;

  // Write back to lower layer
  if (line.valid && line.dirty) {
    uint64_t addr = (line.tag << (s + b)) | (set_idx << b);
    for (int i = 0; i < config_.block_size; i++)
      buf[i] = line.blocks[i];
    int lower_hit, lower_time;
    lower_->HandleRequest(addr, config_.block_size, 0, buf, lower_hit, lower_time);
    time += lower_time;
  }

  free(buf);
  return line_idx;
}

bool Cache::PrefetchDecision() {
  return false;
}

void Cache::PrefetchAlgorithm() {
}

