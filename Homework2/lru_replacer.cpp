/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace scudb {

template <typename T> LRUReplacer<T>::LRUReplacer() {}

template <typename T> LRUReplacer<T>::~LRUReplacer() {}


/*
 * Insert value into LRU
 */
template <typename T> void LRUReplacer<T>::Insert(const T &value) {
  std::lock_guard<std::mutex> lck(latch);
// situation:linked list already had value
  std::shared_ptr<CacheNode> temp;
  //find() will return iterator pointing to the end of lrumap if not found
  if(lrumap.find(value) != lrumap.end()){ 
    temp = lrumap[value];
    //remove the node from its current position
    lrulist.remove(temp);
  }else{
    temp = std::make_shared<CacheNode>(value);
  }
  lrulist.push_front(temp);
  lrumap[value] = temp;
  return;
}

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
template <typename T> bool LRUReplacer<T>::Victim(T &value) {
  std::lock_guard<std::mutex> lck(latch);
  if(lrumap.empty()){
    return false;
  }
  // remove least rencently used
  std::shared_ptr<CacheNode> temp;
  // choose tail of list
  temp = lrulist.back();
  lrumap.erase(temp->value);
  lrulist.pop_back();
  value = temp->value;
  return true;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
  std::lock_guard<std::mutex> lck(latch);
  if(lrumap.find(value) != lrumap.end()){
    std::shared_ptr<CacheNode> temp = lrumap[value];
    lrulist.remove(temp);
  }
  return lrumap.erase(value);
}

template <typename T> size_t LRUReplacer<T>::Size() {
  std::lock_guard<std::mutex> lck(latch);
  return lrumap.size();
}

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace scudb
