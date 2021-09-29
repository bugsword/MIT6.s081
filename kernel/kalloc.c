// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

uint64 pg_counter;

uint64 
pg_counter_add_uint64(uint64 index, int value)
{
  uint64* tmp;
  tmp = (uint64*)pg_counter;
  return *(tmp - index) += value;
}

int 
pg_counter_add(uint64 index, int value)
{
  int* tmp;
  //index -= KERNBASE/4096;
  tmp = (int*)pg_counter;
  return *(tmp - index) += value;
}

uint64 
pg_counter_sub(uint64 index)
{
  uint64* tmp;
  tmp = (uint64*)pg_counter;
  *(tmp - index) -= 1;
  return *(tmp - index);
}

int is_kinit = 0;
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  //uint64 pg_counter_size = PGROUNDUP(PHYSTOP) / 4096 * 8; // /4096得到页表数量，每个页表的计数器使用uint64来计数，所以*8得到存储pg_counter需要内存大小；
  //uint64 pg_counter_size = (PHYSTOP - KERNBASE) / 4096 * 4; // /4096得到页表数量，每个页表的计数器使用uint64来计数，所以*8得到存储pg_counter需要内存大小；
  uint64 pg_counter_size = PHYSTOP / 4096 * 4; // /4096得到页表数量，每个页表的计数器使用uint64来计数，所以*8得到存储pg_counter需要内存大小；
  pg_counter = PHYSTOP - 1;
  freerange(end, (void*)(PHYSTOP - 1 - pg_counter_size));
  is_kinit = 1;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  if (is_kinit == 1) {
    if(pg_counter_add((uint64)pa/4096, -1) >= 1)
    //printf("test:%d\n", pg_counter_add((uint64)pa/4096, 0));
      return;
  }
  //printf("test:%d\n", pg_counter_add((uint64)pa/4096, 0));
  /*
  {
    printf("ignore");
    //非首次初始化且引用计数值大于等于1，则不进行free，直接返回
	return;
  }
  */
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
	int* tmp;
	tmp = (int*)pg_counter;
	//*(tmp - ((uint64)r - KERNBASE)/4096) = 1;
	*(tmp - (uint64)r/4096) = 1;
  }
  return (void*)r;
}
