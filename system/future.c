#include <xinu.h>
#include <future.h>

/*FUTURE GET implementation*/
syscall future_get(future_t *f, int *value)
{
	/*IF future is already NULL return Error*/
	if(f == NULL || (f->state == FUTURE_WAITING && f->mode == FUTURE_EXCLUSIVE))
	{
	 return SYSERR;
	}
	
	else if(f->state == FUTURE_READY && (f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED))
	{
	 *value =  f->value;
	 f->state = FUTURE_EMPTY;
	}
	/*Empty and mode is exclusive then it should wait*/
	else if(f->state == FUTURE_EMPTY && f->mode == FUTURE_EXCLUSIVE)
	{
	 f->pid = getpid();
         f->state = FUTURE_WAITING;
         suspend(f->pid);
	 *value = f->value;
	 f->state = FUTURE_EMPTY;
	}
	/*Shared Mode*/
	else if((f->state == FUTURE_WAITING || f->state == FUTURE_EMPTY) && f->mode == FUTURE_SHARED)
	{
		pid32 pid = getpid();
		future_enqueue(f->get_queue, pid);
		f->state = FUTURE_WAITING;
		suspend(pid);
		*value = f->value;
		if(future_isempty(f->get_queue)) f->state = FUTURE_EMPTY;
	}
	/*Mode is Future Queue*/
	else if(f->mode == FUTURE_QUEUE)
	{
		/*Future Empty then enqueue*/
		if(future_isempty(f->set_queue))
		{
			pid32 pid = getpid();
			future_enqueue(f->get_queue, pid);
			f->state = FUTURE_WAITING;
			suspend(pid);
			*value = f->value;
			if(future_isempty(f->get_queue)) f->state = FUTURE_EMPTY;
		}
		/*Future not empty then deque*/
		else
		{
			resume(dequeue(f->set_queue));
			*value = f->value;
			if(future_isempty(f->get_queue)) f->state = FUTURE_EMPTY;
		}
	}
	return OK;
}


syscall future_set(future_t *f, int value)
{
	/*Future already set if state is FUTURE_READY then return NULL*/
	if(f == NULL || (f->state == FUTURE_READY && (f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED)))
	{
	 return SYSERR;
	}
	/*Make future ready if empty*/	
	else if(f->state == FUTURE_EMPTY && (f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED))
	{
	 f->value = value; 
         f->state = FUTURE_READY;
	}
	else if(f->state == FUTURE_WAITING && (f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED))
	{
	 f->value = value;
	 f->state = FUTURE_READY;
		if(f->mode == FUTURE_EXCLUSIVE)
		{
			resume(f->pid);
		}
		else if(f->mode == FUTURE_SHARED)
		{
			while(!future_isempty(f->get_queue))
			{
				resume(future_dequeue(f->get_queue));
			}
		}
	}
	/*Mode is Future_Queue*/
	else if(f->mode == FUTURE_QUEUE)
	{
		/*Future state empty then enqueue*/
		if(f->state == FUTURE_EMPTY)
		{
			if(future_isempty(f->get_queue))
			{
				pid32 pid = getpid();
				future_enqueue(f->set_queue ,pid);
				suspend(pid);
				f->value = value;
			}
		}
		else if(f->state == FUTURE_WAITING)
		{
			f->value = value;
			resume(future_dequeue(f->get_queue));
		}
	}
	return OK;
}


future_t* future_alloc(future_mode_t mode)
{
	future_t* f = (future_t*)getmem(sizeof(future_t));
	if(f == SYSERR){
		return NULL;
	}
	f->mode = mode;
	f->state = FUTURE_EMPTY;
/*Future Queue needs both set_queue and get_queue*/
	if(mode == FUTURE_QUEUE)
	{
		f->set_queue = future_newqueue();
		f->get_queue = future_newqueue();
	}
/*Future Shared needs only get_queue*/
	else if(mode == FUTURE_SHARED)
	{
		f->set_queue = NULL;
		f->get_queue = future_newqueue();
	}
/*Future Exclusive doesn't need set_queue and get_queue*/
	else
	{
		f->set_queue = NULL;
		f->get_queue = NULL;
	}
	return f;	
}

/*Free the set_queue and get_queue allocated to future*/
syscall future_free(struct future *f)
{
	if(f->get_queue != NULL)
	{
          freemem(f->get_queue, sizeof(struct fentry));
        }
	if(f->set_queue != NULL)
	{
	  freemem(f->set_queue, sizeof(struct fentry));
	}
	return freemem(f, sizeof(future_t));
}
