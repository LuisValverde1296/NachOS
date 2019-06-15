#include "nachostabla.h"

NachosOpenFilesTable::NachosOpenFilesTable() : usage(0)      // Initialize
{
    openFiles = new int[SIZE];
    openFilesMap = new BitMap(SIZE);

    for(int i = 0; i < 3; ++i){
        openFiles[i] = i;
        openFilesMap->Mark(i);
    }
    for(int i = 3; i < SIZE; ++i){
        openFiles[i] = -1;
    }
} 
NachosOpenFilesTable::~NachosOpenFilesTable()      // De-allocate
{
    delete openFiles;
    delete openFilesMap;
}
int NachosOpenFilesTable::Open( int UnixHandle ) // Register the file handle
{
    int available = openFilesMap->Find();
    if (available != -1){ // If there is space available
        openFiles[available] = UnixHandle; 
    }
    return available; //If available returns the free space position where it was allocated, otherwise returns -1.
}
int NachosOpenFilesTable::Close( int NachosHandle )      // Unregister the file handle
{
    if (!isOpened(NachosHandle)){ // If it's not a running process
        return -1; //Returns -1
    } else { //Otherwise
        openFilesMap->Clear(NachosHandle); // Clears the process.
        return getUnixHandle(NachosHandle); //and returns the handler.
    }
}
bool NachosOpenFilesTable::isOpened( int NachosHandle )
{
    bool result = openFilesMap->Test(NachosHandle);
    return result;
}
int NachosOpenFilesTable::getUnixHandle( int NachosHandle )
{
    if (isOpened(NachosHandle)){
        return this->openFiles[NachosHandle];
    } else {
        return -1;
    }
}
void NachosOpenFilesTable::addThread()		// If a user thread is using this table, add it
{
    ++usage;
}
void NachosOpenFilesTable::delThread()		// If a user thread is using this table, delete it
{
    --usage;
}
void NachosOpenFilesTable::Print()               // Print contents
{
    printf("Table Pos    UnixHandle    IsOpen");
    for(int i = 0; i < SIZE;++i){
        if(openFilesMap->Test(i)){
            printf("%-12d : %-14d : %d",i, this->getUnixHandle(i), this->openFilesMap->Test(i));
        }
    }
}