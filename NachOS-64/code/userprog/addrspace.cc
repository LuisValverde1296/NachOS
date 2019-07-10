// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(int id) {
    this->executionId = id;
}

AddrSpace::AddrSpace(OpenFile *exec)
{
    //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*
    executable = exec;
    static int nSwaps = 0; //Número de swaps
    sprintf(swapFileName, "swap%03d", ++nSwaps);

    int f = open(swapFile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); //Open con banderas de permiso
    ftruncate(f, NumPhysPages*PageSize);
    close(f);

    SWAP = fileSystem->Open(swapFileName); //Abre el arch que está en Swap
    SWAPMap = new BitMap(NumPhysPages);
    //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*

    NoffHeader noffH;
    unsigned int i, size, textPages, dataPages,textOffSet; // cambiar

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    NoffHeader noffH;
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			/*/+ UserStackSize*/;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    numPages += divRoundUp(UserStackSize, PageSize);
    size = numPages * PageSize;

    textPages = divRoundDown(noffH.code.size, PageSize); //Without the offset of text
    textOffSet = noffH.code.size - (textPages * PageSize); // Parte del texto que no se ha contado
    dataPages = divRoundUp(noffH.code.size + textOffSet, PageSize); // Aquí se empieza a contar
    //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*
    dataOffSet = noffH.initData.size + textOffSet, PageSize);
    stackPages = UserStackSize / PageSize;
    //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
    // first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	    pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
        //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*
#ifdef VM // de los labs
        pageTable[i].valid = false; // estaba en tue
#else
        pageTable[i].valid = true;
        pageTable[i].physicalPage = memoryPagesMap->Find();
#endif
	    pageTable[i].use = false;
	    pageTable[i].dirty = false;
	    pageTable[i].readOnly = false;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
        //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*
    }
// then, copy in the code and data segments into memory
#ifndef VM
    unsigned int j = 0;
    for(; j < textPages; ++j){
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        //executable->ReadAt(machine->mainMemory + (pageTable[j].physicalPage*PageSize), PageSize, noffH.code.inFileAddr + j * PageSize);
        executable->ReadAt(machine->mainMemory + (pageTable[j].physicalPage*PageSize), //
            PageSize, noffH.code.inFileAddr + j * PageSize);
        pageTable[j].readOnly = true;
    }
    for(; j < textPages + dataPages; ++j){
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);

        //executable->ReadAt(machine->mainMemory + (pageTable[j].physicalPage*PageSize), PageSize, noffH.code.inFileAddr + j * PageSize); // TO read the offSet.
        executable->ReadAt(machine->mainMemory + (pageTable[j].physicalPage*PageSize),
            PageSize, noffH.code.inFileAddr + j * PageSize); // TO read the offSet of text
    }
#endif
    state = new int[NumTotalRegs + 4];
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
//    bzero(machine->mainMemory, size);
}

AddrSpace::AddrSpace(AddrSpace *fatherAddrSpaces) {

    //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*
    executable = fatherAddrSpaces;
    static int nSwaps = 0; //Número de swaps
    sprintf(swapFileName, "swap%03d", ++nSwaps);

    int f = open(swapFile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); //Open con banderas de permiso
    ftruncate(f, NumPhysPages*PageSize);
    close(f);

    SWAP = fileSystem->Open(swapFileName); //Abre el arch que está en Swap
    SWAPMap = new BitMap(NumPhysPages);
    //-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*


    ASSERT(memoryPagesMap->NumClear() >= 8);
    
    state = new int[NumTotalRegs + 4];
    
    numPages = fatherAddrSpaces->numPages;
    
    pageTable = new TranslationEntry[numPages]; 
    
    unsigned int stackPages = divRoundUp(UserStackSize, PageSize);
    for(unsigned int pagesIndex = 0; pagesIndex < (numPages - stackPages); pagesIndex++){
        pageTable[pagesIndex].virtualPage = pagesIndex;
        pageTable[pagesIndex].physicalPage = fatherAddrSpaces->pageTable[pagesIndex].physicalPage;
        pageTable[pagesIndex].valid = fatherAddrSpaces->pageTable[pagesIndex].valid;
        pageTable[pagesIndex].use = fatherAddrSpaces->pageTable[pagesIndex].use;
        pageTable[pagesIndex].dirty = fatherAddrSpaces->pageTable[pagesIndex].dirty;
        pageTable[pagesIndex].readOnly = fatherAddrSpaces->pageTable[pagesIndex].readOnly;
    }
    
    for(unsigned int pagesIndex = (numPages - stackPages); pagesIndex < numPages;pagesIndex++){
        pageTable[pagesIndex].virtualPage = pagesIndex;
        pageTable[pagesIndex].physicalPage = memoryPagesMap->Find();
        pageTable[pagesIndex].valid = true;
        pageTable[pagesIndex].use = false;
        pageTable[pagesIndex].dirty = false;
        pageTable[pagesIndex].readOnly = false;
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    for(unsigned int i = 0; i < numPages;++i){
        if(pageTable[i].valid)
            memoryPagesMap->Clear(pageTable[i].physicalPage);  
    }
    remove(swapFileName);
    delete pageTable;
    delete pageTable;
    delte executable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
#ifdef VM
    int VPN = -1; // Se inicia en un número inválido
    bool currentUseValue = false;
    bool currentDirtyValue = false;

    for (int i - 0; i < TBLSize; i++){
        VPN = machine->tlb[i].virtualPage;
        
    }

}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::setExecutionId(int id){
    this->executionId = id;
}

int AddrSpace::getExecutionId(){
    return this->executionId;
}