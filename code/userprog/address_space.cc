/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"

#include <string.h>

#ifdef SWAP
#include "vmem/coremap.hh"
#include "filesys/directory_entry.hh"
#endif


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file,  unsigned pid)
{
    ASSERT(executable_file != nullptr);

    exe = new Executable(executable_file);
    ASSERT(exe->CheckMagic());

    // How big is address space?

    unsigned size = exe->GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;


    #ifdef SWAP
        nameSwapPid = new char[FILE_NAME_MAX_LEN];

        snprintf(nameSwapPid, FILE_NAME_MAX_LEN, "SWAP.%u", pid);
		fileSystem->Remove(nameSwapPid);
		ASSERT(fileSystem->Create(nameSwapPid, size));

        ASSERT(swapFD = fileSystem->Open(nameSwapPid));
        swapMap = new Bitmap(numPages);
    #else
    	if(numPages > paginaMapa->CountClear()) {
            DEBUG('a', "Out of space, no swap to disk, just crashing!\n");
            paginaMapa->Print();
        }
        ASSERT(numPages <= paginaMapa->CountClear());

    #endif
   // swapMap = new Bitmap(numPages);

    // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.
    #ifndef DEMAND_LOADING
        char *mainMemory = machine->GetMMU()->mainMemory;
    #endif // DEMAND_LOADING

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;

        #ifndef DEMAND_LOADING
            int newPag = paginaMapa->Find();
            ASSERT(newPag != -1);

            pageTable[i].physicalPage = (unsigned int)newPag;
            pageTable[i].valid        = true;
        #else
            pageTable[i].physicalPage = -1;
            pageTable[i].valid = false;
        #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
        #ifndef DEMAND_LOADING
            memset(&mainMemory[newPag*PAGE_SIZE], 0, PAGE_SIZE);
        #endif // DEMAND_LOADING
    }

 //   char *mainMemory = machine->GetMMU()->mainMemory;

	//const uint32_t absoluteRealPositionStartProgram = pageTable[0].physicalPage*PAGE_SIZE;
	//DEBUG('a', "Initializing program at position 0x%X\n", absoluteRealPositionStartProgram*PAGE_SIZE);

	//memset(&mainMemory[absoluteRealPositionStartProgram], 0, PAGE_SIZE*numPages);//Asegura que otro progama no lea al programa anterior

    // Then, copy in the code and data segments into memory.

    #ifndef DEMAND_LOADING


       for(unsigned i = 0; i < numPages; i++) {
      memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE); //looks fine
    }

        uint32_t codeSize = exe->GetCodeSize();
        if (codeSize > 0) {
            uint32_t virtualAddr = exe->GetCodeAddr();
            DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
                  virtualAddr, codeSize);
            for (uint32_t i = 0; i < codeSize; i++) {
                uint32_t frame = pageTable[DivRoundDown(virtualAddr + i, PAGE_SIZE)].physicalPage;
                uint32_t offset = (virtualAddr + i) % PAGE_SIZE;
                uint32_t physicalAddr = frame * PAGE_SIZE + offset;
                exe->ReadCodeBlock(&mainMemory[physicalAddr], 1, i);
            }
        }
        uint32_t initDataSize = exe->GetInitDataSize();
        if (initDataSize > 0) {
            uint32_t virtualAddr = exe->GetInitDataAddr();
            DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
                  virtualAddr, initDataSize);
            for (uint32_t i = 0; i < initDataSize; i++) {
                uint32_t frame = pageTable[DivRoundDown(virtualAddr + i, PAGE_SIZE)].physicalPage;
                uint32_t offset = (virtualAddr + i) % PAGE_SIZE;
                uint32_t physicalAddr = frame * PAGE_SIZE + offset;
                exe->ReadDataBlock(&mainMemory[physicalAddr], 1, i);
            }
        }
    #endif // DEMAND_LOADING
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
	for(unsigned i = 0; i < numPages; i++) {
        if (pageTable[i].valid)
            paginaMapa->Clear(pageTable[i].physicalPage);
	}

    delete [] pageTable;
    #ifdef SWAP
  //  fileSystem->Remove(nameSwapPid);
    delete [] nameSwapPid;
    delete swapMap;
    #endif

	delete exe;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState(){
#ifdef SWAP
  TranslationEntry *tlb = machine->GetMMU()->tlb;

  for (unsigned i = 0; i < TLB_SIZE; i++){
    if (tlb[i].valid){
      unsigned vpn = tlb[i].virtualPage;
      pageTable[vpn].dirty = tlb[i].dirty;
      pageTable[vpn].use = tlb[i].use;
      //
      machine->GetMMU()->tlb[i].valid=false;
      //
    }
  }
#endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    #ifndef USE_TLB
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
    #else
    DEBUG('e', "Cambio de contexto TLB NO CONSISTENTE...\n");
  //  TranslationEntry *tlb = machine->GetMMU()->tlb;
    for (unsigned i = 0; i < TLB_SIZE; i++) {
        machine->GetMMU()->tlb[i].valid = false;
      }
    #endif
}



