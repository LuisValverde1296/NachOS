#include "nachossemtabla.h"

NachosSemTable::NachosSemTable() {          //Initialize
	semaphores = new long [MAX_SEM];
	semaphoreMap = new BitMap(MAX_SEM);
	semaphores[0] = 0;
	semaphoreMap->Mark(0);
	usage = 1;
	waitingThreads = 0;
    for(int i = 1; i < MAX_SEM; ++i){
        semaphores[i] = -1;
    }
}

NachosSemTable::~NachosSemTable() {         //De-allocate
	delete semaphores;
	delete semaphoreMap;
}

int NachosSemTable::CreateSem(long sem) {           // Register the file handle
	int result = semaphoreMap->Find();
    ASSERT( result != -1);
	semaphores[result] = sem;
	semaphoreMap -> Mark(result);
	return result;
}

int NachosSemTable::DestroySem(int id) {            // Unregister the file handle
	if (exists(id)) {
		semaphoreMap->Clear(id);
		return id;
	}
	return -1;
}

bool NachosSemTable::exists(int id) {           // Search in the table if there's a semaphore with ID = id.
	return id < MAX_SEM && id >= 0 && semaphoreMap->Test(id);
}


long NachosSemTable::getSem(int id) {           // Returns the semaphore with ID = id.
    return exists(id) ? semaphores[id] : 0;
}


void NachosSemTable::addThread() {
	usage++;
}


void NachosSemTable::delThread() {
	usage--;
}


int NachosSemTable::getUsage() {
	return usage;
}


int NachosSemTable::findAvailable() {           // Returns the position id of an available semaphore in the table.
	int result = semaphoreMap->Find();
	if ( result != -1 ) 
		semaphoreMap->Clear(result);
	return result;
}


void NachosSemTable::initThreadSemaphore(long sem) {
	semaphores[0] = sem;
}

void NachosSemTable::print() {
    printf("Sem Table Pos    Id");
    for(int i = 0; i < SIZE;++i){
        if(exists(i)){
            printf("%-12d : %-14ld",i, getSem(i));
        }
    }
}