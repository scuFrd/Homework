#include "buffer/buffer_pool_manager.h"

namespace scudb {

/*
 * BufferPoolManager Constructor
 * When log_manager is nullptr, logging is disabled (for test purpose)
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::BufferPoolManager(size_t pool_size,
                                                 DiskManager *disk_manager,
                                                 LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager),
      log_manager_(log_manager) {
  // a consecutive memory space for buffer pool
  pages_ = new Page[pool_size_];
  page_table_ = new ExtendibleHash<page_id_t, Page *>(BUCKET_SIZE);
  replacer_ = new LRUReplacer<Page *>;
  free_list_ = new std::list<Page *>;

  // put all the pages into free list
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_->push_back(&pages_[i]);
  }
}

/*
 * BufferPoolManager Deconstructor
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
  delete free_list_;
}

Page *BufferPoolManager::GetVictimPage(){
  Page *tar = nullptr;
  // find from free_list_ first
  if(free_list_->empty()){
    // if free_list_ is empty then find from replacer_
    if(replacer_->Size() == 0){
      // return nullptr if both two are empty
      return nullptr;
    }
    // replacer_ is not empty then choose victim page from replacer_
    replacer_->Victim(tar);
  }else{  //free_list_ has empty page object
    tar = free_list_->front();
    free_list_->pop_front();
    // make sure that tar is a free page object
    assert(tar->GetPageId() == INVALID_PAGE_ID);
  }
  // make sure that tar is a unpinned page object
  assert(tar->GetPinCount() == 0);
  return tar;
}

/**
 * 1. search hash table.
 *  1.1 if exist, pin the page and return immediately
 *  1.2 if no exist, find a replacement entry from either free list or lru
 *      replacer. (NOTE: always find from free list first)
 * 2. If the entry chosen for replacement is dirty, write it back to disk.
 * 3. Delete the entry for the old page from the hash table and insert an
 * entry for the new page.
 * 4. Update page metadata, read page content from disk file and return page
 * pointer
 */
Page *BufferPoolManager::FetchPage(page_id_t page_id) {
  lock_guard<mutex> lck(latch_);
  Page *tar = nullptr;
  // 1. search hash table.
  if(page_table_->Find(page_id,tar)){ //1.1 if exist
    tar->pin_count_++;
    replacer_->Erase(tar);
    return tar;
  }
  // 1.2 if no exist
  tar = GetVictimPage(); //find from free_list_ first
  if(tar == nullptr) return tar; //no need to write back
  // 2. If the entry chosen for replacement is dirty, write it back to disk
  if(tar->is_dirty_){
    disk_manager_->WritePage(tar->GetPageId(),tar->data_);
  }
  // 3.delete old page
  page_table_->Remove(tar->GetPageId());
  page_table_->Insert(page_id,tar);
  // 4.
  disk_manager_->ReadPage(page_id,tar->data_);
  tar->pin_count_ = 1;
  tar->is_dirty_ = false;
  tar->page_id_= page_id;

  return tar;
}

/*
 * Implementation of unpin page
 * if pin_count>0, decrement it and if it becomes zero, put it back to
 * replacer if pin_count<=0 before this call, return false. is_dirty: set the
 * dirty flag of this page
 */
bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
  lock_guard<mutex> lck(latch_);
  Page *tar = nullptr;
  // If there is no entry in the page table for the given page_id, then return false
  page_table_->Find(page_id,tar);
  if(tar == nullptr){
    return false;
  }
  tar->is_dirty_=is_dirty;
  if(tar->GetPinCount() <= 0 ){
    return false;
  }
  if(--tar->pin_count_ == 0){
    replacer_->Insert(tar);
  }
  return true;
}

/*
 * Used to flush a particular page of the buffer pool to disk. Should call the
 * write_page method of the disk manager
 * if page is not found in page table, return false
 * NOTE: make sure page_id != INVALID_PAGE_ID
 */
bool BufferPoolManager::FlushPage(page_id_t page_id) {
  lock_guard<mutex> lck(latch_);
  Page *tar = nullptr;
  page_table_->Find(page_id,tar);
  // don't have page or page_id is invalid
  if(tar == nullptr || tar->page_id_ == INVALID_PAGE_ID){
    return false;
  }
  // if(is_dirty) then write back 
  if(tar->is_dirty_){
    disk_manager_->WritePage(page_id,tar->GetData());
    tar->is_dirty_ = false;
  }
  return true;
}

/**
 * User should call this method for deleting a page. This routine will call
 * disk manager to deallocate the page. First, if page is found within page
 * table, buffer pool manager should be reponsible for removing this entry out
 * of page table, reseting page metadata and adding back to free list. Second,
 * call disk manager's DeallocatePage() method to delete from disk file. If
 * the page is found within page table, but pin_count != 0, return false
 */
bool BufferPoolManager::DeletePage(page_id_t page_id) {
  lock_guard<mutex> lck(latch_);
  Page *tar = nullptr;
  page_table_->Find(page_id,tar);
  if(tar != nullptr){
    // page can be deleted only when pincount = 0
    if(tar->GetPinCount() > 0){
      return false;
    }
    // delete records
    replacer_->Erase(tar);
    page_table_->Remove(page_id);
    tar->is_dirty_= false;
    tar->ResetMemory();
    // reclaim
    free_list_->push_back(tar);
  }
  disk_manager_->DeallocatePage(page_id);
  return true;
}


// void BufferPoolManager::FlushAllPages(){
//   lock_guard<mutex> lck(latch_);
//   Page *tar = nullptr;
//   // for(size_t i = 0; i < pool_size_; i++) {
//   //   disk_manager_->WritePage(page_table_[i]);
//   // }
//   for_each(page_id_t page_id_temp in page_table_.keys){
//     page_table_.Find(page_id_temp,tar);
//     disk_manager_.WritePage(page_id_temp,tar.GetData());
//   }
// }

/**
 * User should call this method if needs to create a new page. This routine
 * will call disk manager to allocate a page.
 * Buffer pool manager should be responsible to choose a victim page either
 * from free list or lru replacer(NOTE: always choose from free list first),
 * update new page's metadata, zero out memory and add corresponding entry
 * into page table. return nullptr if all the pages in pool are pinned
 */
Page *BufferPoolManager::NewPage(page_id_t &page_id) {
  lock_guard<mutex> lck(latch_);
  Page *tar = nullptr;
  tar = GetVictimPage();
  if (tar == nullptr) return tar;
  // allocate page_id
  page_id = disk_manager_->AllocatePage();
  //write back
  if (tar->is_dirty_) {
    disk_manager_->WritePage(tar->GetPageId(),tar->data_);
  }
  //remove old record
  page_table_->Remove(tar->GetPageId());
  // insert new record
  page_table_->Insert(page_id,tar);

  tar->page_id_ = page_id;
  tar->ResetMemory();
  tar->is_dirty_ = false;
  tar->pin_count_ = 1;

  return tar;
}

} // namespace scudb
