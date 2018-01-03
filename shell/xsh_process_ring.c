#include<process_ring.h>

volatile int inbox[64];
volatile int exit_value;
sid32 demo_sem[64];
sid32 exit_sem;
sid32 poll_value;

shellcmd xsh_process_ring(int argc, char *args[]){
    int process_count = 2; /* specified default */
    int rounds_count = 3; /*specified default */       
    int i; 
    uint32 start_time,end_time,time_diff;
	int flag = 0;
	for(i = 1; i < argc; i++)
	{
        if((strncmp("-p", args[i],3)) == 0 ){
		if(!(i+1 < argc))
		{
			printf("\n -p flag expected an argument\n");
			return SHELL_ERROR;	
		}
            // parse numeric  argument passed to -p flag 	
	      	process_count  = atoi(args[i+1]);
		 if(process_count < 0)
	        {
        	 printf("\n-p flag  expected a positive integer\n");
           	return SHELL_ERROR;

        	}
 
	    if (!(0 <= process_count && process_count <= 64)){
               printf("\n-p flag  expected a number between 0-64\n");
               return SHELL_ERROR;
            }	
             i += 1; // Skip the numeric argument 
        } else {
         // handle other optional arguments
	if((strncmp("-r", args[i],3)) == 0){
	if(!(i+1 < argc))
                {
                        printf("\n -r flag expected an argument\n");
                        return SHELL_ERROR;
                }

            // parse numeric argument to -r 
            rounds_count = atoi(args[i+1]);
	if(rounds_count < 0)
	{
	 printf("\n-r flag  expected a positive integer\n");
           return SHELL_ERROR;

	}
	if (!(0 <= rounds_count &&
                  rounds_count <= 100)){
               printf("\n-r flag  expected a number between 0-100\n");
               return SHELL_ERROR;
	}
		  i += 1; // Skip the numeric argument

	}        
	if((strncmp("--help",args[i],7)) == 0){
	printf("\nThe number of processes -p <0-64> default 2");
	printf("\nThe number of rounds -r <uint32> default 3.");
	printf("\nThe implementation -i <poll or sync> default poll.");
	printf("\nExample Usage : process_ring -p 64 -r 1 -i poll\n");
	return SHELL_OK;
	}

	if((strncmp("-i",args[i],3)) == 0){	
		if(strncmp(args[i+1],"poll",5) == 0)
		flag = 0;
		if(strncmp(args[i+1],"sync",5) == 0)
		flag = 1;	
	}
	}
}

    // setting process 0th inbox to the counter value
     inbox[0] = rounds_count * process_count - 1;
     exit_value = 0;
      for(i=0;i<process_count;i++){
        demo_sem[i] = semcreate(0);
       }


printf("\n Number of Processes: %d",process_count);
printf("\n Number of Rounds: %d", rounds_count);

if(flag == 0)
printf("\n Type : POLL\n");
else
printf("\n Type : Sync\n");

gettime(&start_time);
//create processes
for(i=0;i < process_count;i++)
{
	if(flag == 0) /*POLLING*/
		resume(create(counterTrack_Polling,1024,20,"counterTrack",3,i,process_count,rounds_count));
	else	/*Synchronization VIA Semaphore */
	{		
		if(i==0)
		signal(demo_sem[i]); //0th process inbox value is updated by parent so signal is given 		         	  
		resume(create(sync,1024,20,"counterTrack",4,i,process_count,rounds_count));
	}
}

if(flag == 1)
{
	for(i=0;i<process_count;i++)
	{
		wait(exit_sem);
		semdelete(demo_sem[i]);
	
	}
}
else
{
while(exit_value != ((rounds_count * process_count)));
}

gettime(&end_time);
time_diff = end_time - start_time;
printf("\nElapsed Time:%d\n",time_diff);

return SHELL_OK;
}
