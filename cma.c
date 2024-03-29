#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "debug.h"
#include "cma.h"

#define ITEMNOTFOUND ((void *)-1)

static void *class_membase=NULL;
static void *class_limit=NULL;
static MNode class_inuse=NULL;
static MNode class_nouse=NULL;

static struct {
  unsigned long malloc,calloc,realloc,free,gc,nomem;
} class_counters={0,0,0,0,0,0};

static MNode class_AddToList(MNode list, MNode item) {
  ENTER;
  item->next = list;
  RETURN(item);
}

static MNode class_RemoveFromList(MNode list,MNode item) {
  ENTER;

  MNode p,prev;

  prev = NULL;
  for (p = list; p!=NULL; p = p->next) {
    if (p == item) {
      if (prev == NULL)
list=p->next;
      else
prev->next = p->next;
      RETURN(list);
    }
    prev=p;
  }
  //not in list..
  RETURN(ITEMNOTFOUND);
}


static void class_printList(MNode list) {
  ENTER;
  if (!list) {
EXIT;
    return;
  }
  printf("Node %p, %ud\n",list,list->size);
  class_printList(list->next);
  EXIT;
}

int class_memory(void *mem, size_t size) {
  ENTER;
  MNode item;
  if (class_membase ) {
    RETURN(FALSE);
  }

  class_membase = mem;
  class_limit = mem+size;
  
  //setup initial list item
  item = (MNode)mem;
  item->size=size-sizeof(struct MemNode);
  item->next = NULL;
  
  DEBUG("Creating Initial NoUseList with %x: %ud",item,size); 
  class_nouse = class_AddToList(class_nouse,item);
  EXIT;
}

void *class_calloc(size_t nmemb, size_t size) {
  ENTER;
  void *mem;
  class_counters.calloc++;

  mem = class_malloc(nmemb*size);
  memset(mem,0,nmemb*size);
  RETURN(mem);
}

static MNode class_findNoUse(size_t target) {
  ENTER;
  size_t closeness=LONG_MAX;
  size_t c = 0;
  MNode best=NULL;
  MNode p=NULL;
  DEBUG("Searching for a block of size: %ud",target);
  for (p=class_nouse;p!=NULL;p=p->next) {
    c = p->size - target;
    if (c >= 0 && c<closeness) {
      best = p;
      closeness=c;
      DEBUG("Best is now: %x size=%ud",best,p->size);
	}
  }
  RETURN(best);
}

MNode class_splitNode(MNode org,size_t size) {
	ENTER;
	MNode extra=NULL;
	size_t orgsz = org->size;

	//we need room for a new header
	if ( (orgsz-size-sizeof(struct MemNode)) > 0 ) {
		DEBUG("Node split: %ud => %ud,%ud",org->size,size,orgsz-sizeof(struct MemNode)-size);
		org->size = size;
		extra = (MNode)((void*)org+size+sizeof(struct MemNode));
		extra->next = 0;
		extra->size = orgsz-sizeof(struct MemNode)-size;
	}
	else
		DEBUG("Node does not have enough size to split:%ud %ud",org->size,size);

	RETURN(extra);
}

void *class_malloc(size_t size) {
  ENTER;
  MNode newnode,extra;

  class_counters.malloc++;
  newnode = class_findNoUse(size);

  if (newnode) {
    class_nouse=class_RemoveFromList(class_nouse,newnode);//assume it is there since we just found it there

    //split the node..
    extra = class_splitNode(newnode,size);
    if (extra)
      class_nouse=class_AddToList(class_nouse,extra);
    
    newnode->next = NULL;
    class_inuse = class_AddToList(class_inuse,newnode);
    RETURN((void *)newnode+sizeof(struct MemNode));
  }
  else {
    class_counters.nomem++;
    RETURN(NULL);
  }
}


//attempt to find adjacent unused nodes and collapse them.
/* track two MNodes here and there
set here to start of no_use list
until no more postenders are found do
  traverse linked list from here->next to end using there as pointer
    if here+here->size is the same as there you found a post-ender
       change here->next to there->next
       increase here->size by sizeof there, and size of there's header
       add one to the garbage collect counter
       break out of traversal loop
  end traverse
end until
*/
static void class_garbage() {
        int count = 0;
		MNode here = class_nouse;
		MNode there = here->next;		
		while(TRUE) {
        unsigned int startCount = count;
        
//        for(MNode here=class_nouse; here; here = here->next ) {
            for(there=here->next; there; there = there->next) {
                    if(NXT(here) == there) {
                    	here->next = there->next;
                        here->size += sizeof(*there) + there->size;
                        printf("Collapsing! -- Garbage Count = %d\n", ++count);
                    }
            } 
//        } 

        // If inside loop did nothing stop searching. 
        if(startCount == count) { 
            if(count)
                printf("Garbage Final Count = %d\n", count);
            break;
        }
    }
}


void class_free(void *ptr) {
  ENTER;
  MNode cur=NULL;
  if (!ptr) {
EXIT;
    return;
  }

  class_counters.free++;
  cur=class_RemoveFromList(class_inuse,PTRTOMNODE(ptr));
  if (cur==ITEMNOTFOUND) {//not our pointer
EXIT;
    return;
  }
  class_inuse = cur;
  class_nouse = class_AddToList(class_nouse,PTRTOMNODE(ptr));
  class_garbage();
  EXIT;
}

void *class_realloc(void *ptr, size_t size) {
  ENTER;
  void *mem;
  size_t oldsize;

  class_counters.realloc++;
  mem=class_malloc(size);
  if (!mem) {
EXIT;
return;
  }
    
  oldsize=PTRTOMNODE(ptr)->size;
  memcpy(mem,ptr,oldsize);

  class_free(ptr);
  RETURN(mem);
}

void class_stats() {
  ENTER;
  printf("InUse\n");
  class_printList(class_inuse);

  printf("NoUse\n");
  class_printList(class_nouse);

  printf("Counters:\n");
#define DUMPC(x) printf(" %10s : %ld\n",#x,class_counters.x)
  DUMPC(malloc);
  DUMPC(calloc);
  DUMPC(realloc);
  DUMPC(free);
  DUMPC(gc);
  DUMPC(nomem);
#undef DUMPC
  EXIT
}
