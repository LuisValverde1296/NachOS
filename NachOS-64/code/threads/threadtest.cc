// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//

#include <unistd.h>
#include "copyright.h"
#include "system.h"
#include "dinningph.h"
#include "santa.h"

DinningPh * dp;
Santa* st;

void Philo( void * p ) {

    int eats, thinks;
    long who = (long) p;

    currentThread->Yield();

    for ( int i = 0; i < 10; i++ ) {

        printf(" Philosopher %ld will try to pickup sticks\n", who + 1);

        dp->pickup( who );
        dp->print();
        eats = Random() % 6;

        currentThread->Yield();
        sleep( eats );

        dp->putdown( who );

        thinks = Random() % 6;
        currentThread->Yield();
        sleep( thinks );
    }

}

void Call_Santa( void* p){
    int rest, works, taller;
    long goblin = (long)p;
    char* name = new char[25];
    currentThread->Yield();

    for (int i = 0; i< 3; ++i){
        switch(goblin){
            case 0:
                strcpy(name, "Tom");
                break;
            case 1:
                strcpy(name, "Jerry");
                break;
            case 2:
                strcpy(name, "Gruñon");
                break;
            case 3:
                strcpy(name, "Miedoso");
                break;
            default:
                break;
        }
        printf("***** %s intenta trabajar *****\n", name);
        taller = st->slave( goblin );
        st->print();
        works = Random() % 6;

        currentThread->Yield();
        sleep( works );

        st->procrastinate( goblin, taller);

        rest = Random() % 6;
        currentThread->Yield();
        sleep( rest );
    }
}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

void
SimpleThread(void* name)
{
    // Reinterpret arg "name" as a string
    char* threadName = (char*)name;
    
    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
    for (int num = 0; num < 10; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	printf("*** thread %s looped %d times\n", threadName, num);
	//interrupt->SetLevel(oldLevel);
        //currentThread->Yield();
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> Thread %s has finished\n", threadName);
    //interrupt->SetLevel(oldLevel);
}



//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling 
//	SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    Thread * Ph;

    DEBUG('t', "Entering SimpleTest");

    st = new Santa();
    for(int k = 0; k < 4; ++k){
        Ph = new Thread("st");
        Ph->Fork( Call_Santa, (void*) k );
    }

    return;
    /*dp = new DinningPh();

    for ( long k = 0; k < 5; k++ ) {
        Ph = new Thread( "dp" );
        Ph->Fork( Philo, (void *) k );
    }

    return;

    for ( int k=1; k<=5; k++) {
      char* threadname = new char[100];
      sprintf(threadname, "Hilo %d", k);
      Thread* newThread = new Thread (threadname);
      newThread->Fork (SimpleThread, (void*)threadname);
    }
    
    SimpleThread( (void*)"Hilo 0");*/
}

