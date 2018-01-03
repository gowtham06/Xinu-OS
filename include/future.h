#include<xinu.h>

#ifndef _FUTURE_H_
#define _FUTURE_H_  

//queue declaration
struct fentry{		
	pid32 pid;
	int value;		
	struct fentry *fnext;
	struct fentry *fprev;		
};

typedef struct fentry* fqueue;

typedef enum {
  FUTURE_EMPTY,
  FUTURE_WAITING,
  FUTURE_READY
} future_state_t;

typedef enum {
  FUTURE_EXCLUSIVE,
  FUTURE_SHARED,
  FUTURE_QUEUE
} future_mode_t;


typedef struct future{
  int value;
  future_state_t state;
  future_mode_t mode;
  pid32 pid;
fqueue set_queue;
fqueue get_queue;
}future_t;



//Queue functions
fqueue future_newqueue(void);
int future_isempty(fqueue);
void future_enqueue(fqueue, pid32);
pid32 future_dequeue(fqueue);


/* Interface for the Futures system calls */
future_t* future_alloc(future_mode_t mode);
syscall future_free(future_t*);
syscall future_get(future_t*, int*);
syscall future_set(future_t*, int);


uint future_prod(future_t*,int);
uint future_cons(future_t*);
 
#endif /* _FUTURE_H_ */
