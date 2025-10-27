// Physical memory allocator for xv6
// Each CPU has its own freelist and lock to reduce contention.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern char end[]; // first address after kernel, defined by kernel.ld

struct run {
  struct run *next;
};

// Per-CPU memory allocator
struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];       // One freelist per CPU
static char kmem_names[NCPU][8]; // Persistent lock names

// Helper: format lock name "kmem0", "kmem1", ...
static void
format_kmem_name(char *buf, int id)
{
  buf[0] = 'k'; buf[1] = 'm'; buf[2] = 'e'; buf[3] = 'm';
  if (id < 10) {
    buf[4] = '0' + id;
    buf[5] = 0;
  } else if (id < 100) {
    buf[4] = '0' + (id / 10);
    buf[5] = '0' + (id % 10);
    buf[6] = 0;
  } else {
    buf[4] = 0;
  }
}

// Forward declarations
void freerange(void *pa_start, void *pa_end);
void kfree(void *pa);
void *kalloc(void);

// Initialize memory allocator
void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    format_kmem_name(kmem_names[i], i);
    initlock(&kmems[i].lock, kmem_names[i]);
    kmems[i].freelist = 0;
  }
  freerange(end, (void*)PHYSTOP);
}

// Free a range of physical memory
void
freerange(void *pa_start, void *pa_end)
{
  char *p = (char*)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free one 4KB page of physical memory
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  memset(pa, 1, PGSIZE); // fill with junk
  r = (struct run*)pa;

  push_off();             // disable interrupts
  int id = cpuid();       // current CPU
  pop_off();

  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  release(&kmems[id].lock);
}

// Allocate one 4KB page of physical memory
void *
kalloc(void)
{
  struct run *r = 0;

  push_off();
  int id = cpuid();
  pop_off();

  // 1. Try current CPU freelist first
  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if (r)
    kmems[id].freelist = r->next;
  release(&kmems[id].lock);

  // 2. If empty, try to steal from other CPUs
  if (!r) {
    for (int i = 0; i < NCPU; i++) {
      if (i == id) continue;
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if (r) {
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      }
      release(&kmems[i].lock);
    }
  }

  if (r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}
