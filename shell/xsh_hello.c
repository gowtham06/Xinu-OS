/*Implementation of Command 'Hello'*/
/*Syntax: 
	xsh$ hello <string>
Output 
	Hello <string>, Welcome to the world of Xinu!!*/

/*Header Files*/

#include<xinu.h>
#include<string.h>
#include<stdio.h>


void p3(uint16 pid)
{
chprio(pid,40);
}
void p2()
{
uint16 pid;
pid = getpid();
 resume(create(p3,1024,30,"p3",1,pid));	
}
void p1()
{ 
 resume(create(p2,1024,28,"p2",0));
}

/*Implementation of hello command*/
shellcmd xsh_hello(int nargs, char *args[]) {
            
      if(nargs == 2){
      	    resume(create(p1,1024,24,"p1",0));        			         	   	
  				printf("\nHello %s ,Welcome to the World of Xinu!!\n", args[1]); //argv[1] is first argument		     
   }		 
       
	else if (nargs > 2) {
		printf("Too many arguments\n");		
	}

	else {
		printf(" Less no. of arguments\n");
	}

}

