#ifndef NACHOS_SEM_TABLE_H
#define NACHOS_SEM_TABLE_H

#include "bitmap.h"
#include "syscall.h"
#include "copyright.h"

#define MAX_SEM 20 //Cantidad maxima de semaforos
#define SIZE 256

class NachosSemTable {
  public:
    NachosSemTable();       // Initialize
    ~NachosSemTable();      // De-allocate

    int CreateSem( long sem ); // Register the file handle
    int DestroySem( int id );      // Unregister the file handle
    bool exists( int id ); // Search in the table if there's a semaphore with ID = id.
	  long getSem (int id); // Returns the semaphore with ID = id.
    void addThread();		// If a user thread is using this table, add it
    void delThread();		// If a user thread is using this table, delete it
	  int getUsage();
   	int findAvailable(); // Returns the position id of an available semaphore in the table.
  	void initThreadSemaphore(long sem);
    void print();
  	int waitingThreads; // Number of threads in queue.

  private:
    long * semaphores;		// A vector with user opened semaphores
    BitMap * semaphoreMap;	// A bitmap to control our vector
    int usage;			// How many threads are using this table

};

#endif //NACHOS_SEM_TABLE_H