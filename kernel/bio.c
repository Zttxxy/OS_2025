// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  
  // 每个哈希桶一个链表头（双向循环链表）
  struct buf head[NBUCKET];
} bcache;

// 哈希函数
static uint
hash(uint blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  // 初始化所有桶的锁
  for(int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache");
    // 初始化每个桶的空双向循环链表
    bcache.head[i].next = &bcache.head[i];
    bcache.head[i].prev = &bcache.head[i];
  }

  // 将所有buf分配到桶0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    // 插入到桶0的链表头部
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint idx = hash(blockno);

  acquire(&bcache.lock[idx]);

  // Is the block already cached?
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // First try to find in current bucket (从尾部向前找，LRU)
  for(b = bcache.head[idx].prev; b != &bcache.head[idx]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // No available buffer in current bucket, try other buckets
  release(&bcache.lock[idx]);

  for(int i = 0; i < NBUCKET; i++) {
    if(i == idx) continue;
    
    acquire(&bcache.lock[i]);
    
    // Look for unused buffer in this bucket (从尾部向前找，LRU)
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        // Remove from current bucket
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.lock[i]);
        
        // Add to target bucket at head (MRU position)
        acquire(&bcache.lock[idx]);
        b->next = bcache.head[idx].next;
        b->prev = &bcache.head[idx];
        bcache.head[idx].next->prev = b;
        bcache.head[idx].next = b;
        
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.lock[idx]);
        
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
  
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint idx = hash(b->blockno);

  acquire(&bcache.lock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    // Move to head of bucket for LRU (MRU position)
    // Remove from current position
    b->next->prev = b->prev;
    b->prev->next = b->next;
    
    // Insert at head (MRU position)
    b->next = bcache.head[idx].next;
    b->prev = &bcache.head[idx];
    bcache.head[idx].next->prev = b;
    bcache.head[idx].next = b;
  }
  
  release(&bcache.lock[idx]);
}

void
bpin(struct buf *b) {
  uint idx = hash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt++;
  release(&bcache.lock[idx]);
}

void
bunpin(struct buf *b) {
  uint idx = hash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  release(&bcache.lock[idx]);
}