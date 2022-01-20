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

#define num_bucket  13
#define  bucket_size NBUF/num_bucket;

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct spinlock bucket_locks[num_bucket];
} bcache;

char lock_names[num_bucket][16];

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i< num_bucket; ++i) {
    char *name = strncpy(lock_names[i], "bcache.bucket", 16);
    initlock(&bcache.bucket_locks[i], name);
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->t_last_use = 0;
  }
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  //acquire(&bcache.lock);
  
  uint bucket_id = blockno % num_bucket;
  // Is the block already cached?
  uint start_idx = bucket_id * bucket_size;
  uint end_idx = (bucket_id == (num_bucket - 1)) ? NBUF : (bucket_id + 1) * bucket_size;
  acquire(&bcache.bucket_locks[bucket_id]);
  for(int idx = start_idx; idx < end_idx; ++idx) {
    b = &bcache.buf[idx];
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
  	  release(&bcache.bucket_locks[bucket_id]);
      //release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint min_t = ticks;
  int min_idx = -1;
  for(int idx = start_idx; idx < end_idx; ++idx) {
    b = &bcache.buf[idx];
    if(b->refcnt == 0 && min_t > b->t_last_use) {
 	  min_idx = idx;
      min_t = b->t_last_use;
    }
  }

  //printf("blockno:%d bucket_id:%d start_idx:%d end_idx:%d min_t:%d min_idx:%d\n", blockno, bucket_id, start_idx, end_idx, min_t, min_idx);
  if (min_idx != -1) {
    b = &bcache.buf[min_idx];
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
  	release(&bcache.bucket_locks[bucket_id]);
    //release(&bcache.lock);
    acquiresleep(&b->lock);
    return b;
  }
  panic("bget: no buffers");
}

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  //acquire(&bcache.lock);
  int bucket_id = b->blockno % num_bucket;
  acquire(&bcache.bucket_locks[bucket_id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->t_last_use = ticks;
  }
  
  release(&bcache.bucket_locks[bucket_id]);
  //release(&bcache.lock);
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


void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


