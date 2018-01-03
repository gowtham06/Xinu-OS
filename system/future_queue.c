#include <xinu.h>
#include <future.h>

/*New queue function for future*/
fqueue future_newqueue(void)
{
	fqueue head = (fqueue)getmem(sizeof(struct fentry));
	head->pid = MAXKEY;
	head->fprev = NULL;
	fqueue tail = (fqueue)getmem(sizeof(struct fentry));
	tail->pid = MINKEY;
	tail->fnext = NULL;
	head->fnext = tail;
	tail->fprev = head;
	return head;
}


int future_isempty(fqueue head)
{
//head next's pid is Minkey that means empty queue
	if(head->fnext->pid == MINKEY)
	{
		return 1;
	}
	return 0;
}

//Insert at last
void future_enqueue(fqueue head, pid32 pid)
{
	
	fqueue node = (fqueue)getmem(sizeof(struct fentry));
	node->pid = pid;
	fqueue tail = head;
	while(tail->fnext->pid != MINKEY)
	{
		tail = tail->fnext;
	}	
//Insert new node at last
	node->fnext = tail->fnext;
	node->fprev = tail;
	tail->fnext->fprev = node;
	tail->fnext = node;
}

pid32 future_dequeue(fqueue head)
{
	
	if(future_isempty(head)) return SYSERR;
	fqueue node = head->fnext;
	pid32 pid = node->pid;
	node->fnext->fprev = head;
	head->fnext = node->fnext;
	freemem(node,(sizeof(struct fentry)));
	return pid;
}

