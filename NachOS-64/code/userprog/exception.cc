// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "nachostabla.h"
#include "nachossemtabla.h"
#include "syscall.h"
#include "synch.h"
#include "system.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

Semaphore* Console = new Semaphore("Console", 1);

void returnFromSystemCall() {

    int pc, npc;

    pc = machine->ReadRegister( PCReg );
    npc = machine->ReadRegister( NextPCReg );
    machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
    machine->WriteRegister( PCReg, npc );           // PC <- NextPC
    machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4
}       // returnFromSystemCall


void Nachos_Halt() {                    // System call 0

   DEBUG('a', "Shutdown, initiated by user program.\n");
   interrupt->Halt();

}       // Nachos_Halt

void Nachos_Exit(){
   if(joinable[currentThread->id]){
      int semid = semIds[currentThread->id];
      Semaphore* tmp = (Semaphore*)semJoin->getSem(semid);
      tmp->V();
   }
   currentThread->Finish();
   returnFromSystemCall();
}

void Nachos_Execute(void* filePath){
   currentThread->space = new AddrSpace((OpenFile*)filePath);

   currentThread->space->InitRegisters();
   currentThread->space->RestoreState();

   machine->WriteRegister(RetAddrReg, 4);

   machine->Run();
   ASSERT(false); //Si no salta al codigo del usuario hace un ASSERT
}

void Nachos_Exec(){
   ASSERT(execJoinSemMap->NumClear() > 0);

   int register_4 = machine->ReadRegister(4);
   int actual = -1;
   int index = 0;
   char* buffer = new char[SIZE];

   while(actual != 0){
      machine->ReadMem(register_4, 1, &actual);
      buffer[index] = (char) actual;
      ++register_4;
      ++index;
   }
   Thread *t1 = new Thread("Child");
   int id = execJoinSemMap->Find();
   t1->id = id;
   t1->Fork(Nachos_Execute, (void*)fileSystem->Open(buffer));
   machine->WriteRegister(2,id);

   //return returnFromSystemCall();
}

void Nachos_Join(){
   int register_4 = machine->ReadRegister(4);
   int id = register_4;
   if(execJoinSemMap->Test(id)){
      joinable[id] = true;
      int semaphoreId = semJoin->CreateSem(0);
      semIds[id] = semaphoreId;
      Semaphore* tmp = (Semaphore*) semJoin->getSem(semaphoreId);
      tmp->P();
      semJoin->DestroySem(semaphoreId);
      joinable[id] = false;
      execJoinSemMap->Clear(id);
   } else{
      ASSERT(false);
   }
}

void Nachos_Create() {
   int register_4 = machine->ReadRegister(4);
   char* buffer = new char[SIZE];
   int actual = -1;
   int index = 0;
   int result = -1;

   while( actual != 0 ){
      machine->ReadMem(register_4, 1, &actual);
      buffer[index] = (char) actual;
      ++register_4;
      ++index;
   }
   result = creat(buffer, 0766);
   if(result != -1){
      result = close(result);
      ASSERT(result != -1);
   }
   returnFromSystemCall();
}

void Nachos_Open() {                    // System call 5
/* System call definition described to user
	int Open(
		char *name	// Register 4
	);
*/
	// Read the name from the user memory, see 5 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors

    
   int register_4 = machine->ReadRegister(4); // Keeps the tracking of the content of the 4th register.
   int actual = -1; //The char I'm currently at.
   int index = 0; //The index is used to advance over the register's content.
   char *path = new char[SIZE]; //Saves the path of the file
   int result = -1; // Keeps the result returned by the table

   while (0 != actual) { //Reads the filename, EOF = 0.
      machine->ReadMem(register_4, 1, &actual);
      path[index] = (char) actual;
      register_4++;
      index++;
   }

   int ID = open(path, O_RDWR); //Linux open
   // Verify for errors

   ASSERT(ID != -1); // test to see if the file was open, if not: breaks the program.
   result = currentThread->nachosTabla->Open(ID);
   machine->WriteRegister(2, result);
   ++stats->numDiskReads; //Stats up to date.
   delete path;
   returnFromSystemCall();		// Update the PC registers
}       // Nachos_Open

