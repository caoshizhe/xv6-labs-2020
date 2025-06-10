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


// 物理地址转 index
#define PA2IDX(p) (((uint64)(p)) / PGSIZE)

struct {
  struct spinlock lock; // 并发安全锁
  int ref_arr[PHYSTOP / PGSIZE]; // 物理页面的引用次数
} page_ref; // 

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_ref.lock, "pageref"); // 初始化并发锁
  freerange(end, (void*)PHYSTOP);
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
    
  acquire(&page_ref.lock);

  // Fill with junk to catch dangling refs.
  if (--page_ref.ref_arr[PA2IDX(pa)]<=0){
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&page_ref.lock);
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

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    page_ref.ref_arr[PA2IDX(r)] = 1;
  }
  return (void*)r;

}

// 
void *ktry_pgclone(void *pa) {

    acquire(&page_ref.lock);

    // 
    if(page_ref.ref_arr[PA2IDX(pa)] <= 1) {
        release(&page_ref.lock);
        return pa;
    }

    // 
    uint64 newpa = (uint64)kalloc();

    if(newpa == 0) {
        release(&page_ref.lock);
        return 0;
    }

    // 
    memmove((void*)newpa, (void*)pa, PGSIZE);

    // 
    page_ref.ref_arr[PA2IDX(pa)]--;

    release(&page_ref.lock);
    return (void*)newpa;
}

// 
void kparef_inc(void *pa) {
    acquire(&page_ref.lock);
    page_ref.ref_arr[PA2IDX(pa)]++;
    release(&page_ref.lock);
}