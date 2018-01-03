/*function defining ring processes here*/

#include<process_ring.h>


volatile int round_num = 0;

process counterTrack_Polling(int processID,int process_count,int rounds_count){	
	
	volatile int j = 0;

	while(j < rounds_count){
		
			     	if(inbox[processID] != -1) // check whether it has admissible value 
				{		
				printf("\nRing Element %d : Round %d : Value %d \n", processID, j, inbox[processID]);
				exit_value++; // variable used to keep alive parent process till all child finish execution
				inbox[(processID+1) % process_count] = inbox[processID] - 1; // update neighbour process inbox value 
				j++;				 		
				inbox[processID] = -1; // mark -1 to avoid printing same value
			
		}
						
}

return SHELL_OK;
}


void sync(volatile int processID,volatile int process_count, volatile int rounds_count){

        volatile int j = 0;

                        while(j < rounds_count){
                                wait(demo_sem[processID]); /* wait for the signal from the previous process that it has updated inbox value
							     except for zeroth process whose inbox value is updated by parent								*/
                                printf("\nRing Element %d : Round %d : Value %d \n", processID, j, inbox[processID]);
                                inbox[(processID+1) % process_count] =  inbox[processID]-1;
                                signal(demo_sem[(processID+1)%process_count]);// signal neighbouring process that inbox value is updated
                                j++;

                                }
signal(exit_sem); // this semaphore is used to keep parent process alive till all child finish 
}