void Nachos_Read(){
   char* buffer = NULL;
   int size = machine->ReadRegister(5);

   buffer = new char[size];
   int register_4 = machine->ReadRegister(5);
   int actual = -1;
   int result = -1;
   int unixHandle = -1;
   OpenFileId id = machine->ReadRegister(6);

   bool error = false;
   Console->P();
   switch(id){
      case  ConsoleInput:	// User can read from standard input
			result = read(id, buffer, size);
			stats->numConsoleCharsRead++;
			machine->WriteRegister(2, result);
			break;
		case  ConsoleOutput:
			machine->WriteRegister(2, result);
			printf( "Error: user can not read from output\n" );
         error = true;
		break;
		case ConsoleError:
			printf( "Error: user can not read from console error\n" );
         error = true;
			break;
		default:
         if(currentThread->nachosTabla->isOpened(id)){
            unixHandle = currentThread->nachosTabla->getUnixHandle(id);
            result = read(unixHandle, buffer, size);
            ++stats->numDiskReads;
         }
         machine->WriteRegister(2, result);
   }
   Console->V();

   if(!error){ //if there wasn't a reading error
      actual = (int)buffer[0];
      machine->WriteMem(register_4, 1, actual);
      ++register_4;
   }

	delete buffer;

	returnFromSystemCall();
}

void Nachos_Write() {                   // System call 7

/* System call definition described to user
      void Write(
		char *buffer,	// Register 4
		int size,	// Register 5
		OpenFileId id	// Register 6
	);
*/

   char * buffer = NULL;
   int size = machine->ReadRegister(5);	// Read size to write

   buffer = new char[size]; 
   int register_4 = machine->ReadRegister(4); //Tracking of the 4th register's content
   int actual = -1; //Actual character reading.
   int result = -1; //return value.

   for(int i = 0; i < size; ++i){
      machine->ReadMem(register_4, 1, &actual);
      buffer[i] = (char)actual;
      ++register_4;
   }
   // buffer = Read data from address given by user;
   OpenFileId id = machine->ReadRegister( 6 );	// Read file descriptor

	// Need a semaphore to synchronize access to console
	Console->P();
	switch (id) {
		case  ConsoleInput:	// User could not write to standard input
			machine->WriteRegister( 2, result );
			break;
		case  ConsoleOutput:
			buffer[ size ] = 0;
			printf( "%s", buffer );
		   break;
		case ConsoleError:	// This trick permits to write integers to console
			printf( "%d\n", machine->ReadRegister( 4 ) );
			break;
		default:	// All other opened files
			// Verify if the file is opened, if not return -1 in r2
			// Get the unix handle from our table for open files
			// Do the write to the already opened Unix file
			// Return the number of chars written to user, via r2
         if(currentThread->nachosTabla->isOpened(id)){
            int unixHandle = currentThread->nachosTabla->getUnixHandle(id);
            result = write(unixHandle, (void*)buffer, size); //unix write(int fd, const void *buf, size_t count);
            ++stats->numDiskWrites;
         }
         machine->WriteRegister( 2, result );
			break;

	}
   delete buffer;
	// Update simulation stats, see details in Statistics class in machine/stats.cc
   Console->V();
   returnFromSystemCall();		// Update the PC registers

}       // Nachos_Write

void Nachos_Close() {
	int register_4 = machine->ReadRegister(4);
   int result = -1;
   if(currentThread->nachosTabla->isOpened(register_4)){
      result = close(currentThread->nachosTabla->getUnixHandle(register_4));
      ASSERT(result != -1);
      currentThread->nachosTabla->Close(register_4);
   }
   returnFromSystemCall();
}

