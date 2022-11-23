#include <list>

#include "hash/extendible_hash.h"
#include "page/page.h"
using namespace std;

namespace scudb {

/*
 * constructor
 * array_size: fixed array size for each bucket
 */
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size) : globalDepth(0),buketSize(size),bucketNum(1){
  BucketTable.push_back(std::make_shared<Bucket>(0));
}
// constructor with no param
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(){
  ExtendibleHash(64);
}
/*
 * helper function to calculate the hashing address of input key
 */
template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) const {
  // return hashvalue of key in type of K
  return std::hash<K>{}(key);
}

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetGlobalDepth() const {
  std::lock_guard<std::mutex> lock(tableLatch);
  return globalDepth;
}

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
  if(!BucketTable[bucket_id])
    return -1;
  std::lock_guard<std::mutex> lck(BucketTable[bucket_id]->latch);
  if(BucketTable[bucket_id]->items.empty()) return -1;
  return BucketTable[bucket_id]->localDepth;
}

/*
 * helper function to return current number of bucket in hash table
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const {
  std::lock_guard<mutex> lock(tableLatch);
  return bucketNum;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::getBucketIndex(const K &key) const {
  std::lock_guard<std::mutex> lck(tableLatch);
  return HashKey(key) & ((1 << globalDepth)-1);
}

/*
 * lookup function to find value associate with input key
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
  int index = getBucketIndex(key);
  std::lock_guard<std::mutex> lck(BucketTable[index]->latch);
  std::shared_ptr<Bucket> bucket = BucketTable[index];
  //find() will return iterator pointing to the end of map if not found
  if(bucket != nullptr && bucket->items.find(key)!= bucket->items.end()){
    value = bucket->items[key];
    return true;
  }
  
  return false;
}

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {
  int index = getBucketIndex(key);
  std::lock_guard<std::mutex> lck(BucketTable[index]->latch);
  std::shared_ptr<Bucket> cur = BucketTable[index];
  if(cur->items.find(key)!=cur->items.end()){
    cur->items.erase(key);
    return true;
  }
  return false;
}

/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
  int index = getBucketIndex(key);
  std::shared_ptr<Bucket> cur = BucketTable[index];
  while (true) {
    std::lock_guard<std::mutex> lck(cur->latch);
    //no need to expansion
    if (cur->items.find(key) != cur->items.end() || cur->items.size() < buketSize) {
      cur->items[key] = value;
      break;
    }
    // Bucket splitting
    int mask = (1 << (cur->localDepth));
    cur->localDepth++;

    {
      lock_guard<mutex> lck2(tableLatch);
      // BucketTable dilatation
      if (cur->localDepth > globalDepth) {

        size_t length = BucketTable.size();
        for (size_t i = 0; i < length; i++) {
          BucketTable.push_back(BucketTable[i]);
        } 
        globalDepth++;

      }
      bucketNum++;
      auto newBuc = make_shared<Bucket>(cur->localDepth);

      typename map<K, V>::iterator it;
      for (it = cur->items.begin(); it != cur->items.end(); ) {
        if (HashKey(it->first) & mask) {
          newBuc->items[it->first] = it->second;
          it = cur->items.erase(it);
        } else it++;
      }
      for (size_t i = 0; i < BucketTable.size(); i++) {
        if (BucketTable[i] == cur && (i & mask))
          BucketTable[i] = newBuc;
      }
    }
    index = getBucketIndex(key);
    cur = BucketTable[index];
  }
}

template class ExtendibleHash<page_id_t, Page *>;
template class ExtendibleHash<Page *, std::list<Page *>::iterator>;
// test purpose
template class ExtendibleHash<int, std::string>;
template class ExtendibleHash<int, std::list<int>::iterator>;
template class ExtendibleHash<int, int>;
} // namespace scudb
