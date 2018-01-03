/*Using System Calls Fork() and Execl() and Make file*/

/*HEADER FILES */

#include<stdio.h>
#include<unistd.h> //for pid_t data type,read and write
#include<string.h> // include for string compare and string length functions
#include<stdlib.h>


int main(int argc, char *argv[])
{

pid_t pid;
int fd[2]; //file descriptor to simulate one way communication of process thro
           //pipe
int len;
char string1[1024];

if( argc > 2 )
{
	printf("\nToo many arguments\n");
	return -1;
}
else if( argc < 2)
{
	printf("\nLess arguments than required\n");
	return -1;
}

pipe(fd);
pid = fork(); //fork returns value of type pid_t
if (pid < 0) /*Fork returned -ve value so fork failed*/
{
		printf("Process creation failed\n");
		return -1;
}
else if(pid == 0) /*Fork returns zero indicating it is in child process*/ 
{
                //getpid() returns process ID 
                printf("\n Child PID:%d\n",getpid());
                close(fd[1]); //close writing side

                read(fd[0],&string1,1024);	         
		close(fd[0]);//close read side

		if(strcmp(string1,"/bin/date") == 0)
		execl("/bin/date", "date", 0, 0);
		else if(strcmp(string1,"/bin/uname") == 0)
		execl("/bin/uname","uname",0,0);
		else if(strcmp(string1,"/bin/hostname") == 0)
		execl("/bin/hostname","hostname", 0 , 0 );
		else if(strcmp(string1,"/bin/echo") == 0)
		execl("/bin/echo","echo","Hello World!",0);
                else if(strcmp(string1,"echo")==0)
		execl("/bin/echo","echo","Hello World!",0);			
		else
                printf("%s",string1); //print whatever passed on cmd line arg       

}
else /*parent process*/
{
		 printf("\n Parent PID : %d\n",getpid());
                 close(fd[0]); //close read side
               //write takes 3 args.File descriptor,data to be written 
	       //and length of data
                 len = strlen(argv[1]); 
                 argv[1][len] = 0;    
		 write(fd[1],argv[1],len);                
                 close(fd[1]); //close writing side of pipe
                 wait(NULL);//parent waits for child process to complete
}


}