#ifdef DEMAND_LOADING

#ifdef SWAP

void
AddressSpace::EscribirFrameenSwap(unsigned frame){

  int vpn = paginaMapa->GetVpn(frame);

  AddressSpace* space = paginaMapa->GetAddressSpace(frame);

  TranslationEntry *tlb = machine->GetMMU()->tlb;

  for (unsigned i = 0; i < TLB_SIZE; i++) {
    if (tlb[i].physicalPage == frame && tlb[i].valid) {
      pageTable[vpn] = tlb[i];
      tlb[i].valid = false;
      break;
    }
  }

  if (pageTable[vpn].dirty) { // SI LA PAGINA ESTA DIRTY ANTES SOBREESCRIBIR CON MARK LA MANDO A MEMORIA AL ARCHIVO SWWAPPPPPPPPP GIL, SI YA ESTA LIMPIA LA PISO TRANQUI
    DEBUG('e', "Pagina Victima Sucia, hay que lavarla y mandarla a escribir en mem...\n");
    char *mainMemory = machine->GetMMU()->mainMemory;
    space->swapFD->WriteAt(&mainMemory[frame * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
    space->swapMap->Mark(vpn);
    stats->numPageToSwap++;
  }

  pageTable[vpn].dirty = false;
  pageTable[vpn].valid = false;

  return;
}


int
AddressSpace::SustitucionEnSwapDeMarcoVictima( unsigned vpn){ ///Elegimos el marco victima de reemplazo, y meto la pagina que necesito en el y le asigno al marco su nueva vpn con mark
    int frame = PickVictim();

    EscribirFrameenSwap(frame);
    DEBUG('e', "Frame Liberado, listo para recibir su nueva pagina\n");

    paginaMapa->Mark(frame, this, vpn);///Mark sobreescribe, no le importa si no está limpia
    return frame;
}


unsigned victima = 0;


TranslationEntry *
AddressSpace::TLBSearch(){

    TranslationEntry *tlb = machine->GetMMU()->tlb;

    for (unsigned j = 0; j < TLB_SIZE; j++) {
        if (tlb[j].valid && tlb[j].physicalPage == victima)
            return &tlb[j];
    }

    AddressSpace* space = paginaMapa->GetAddressSpace(victima);
    if (space == nullptr)
        return nullptr;
    int vpn = paginaMapa->GetVpn(victima);
    return &pageTable[vpn];
}


unsigned
AddressSpace::PickVictim()
{
    #ifdef PRPOLICY_FIFO
        int marcoVictima = victima;
        victima++;
        victima %= NUM_PHYS_PAGES;
        return marcoVictima;
    #endif // PRPOLICY_FIFO

    #ifdef PRPOLICY_CLOCK
        TranslationEntry *entry;
        for (unsigned round = 1; round < 5; round++) {
            for (unsigned p = victima; p < NUM_PHYS_PAGES + victima; p++) {
                entry = TLBSearch();
                if (entry != nullptr) {
                    switch (round) {
                        case 1:
                            if (!entry->use && !entry->dirty){
                                victima = (p + 1) % NUM_PHYS_PAGES;
                                return p % NUM_PHYS_PAGES;
                            }
                        break;
                        case 2:
                            if (!entry->use && entry->dirty){
                                victima = (p + 1) % NUM_PHYS_PAGES;
                                return p % NUM_PHYS_PAGES;
                            }
                            else
                              entry->use = false;
                        break;
                        case 3:
                            if (!entry->use && !entry->dirty){
                                victima = (p + 1) % NUM_PHYS_PAGES;
                                return p % NUM_PHYS_PAGES;
                            }
                        break;
                        case 4:
                            if (!entry->use && entry->dirty){
                                victima = (p + 1) % NUM_PHYS_PAGES;
                                return p % NUM_PHYS_PAGES;
                            }
                        break;
                        }
                }
            }
        }
        victima = (victima + 1) % NUM_PHYS_PAGES;
        return victima;
    #endif // PRPOLICY_CLOCK
    return random() % NUM_PHYS_PAGES;/// RANDOM_POLICY
}

#endif // SWAP



void
AddressSpace::LoadPage(unsigned vpn)
{
    ASSERT(!pageTable[vpn].valid);
    char *mainMemory = machine->GetMMU()->mainMemory;
    int frame;
    #ifndef SWAP
        frame = paginaMapa->Find(); /// EL MARCO RESULTANTE TIENE QUE SER VALIDO, Para poder cargar la pag en el
        ASSERT(frame != -1);
    #else
        frame = paginaMapa->Find(this, vpn); ///Si estamos en swap buscamos la pagina, si no esta entonces tenemos que elegir la victima  y Luego cargarla escribiento en la swap
        if (frame == - 1)
            frame = SustitucionEnSwapDeMarcoVictima(vpn);
    #endif
///****
    uint32_t DirPhy = frame * PAGE_SIZE;
    unsigned DirVir = vpn * PAGE_SIZE;
    pageTable[vpn].physicalPage = frame;
    pageTable[vpn].valid = true;
///****
    memset(mainMemory + DirPhy, 0, PAGE_SIZE);/// Pongo en 0 la memoria , y luego empiezo a copiar el espacio de code  y el de data

    #ifdef SWAP
        if (swapMap->Test(vpn)){  ///Si la pagina ya esta en la swap se la lee trank palank
            swapFD->ReadAt(&mainMemory[DirPhy], PAGE_SIZE, DirVir);
            stats->numPagetoTLB++;
            DEBUG('f', "READAT ADDRSPACE IN\n");
        }
    #endif // SWAP

    #ifdef DEMAND_LOADING
	    #ifdef SWAP
        if(!swapMap->Test(vpn)){
		#else
		{
		#endif // SWAP
            DEBUG('e',"CARGANDO PAGINA POR DEMANDA\n");
            //uint32_t codeAddr = exe->GetCodeAddr();
            uint32_t initDataAddr = exe->GetInitDataAddr();
            uint32_t codeSize = exe->GetCodeSize();
            uint32_t initDataSize = exe->GetInitDataSize();

            unsigned bytesCopyCODE = 0;
            unsigned bytesCopyDATA = 0;

            if (codeSize > 0 && DirVir < codeSize) { ///Copiamos el segmento de Codigo del Exe
                DEBUG('e', "ESPACIO DE CODE COPY\n");
                bytesCopyCODE = codeSize - DirVir;

                if(PAGE_SIZE < bytesCopyCODE) ///El bloque a leer no puede ser mayor a 1 pag :,(
                    bytesCopyCODE = PAGE_SIZE;

                exe->ReadCodeBlock(&mainMemory[DirPhy], bytesCopyCODE, DirVir);
            }

            if (initDataSize > 0 && DirVir + bytesCopyCODE < initDataAddr + initDataSize && bytesCopyCODE < PAGE_SIZE ) {
                DEBUG('e', "ESPACIO DE DATA COPY\n");
                unsigned offset = 0;
                if(bytesCopyCODE > 0)
                    offset = 0;
                else
                    offset = DirVir - codeSize;

                if(initDataSize - offset < PAGE_SIZE - bytesCopyCODE) ///El bloque no puede superar una 1 pag por lo tanto lo que sea ahora tiene que ser menor a PAGE ZISE menos lo que ya se leyó en COPYCODE
                    bytesCopyDATA = initDataSize - offset ;
                else
                    bytesCopyDATA = PAGE_SIZE - bytesCopyCODE;

                exe->ReadDataBlock(&mainMemory[DirPhy + bytesCopyCODE], bytesCopyDATA, offset);
            }
            ASSERT(bytesCopyCODE+bytesCopyDATA <= PAGE_SIZE);
        }
    #endif // DEMAND_LOADING
    return;
}
#endif // DEMAND_LOADING