void Nachos_Child_Fork(void* p){
   AddrSpace* space;
   
   space = currentThread->space;
   space->InitRegisters();
   space->RestoreState();

   machine->WriteRegister(RetAddrReg,4);
   machine->WriteRegister(PCReg, (long)p);
   machine->WriteRegister(NextPCReg, (long)p + 4);

   machine->Run(); //Run the userprog
   ASSERT(false);
}

void Nachos_Fork() {
   Thread *t1 = new Thread("Child Fork");
// Given the fact that the child and the father threads must share
// the same space, we're going to give to this new Thread the same
// space as the father using a copy costructor.
   t1->space = new AddrSpace(currentThread->space);

// The parameters for the new Thread are given in the register 4
   t1->Fork(Nachos_Child_Fork, (void*)machine->ReadRegister(4));

   returnFromSystemCall();
}

void Nachos_Yield() {
	currentThread->Yield();
	returnFromSystemCall();
}

void Nachos_SemCreate() {
   int register_4 = machine->ReadRegister(4);
   int id = -1;

	char* buffer = new char[SIZE];
   sprintf(buffer, "Sem %d", currentThread->nachosSemTabla->findAvailable());
	if((id = currentThread->nachosSemTabla->CreateSem((long)new Semaphore(buffer, register_4))) == -1){
      printf("Unable to create a new Semaphore\n");
   }
	machine->WriteRegister(2, id);
	returnFromSystemCall();
}

void Nachos_SemDestroy() {
   int register_4 = machine->ReadRegister(4);
   int result = -1;
	if (currentThread->nachosSemTabla->exists(register_4)) {
		result = currentThread->nachosSemTabla->DestroySem(register_4);
	}
	machine->WriteRegister(2, result);
	returnFromSystemCall();
}

void Nachos_SemSignal() {
   int register_4 = machine->ReadRegister(4);
   int result = -1;
   Semaphore* tmp;
	if (currentThread->nachosSemTabla->exists(register_4)) {
		tmp = (Semaphore*) currentThread->nachosSemTabla->getSem(register_4);
		tmp->V();
		result = 0;
	}
   machine->WriteRegister(2, result);
	returnFromSystemCall();
}

void Nachos_SemWait() {
   int register_4 = machine->ReadRegister(4);
   int result = -1;
   Semaphore* tmp;
	if (currentThread->nachosSemTabla->exists(register_4)) {
		tmp = (Semaphore*) currentThread->nachosSemTabla->getSem(register_4);
		tmp->P();
		result = 0;
	} 
   machine->WriteRegister(2, result);
	returnFromSystemCall();
}

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:
                Nachos_Halt();             // System call # 0
                break;
             case SC_Exit:
                Nachos_Exit();             // System call # 1
                break;
             case SC_Exec:
                Nachos_Exec();             // System call # 2
                break;
             case SC_Join:
                Nachos_Join();             // System call # 3
                break;
             case SC_Create:
                Nachos_Create();           // Systen call # 4
                break;
             case SC_Open:
                Nachos_Open();             // System call # 5
                break;
             case SC_Read:
                Nachos_Read();             // System call # 6
                break;
             case SC_Write:
                Nachos_Write();             // System call # 7
                break;
             case SC_Close:
                Nachos_Close();             // System call # 8
                break;
             case SC_Fork:
                Nachos_Fork();              // System call # 9
                break;
             case SC_Yield:
                Nachos_Yield();             // System call # 10
                break;
             case SC_SemCreate:
                Nachos_SemCreate();         // System call # 11
                break;
             case SC_SemDestroy:
                Nachos_SemDestroy();        // System call # 12
                break;
             case SC_SemSignal:
                Nachos_SemSignal();         // System call # 13
                break;
             case SC_SemWait:
                Nachos_SemWait();           // System call # 14
                break;
             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT(false);
                break;
          }
       break;
       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT(false);
          break;
    }

}

