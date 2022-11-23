/**
 * lru_replacer.h
 *
 * Functionality: The buffer pool manager must maintain a LRU list to collect
 * all the pages that are unpinned and ready to be swapped. The simplest way to
 * implement LRU is a FIFO queue, but remember to dequeue or enqueue pages when
 * a page changes from unpinned to pinned, or vice-versa.
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <list>
#include <iterator>
#include "buffer/replacer.h"
#include "hash/extendible_hash.h"

using namespace std;
namespace scudb {

template <typename T> class LRUReplacer : public Replacer<T> {
struct CacheNode{
  /* data */
  CacheNode() {};
  CacheNode(T v) : value(v) {};
  T value;
};

public:

  LRUReplacer();

  ~LRUReplacer();

  void Insert(const T &value);

  bool Victim(T &value);

  bool Erase(const T &value);

  size_t Size();

private:
  // add your member variables here
  std::list<std::shared_ptr<CacheNode>> lrulist;
  std::unordered_map<T, std::shared_ptr<CacheNode>> lrumap;
  // mutable make tablelatch can use in const
  mutable std::mutex latch;
};

} // namespace scudb
