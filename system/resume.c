/* resume.c - resume */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  resume  -  Unsuspend a process, making it ready
 *------------------------------------------------------------------------
 */
pri16	resume(
	  pid32		pid		/* ID of process to unsuspend	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process' table entry	*/
	pri16	prio;			/* Priority to return		*/

	mask = disable();
	if (isbadpid(pid)) {
		restore(mask);
		return (pri16)SYSERR;
	}
	prptr = &proctab[pid];
	if (prptr->prstate != PR_SUSP) {
		restore(mask);
		return (pri16)SYSERR;
	}
//kprintf("\nPriority of process  %s before ready:%d\n",prptr->prname,prptr->prprio);
	prio = prptr->prprio;		/* Record priority to return	*/
	ready(pid);
	//prio = prptr->prprio;		/* Record priority to return	*/	
	//kprintf("\nPriority of process %s after ready:%d\n",prptr->prname,prptr->prprio);
	restore(mask);
	return prio;
}
