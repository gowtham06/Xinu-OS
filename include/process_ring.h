#ifndef PROCESS_RING
#define PROCESS_RING
#include<xinu.h>
#include<stdio.h>
#include<stdlib.h>
#include<prototypes.h>
extern volatile int inbox[64];
extern volatile int exit_value;
sid32 poll_value;
sid32 demo_sem[64];
sid32 exit_sem;
extern process counterTrack_Polling(int,int,int);
extern void sync(int,int,int);
#endif
