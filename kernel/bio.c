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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

// struct {
//   struct spinlock lock;
//   struct buf mem;
// } freelist;

struct {
  struct spinlock lock;
  struct buf head;
  struct buf tail;
} lru;

struct frame {
  uint used;
  uint dev;
  uint blockno;
  uint frame_id;
  struct frame *prev;
  struct frame *next;
} frames[NBUF];

struct spinlock frame_lock;

struct bucket {
  struct spinlock lock;
  struct frame head;
};

struct {
  struct bucket entries[NENTRY];
} hashtable;

void add2front(struct buf *b) {
  acquire(&lru.lock);
  b->prev->next = b->next;
  b->next->prev = b->prev;
  b->next = lru.head.next;
  b->prev = &lru.head;
  lru.head.next->prev = b;
  lru.head.next = b;
  release(&lru.lock);
}


int allocateframe() {
  acquire(&frame_lock);
  int i;
  for (i = 0; i < NBUF; i++) {
    if (frames[i].used == 0) {
      frames[i].used = 1;
      break;
    }
  }
  release(&frame_lock);
  // printf("allocateframe %d\n", i);
  return i;
}

void deleteentry(uint dev, uint blockno) {
  int pos = (dev + blockno) % NENTRY;
  struct frame *f = hashtable.entries[pos].head.next;
  int found = 0;
  while (f) {
    if (f->dev == dev && f->blockno == blockno) {
      found = 1;
      break;
    }
    f = f->next;
  }
  if (!found) {
    return;
  }
  f->prev->next = f->next;
  if (f->next) {
    f->next->prev = f->prev;
  }
  acquire(&frame_lock);
  f->prev = 0;
  f->next = 0;
  f->used = 0;
  // printf("delete frame %d dev %d block %d\n", f - frames, bcache.buf[f->frame_id].dev, bcache.buf[f->frame_id].blockno);
  release(&frame_lock);
}

void add2hashtable(struct buf *b) {
  // printf("add2hashtable dev %d blockno %d\n", b->dev, b->blockno);
  int pos = (b->dev + b->blockno) % NENTRY;
  struct frame *f = hashtable.entries[pos].head.next;
  int exist = 0;
  while (f) {
    if (f->dev == b->dev && f->blockno == b->blockno) {
      exist = 1;
      break;
    }
    f = f->next;
  }
  if (exist) {
    return;
  }
  int i = allocateframe();
  frames[i].dev = b->dev;
  frames[i].blockno = b->blockno;
  frames[i].frame_id = b - bcache.buf;

  frames[i].next = hashtable.entries[pos].head.next;
  frames[i].prev = &hashtable.entries[pos].head;
  if (hashtable.entries[pos].head.next) {
    hashtable.entries[pos].head.next->prev = &frames[i];
  }
  hashtable.entries[pos].head.next = &frames[i];
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  initlock(&lru.lock, "lru");
  initlock(&frame_lock, "frames");

  // 1. lru init
  lru.head.next = &lru.tail;
  lru.head.prev = 0;
  lru.tail.prev = &lru.head;
  lru.tail.next = 0;
  printf("lru.head %p, lru.tail %p\n", &lru.head, &lru.tail);
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = lru.head.next;
    b->prev = &lru.head;
    lru.head.next->prev = b;
    lru.head.next = b;
    initsleeplock(&b->lock, "buffer");
  }
  // 2. hashtable init
  for (int i = 0; i < NENTRY; i++) {
    hashtable.entries[i].head.prev = 0;
    hashtable.entries[i].head.next = 0;
    // printf("hashtable.entris[i].head.prev %p, next %p\n", hashtable.entries[i].head.prev, hashtable.entries[i].head.next);
    initlock(&hashtable.entries[i].lock, "entry");
  }

  // 3. frame init
  for (int i = 0; i < NBUF; i++) {
    frames[i].used = 0;
  }
  // printf("init complete\n");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  // printf("cpu %d bget\n", cpuid());
  int pos = (dev + blockno) % NENTRY;
  acquire(&hashtable.entries[pos].lock);

  // 1. is the block already cached ?
  int found = 0;
  struct frame *e = hashtable.entries[pos].head.next;
  while (e) {
    if (e->dev == dev && e->blockno == blockno) {
      found = 1;
      break;
    }
    e = e->next;
    // printf("here1\n");
  }
  if (found) {
    b = &bcache.buf[e->frame_id];
    b->refcnt++;
    add2front(b);
    release(&hashtable.entries[pos].lock);
    acquiresleep(&b->lock);
    return b;
  }
  // 2. evict a block
  acquire(&lru.lock);
  b = lru.tail.prev;
  // printf("b %p lru.head %p\n", b, &lru.head);
  while (b != &lru.head) {
    if (b->refcnt == 0) {
      deleteentry(b->dev, b->blockno);
      b->dev =  dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      found = 1; 
      break;
    }
    // printf("b %p lru.head %p\n", b, &lru.head);
    b = b->prev;
  }
  release(&lru.lock);
  if (found) {
    add2front(b);
    add2hashtable(b);
    release(&hashtable.entries[pos].lock);
    acquiresleep(&b->lock);
    return b;
  }
  release(&hashtable.entries[pos].lock);
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
  if(!holdingsleep(&b->lock)) {
    // printf("dev %d,  blockno %d", b->dev, b->blockno);
    panic("brelse");
  }
  releasesleep(&b->lock);
  int pos = (b->dev + b->blockno) % NENTRY;
  acquire(&hashtable.entries[pos].lock);
  b->refcnt--;
  release(&hashtable.entries[pos].lock);
}

void
bpin(struct buf *b) {
  int pos = (b->dev + b->blockno) % NENTRY;
  acquire(&hashtable.entries[pos].lock);
  b->refcnt++;
  release(&hashtable.entries[pos].lock);
}

void
bunpin(struct buf *b) {
  int pos = (b->dev + b->blockno) % NENTRY;
  acquire(&hashtable.entries[pos].lock);
  b->refcnt--;
  release(&hashtable.entries[pos].lock);
}


