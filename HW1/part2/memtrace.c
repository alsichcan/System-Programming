//------------------------------------------------------------------------------
//
// memtrace
//
// trace calls to the dynamic memory manager
//
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memlog.h>
#include <memlist.h>

//
// function pointers to stdlib's memory management functions
//
static void *(*mallocp)(size_t size) = NULL;
static void (*freep)(void *ptr) = NULL;
static void *(*callocp)(size_t nmemb, size_t size);
static void *(*reallocp)(void *ptr, size_t size);

//
// statistics & other global variables
//
static unsigned long n_malloc  = 0;
static unsigned long n_calloc  = 0;
static unsigned long n_realloc = 0;
static unsigned long n_allocb  = 0;
static unsigned long n_freeb   = 0;
static item *list = NULL;

//
// Self-made method
//
static void nonfreed_list(item* list);

//
// init - this function is called once when the shared library is loaded
//
__attribute__((constructor))
void init(void)
{
  char *error;

  LOG_START();

  // initialize a new list to keep track of all memory (de-)allocations
  // (not needed for part 1)
  list = new_list();

  // ...
}

//
// fini - this function is called once when the shared library is unloaded
//
__attribute__((destructor))
void fini(void)
{
  unsigned long n_alloc = n_malloc + n_calloc + n_realloc;

  LOG_STATISTICS(n_allocb, (n_allocb/n_alloc), n_freeb);

  nonfreed_list(list);

  LOG_STOP();

  // free list (not needed for part 1)
  free_list(list);
}

// ...
void *malloc(size_t size){
  char *error;
  void *ptr;

  if (!mallocp){
    mallocp = dlsym(RTLD_NEXT, "malloc");
    if((error = dlerror()) != NULL){
      fputs(error, stderr);
      exit(1);
    }
  }

  ptr = mallocp(size);
  n_malloc += 1;

  alloc(list, ptr, size);
  n_allocb += size;

  LOG_MALLOC(size, ptr);

  return ptr;
}

void free(void* ptr){
  char *error;

  if (!freep){
    freep = dlsym(RTLD_NEXT, "free");
    if((error = dlerror()) != NULL){
      fputs(error, stderr);
      exit(1);
    }
  }

  freep(ptr);

  item* i = dealloc(list, ptr);
  n_freeb += i->size;
  
  LOG_FREE(ptr);
}

void *calloc(size_t nmemb, size_t size){
  char *error;
  void *ptr;

  if (!callocp){
    callocp = dlsym(RTLD_NEXT, "calloc");
    if((error = dlerror()) != NULL){
      fputs(error, stderr);
      exit(1);
    }
  }

  ptr = callocp(nmemb, size);
  n_calloc += 1;

  item* i = alloc(list, ptr, nmemb * size);
  n_allocb += i->size;

  LOG_CALLOC(nmemb, size, ptr);
  
  return ptr;
}

void *realloc(void *ptr, size_t size){
  char *error;
  void *rptr;

  if (!reallocp){
    reallocp = dlsym(RTLD_NEXT, "realloc");
    if((error = dlerror()) != NULL){
      fputs(error, stderr);
      exit(1);
    }
  } 

  rptr = reallocp(ptr, size);
  n_realloc += 1;

  item* i1 = dealloc(list, ptr);
  n_freeb += i1->size;
  item* i2 = alloc(list, rptr, size);
  n_allocb += i2->size;

  LOG_REALLOC(ptr, size, rptr);

  return rptr;
}

void nonfreed_list(item* list){
  int num = 0;
  char *error;

  if (!freep){
    freep = dlsym(RTLD_NEXT, "free");
    if((error = dlerror()) != NULL){
      fputs(error, stderr);
      exit(1);
    }
  }

  item *prev, *cur, *i;
  if(list == NULL) return;
  
  prev = list;
  cur = list->next;

  while(cur != NULL){
    if(cur->cnt > 0){
      if(num == 0) LOG_NONFREED_START();
      LOG_BLOCK(cur->ptr, cur->size, cur->cnt);
      num++;
    }
    prev = cur;
    cur = cur->next;
  }

  return;
}