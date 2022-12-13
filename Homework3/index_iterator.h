/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "page/b_plus_tree_leaf_page.h"

namespace scudb {

#define INDEXITERATOR_TYPE                                                     \
  IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
public:
  // you may define your own constructor based on your member variables
  IndexIterator(B_PLUS_TREE_LEAF_PAGE_TYPE *leaf_page, int index, BufferPoolManager *bufferPoolManager);
  ~IndexIterator();

  bool isEnd(){
    return(leafPage == nullptr);
  }

  const MappingType &operator*(){
    return leafPage->GetItem(Index);
  }

  IndexIterator &operator++(){
    Index++;
    if (Index >= leafPage->GetSize()) {
      page_id_t next = leafPage->GetNextPageId();
      UnlockAndUnPin();
      if (next == INVALID_PAGE_ID) {
        leafPage = nullptr;
      } else {
        Page *page = bufferPoolManager->FetchPage(next);
        page->RLatch();
        leafPage = reinterpret_cast<B_PLUS_TREE_LEAF_PAGE_TYPE *>(page->GetData());
        Index = 0;
      }
    }
    return *this;
  }

private:
  // add your own private member variables here
  void UnlockAndUnPin() {
    bufferPoolManager->FetchPage(leafPage->GetPageId())->RUnlatch();
    bufferPoolManager->UnpinPage(leafPage->GetPageId(), false);
    bufferPoolManager->UnpinPage(leafPage->GetPageId(), false);
  }
  int Index;
  B_PLUS_TREE_LEAF_PAGE_TYPE *leafPage;
  BufferPoolManager *bufferPoolManager;
};

} // namespace scudb
