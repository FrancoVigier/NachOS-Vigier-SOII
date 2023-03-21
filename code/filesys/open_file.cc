/// Routines to manage an open Nachos file.  As in UNIX, a file must be open
/// before we can read or write to it.  Once we are all done, we can close it
/// (in Nachos, by deleting the `OpenFile` data structure).
///
/// Also as in UNIX, for convenience, we keep the file header in memory while
/// the file is open.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "open_file.hh"
#include "file_header.hh"
#include "threads/system.hh"
#include "threads/channel.hh"
#include <stdio.h>
#include <string.h>

/// Open a Nachos file for reading and writing.  Bring the file header into
/// memory while the file is open.
///
/// * `sector` is the location on disk of the file header for this file.
OpenFile::OpenFile(int sectorParam)
{
    hdr = new FileHeader;
    hdr->FetchFrom(sectorParam);
    seekPosition = 0;
    sector = sectorParam;
    currentSector = sectorParam;
    if(tableDeArchivosAbierta[sector]->count == 0) {    // no one else has the file open
        Lock* writeLock = new Lock("Write Lock");
        tableDeArchivosAbierta[sector]->writeLock = writeLock;

        if( tableDeArchivosAbierta[sector]->removeLock == nullptr ) { // We do not have a removelock yet
            Lock* removeLock = new Lock("Remove Lock");
            tableDeArchivosAbierta[sector]->removeLock = removeLock;
        }
        if( tableDeArchivosAbierta[sector]->closeLock == nullptr ) { // We do not have a removelock yet
            Lock* closeLock = new Lock("Close Lock");
            tableDeArchivosAbierta[sector]->closeLock = closeLock;
        }
    }
    tableDeArchivosAbierta[sector]->count++; // add one user to the count of the threads that have the file open
}

/// Close a Nachos file, de-allocating any in-memory data structures.
OpenFile::~OpenFile()
{
    DEBUG('f', "Removing open file\n");
    // Decrease the counter meaning one less open file for this sector
    tableDeArchivosAbierta[sector]->closeLock->Acquire();
    if(tableDeArchivosAbierta[sector]->count > 0)
        tableDeArchivosAbierta[sector]->count--;
    else { // To support more close calls than open calls
        tableDeArchivosAbierta[sector]->closeLock->Release();
        return;
    }
    tableDeArchivosAbierta[sector]->closeLock->Release();

    // If this is the last open file of this sector free the lock
    DEBUG('f', "The count for the file is: %d\n", tableDeArchivosAbierta[sector]->count);
    if(tableDeArchivosAbierta[sector]->count == 0 ) {
        if(tableDeArchivosAbierta[sector]->removing) {
            int removerSpaceId = tableDeArchivosAbierta[sector]->removerSpaceId;
            if(threadUPS->HasKey(removerSpaceId)){
                DEBUG('f', "About to send msg to remover...\n");
                threadUPS->Get(removerSpaceId)->removeChannel->Send(0);
            } // despierto al hilo que esta esperando que los hilos cierren el archivo que quiere borrar
        }
DEBUG('f', "tableDeArchivos...\n");
        delete tableDeArchivosAbierta[sector]->writeLock;
    }
    DEBUG('f', "HDR IN...\n");
    delete hdr;
    DEBUG('f', "HDR OUT...\n");
}

// More meaningfull name for the deconsructor
void
OpenFile::Close()
{
  delete this;
}

/// Change the current location within the open file -- the point at which
/// the next `Read` or `Write` will start from.
///
/// * `position` is the location within the file for the next `Read`/`Write`.
void
OpenFile::Seek(unsigned position)
{
    seekPosition = position;
}

/// OpenFile::Read/Write
///
/// Read/write a portion of a file, starting from `seekPosition`.  Return the
/// number of bytes actually written or read, and as a side effect, increment
/// the current position within the file.
///
/// Implemented using the more primitive `ReadAt`/`WriteAt`.
///
/// * `into` is the buffer to contain the data to be read from disk.
/// * `from` is the buffer containing the data to be written to disk.
/// * `numBytes` is the number of bytes to transfer.

int
OpenFile::Read(char *into, unsigned numBytes, bool isDirectory)
{
    ASSERT(into != nullptr);
    ASSERT(numBytes > 0);

    if(isDirectory) {
        hdr->FetchFrom(sector);
        seekPosition = 0;
    }

    int result = 0;

    if (seekPosition + numBytes > hdr->GetRaw()->numBytes) {
        DEBUG('w', "seekPostion is: %u, numBytes is: %u, maximuxbytes is:%u\n",seekPosition,numBytes,hdr->GetRaw()->numBytes);
        result = ReadAt(into, numBytes, seekPosition);
        seekPosition += result;
        numBytes -= result;
        char* temp = into + result;
        DEBUG('w', "bytes read: %u\n", result);
        unsigned nextSector = hdr->GetRaw()->nextFileHeader;
        DEBUG('w', "READING the next file header sector is: %u\n", nextSector);
        while(nextSector && numBytes > 0) {
            seekPosition = 0;
            hdr->FetchFrom(nextSector);
            unsigned result_tmp = ReadAt(temp, numBytes, seekPosition);
            temp+= result_tmp;
            numBytes -= result_tmp;
            result += result_tmp;
            DEBUG('w', "bytes read: %u\n", result);
            seekPosition += result_tmp;

            nextSector = hdr->GetRaw()->nextFileHeader;
            DEBUG('w', "READING the next file header sector is: %u\n", nextSector);
        }
    } else {
        DEBUG('w', "going to read with read at!\n");
        result = ReadAt(into, numBytes, seekPosition);
        DEBUG('w', "bytes read: %u\n", result);

        seekPosition += result;
    }
    DEBUG('f', "READAT OF BUF READ\n");
    return result;
}

int
OpenFile::Write(const char *from, unsigned numBytes, bool isDirectory)
{
    DEBUG('w',"Attempting to write: %u bytes\n", numBytes);

    ASSERT(from != nullptr);
    ASSERT(numBytes > 0);

    tableDeArchivosAbierta[sector]->writeLock->Acquire();
///



///
    if(isDirectory) {
        seekPosition = 0;
        currentSector = sector;
    }

    ///Fetcheamo el header porque puede ser modificado
    hdr->FetchFrom(currentSector);

///
    unsigned maxBytesToWrite = NUM_DIRECT * SECTOR_SIZE - seekPosition; ///La cantidad maxima de bytes posibles de escribir
    unsigned bytesToWrite = 0;
    if(numBytes > maxBytesToWrite)
        bytesToWrite = maxBytesToWrite;
    else
        bytesToWrite = numBytes;
///
    bool success = true;
    unsigned result_tmp = 0;
	int result  = 0;

	Bitmap *freeMap = new Bitmap(NUM_SECTORS);
	freeMap->FetchFrom(fileSystem->GetFreeDirEntries());

    if(bytesToWrite > 0) {  ///Podemos escribir en el File Header
        if(seekPosition + bytesToWrite > hdr->GetRaw()->numBytes) {
            unsigned bytesToAllocate = (seekPosition + bytesToWrite) - hdr->GetRaw()->numBytes; ///Calculamos dif para allocar los bytes
            success = hdr->Allocate(freeMap, bytesToAllocate); ///Intento allocar la porcion nueva en los bloques gratis del header
            if(!success) { ///Si no pude alocar la diferencia del escribir. Devuelvo 0
                delete freeMap;
                tableDeArchivosAbierta[sector]->writeLock->Release();
                return result;
            }
            freeMap->WriteBack(fileSystem->GetFreeDirEntries()); ///Como se escribe, el bitmap de las direntries libres y el hdr se tienen que llevar a disk
            hdr->WriteBack(currentSector);
        }

        result_tmp = WriteAt(from, bytesToWrite, seekPosition); ///Escribo desde la posicion de seek para delante

        ///Tengo que actualizar las posiciones
        result += result_tmp; ///Acumulo la escrituras

        ///Cambio las posiciones, ahora seek es hasta donde escribi recien y la cantidad de bytes qe faltan le resto lo que escribi
        from += result_tmp;

        seekPosition += result_tmp;
        numBytes -= result_tmp;
    }

    ///Como es synch tengo que recorrer la lista de los headers para ver que otro hilo no haya creado el header antes
    unsigned iterFh = hdr->GetRaw()->nextFileHeader;///Obtengo el next header
    while(numBytes > 0 && iterFh != 0) {
        hdr->FetchFrom(iterFh); ///Lo fetcheo

        maxBytesToWrite = NUM_DIRECT * SECTOR_SIZE;

        if(numBytes > maxBytesToWrite)
            bytesToWrite = maxBytesToWrite;
        else
            bytesToWrite = numBytes;

        if(bytesToWrite > hdr->GetRaw()->numBytes) { ///Si necesito execerme del header al escribir, se lo robo al header siguiente
            unsigned bytesToAllocate =  bytesToWrite - hdr->GetRaw()->numBytes;
            success = hdr->Allocate(freeMap, bytesToAllocate);
            if(!success) {
                delete freeMap;
                tableDeArchivosAbierta[sector]->writeLock->Release();
                return result;
            }
            ///Como se cambio el raaw file header, al disco lo mando
            freeMap->WriteBack(fileSystem->GetFreeDirEntries());
            hdr->WriteBack(iterFh);
        }

        currentSector = iterFh;///Como robe del next header, tengo que actualizarlo y con �l las posiciones de lectura

        seekPosition = 0;
        result_tmp = WriteAt(from, bytesToWrite, seekPosition);
        result += result_tmp;
        from += result_tmp;

        seekPosition += result_tmp;
        numBytes -= result_tmp;

        iterFh = hdr->GetRaw()->nextFileHeader;
    }


    if (numBytes > 0) { ///Si no me alcanza escribir en el primer hd, ni tampoco me alcanza escribir en los siguientes... necesito uno nuevo

        unsigned totalSectors = DivRoundUp(numBytes, SECTOR_SIZE); ///Creamos una nueva lista de sectores para poner a todos los fileheader
        unsigned fileHeaderSectors = DivRoundUp(totalSectors, NUM_DIRECT);

        if(freeMap->CountClear() < fileHeaderSectors){ ///Si al crear la lista nos damos cuenta que no hay espacio para el nuevo header, retornamos lo carry de result que en algun momento si todo sale bien se hace 0
            delete freeMap;
            tableDeArchivosAbierta[sector]->writeLock->Release(); ///No espacio en disco, suelto lock de escritura
            return result;
        }

        int* sectors = new int[fileHeaderSectors];
        for(unsigned i = 0; i < fileHeaderSectors; ++i) { ///Busco tantos sectores libres como fileheaders alla para 1:1
            sectors[i] = freeMap->Find();
            freeMap->WriteBack(fileSystem->GetFreeDirEntries());
        }


        unsigned maxSize = NUM_DIRECT * SECTOR_SIZE;
        unsigned bytesToAllocate = 0;

        if(numBytes <  maxSize)
            bytesToAllocate = numBytes;
        else
            bytesToAllocate = maxSize;

        FileHeader *firstHeader = new FileHeader; ///LSE de Fileheader este es su head
        firstHeader->GetRaw()->nextFileHeader = 0;
        firstHeader->GetRaw()->numBytes = 0;
        firstHeader->GetRaw()->numSectors = 0;

        success = firstHeader->Allocate(freeMap, bytesToAllocate); ///Alocando el primer header en alguna dir entry libre
        if(!success) { ///No se pudo alocar el 1er header, cosa que no pasaria
            delete freeMap;
            delete firstHeader;
            delete [] sectors;
            tableDeArchivosAbierta[sector]->writeLock->Release();
            return result;
        }

        freeMap->WriteBack(fileSystem->GetFreeDirEntries());///Como alloque el primer header tengo que mandar el bitmap a disk

        firstHeader->WriteBack(sectors[0]); ///El primer header tiene que mandar a disk el sector 0 porque va a empezar a allocar su siguiente fh

        hdr->GetRaw()->nextFileHeader = sectors[0];
        hdr->WriteBack(currentSector); ///Escribo el hdr actual con la siguiente cabecera

        FileHeader * fhToDestroy = hdr;
        hdr = firstHeader; ///Se pisa el viejo hdr con el nuevo

        delete fhToDestroy;

        ///Ahora vamos a armar el resto de la lse de hd

        seekPosition = 0;

        result_tmp = WriteAt(from, bytesToAllocate, seekPosition); ///Como ya esta alocado el primero tengo que correr las posiciones
        result += result_tmp;
        from += result_tmp;

        seekPosition += result_tmp;
        numBytes -= result_tmp;

        FileHeader** fileHeaders = new FileHeader*[fileHeaderSectors]; ///Creo la lista LSE propiamente dicha
        fileHeaders[0] = firstHeader; ///Pongo como cabecera a first header

        FileHeader* previousFileHeader = firstHeader;

        unsigned allocatedFileHeaders = 1; ///Contador que nos va a ayudar a llevar la alloc de los hd restantes de la cola
        while(allocatedFileHeaders < fileHeaderSectors && success) {

            if(numBytes <  maxSize)
                bytesToAllocate = numBytes;
            else
                bytesToAllocate = maxSize;

            FileHeader *newFileHeader = new FileHeader;  ///Creo el nodo de la lista
            newFileHeader->GetRaw()->nextFileHeader = 0;
            newFileHeader->GetRaw()->numBytes = 0;
            newFileHeader->GetRaw()->numSectors = 0;

            if(freeMap->CountClear() < DivRoundUp(bytesToAllocate, SECTOR_SIZE))
                bytesToAllocate = freeMap->CountClear() * SECTOR_SIZE;

            if(bytesToAllocate > 0) ///Alloco en memoria el nuevo nodo
                success = newFileHeader->Allocate(freeMap, bytesToAllocate);
            else
                success = false;

            if(success) {
                freeMap->WriteBack(fileSystem->GetFreeDirEntries());///Como actualice el nodo y el bitmap allocandolo los mando a disk
                newFileHeader->WriteBack(sectors[allocatedFileHeaders]);///Cabe decir que lo mando a disk y se respeta el indice del sector que fue indicado

                previousFileHeader->GetRaw()->nextFileHeader = sectors[allocatedFileHeaders]; ///Linkeo la LSE que ya tenia con el nodo nuevo
                previousFileHeader->WriteBack(sectors[allocatedFileHeaders - 1]); ///Como se actualizo la lista el anterior manda a disco

                ///Logramos encolar por lo que tenemos que actualizar las posiciones
                seekPosition = 0;
                hdr = newFileHeader;
                result_tmp = WriteAt(from, bytesToAllocate, seekPosition);
                result += result_tmp;
                from += result_tmp;

                seekPosition += result_tmp;
                numBytes -= result_tmp;

                previousFileHeader = newFileHeader;
                fileHeaders[allocatedFileHeaders] = newFileHeader; ///A la lista de fh indexo el nuevo, mas alla de la LSE para eliminar la referencia
                ++allocatedFileHeaders;
            }
        }

        for(unsigned i = 0; i < allocatedFileHeaders - 1; ++i) { ///Deleteo los New FileHeader que se crearon a lo largo de la conformacion de la LSE
            delete fileHeaders[i];
        }
        delete [] fileHeaders;

        if(!success)
            for(unsigned i = allocatedFileHeaders; i < fileHeaderSectors;++i) ///Si algun sector falla lo limpio...
                freeMap->Clear(sectors[i]);


        currentSector = sectors[allocatedFileHeaders - 1]; ///El current sector es el penultimo allocado porque asi si se da de nuevo write puedo robar del ultimo sector(fh) dado el caso

        delete [] sectors;
    }
    delete freeMap;
    tableDeArchivosAbierta[sector]->writeLock->Release(); ///Allocado todo, se suelta el lock de escritura

    return result;
}

/// OpenFile::ReadAt/WriteAt
///
/// Read/write a portion of a file, starting at `position`.  Return the
/// number of bytes actually written or read, but has no side effects (except
/// that `Write` modifies the file, of course).
///
/// There is no guarantee the request starts or ends on an even disk sector
/// boundary; however the disk only knows how to read/write a whole disk
/// sector at a time.  Thus:
///
/// For ReadAt:
///     We read in all of the full or partial sectors that are part of the
///     request, but we only copy the part we are interested in.
/// For WriteAt:
///     We must first read in any sectors that will be partially written, so
///     that we do not overwrite the unmodified portion.  We then copy in the
///     data that will be modified, and write back all the full or partial
///     sectors that are part of the request.
///
/// * `into` is the buffer to contain the data to be read from disk.
/// * `from` is the buffer containing the data to be written to disk.
/// * `numBytes` is the number of bytes to transfer.
/// * `position` is the offset within the file of the first byte to be
///   read/written.

int
OpenFile::ReadAt(char *into, unsigned numBytes, unsigned position)
{
    ASSERT(into != nullptr);
    ASSERT(numBytes > 0);

    unsigned fileLength = hdr->FileLength();
    unsigned firstSector, lastSector, numSectors;
    char *buf;

    if (position >= fileLength)
        return 0;

    if (position + numBytes > fileLength)
        numBytes = fileLength - position;

    DEBUG('f', "Reading %u bytes at %u, from file of length %u.\n",
          numBytes, position, fileLength);

    firstSector = DivRoundDown(position, SECTOR_SIZE);
    lastSector = DivRoundDown(position + numBytes - 1, SECTOR_SIZE);
    numSectors = 1 + lastSector - firstSector;

    // Read in all the full and partial sectors that we need.
    buf = new char [numSectors * SECTOR_SIZE];
    for (unsigned i = firstSector; i <= lastSector; i++) {
        unsigned sectorToRead = hdr->ByteToSector(i * SECTOR_SIZE);
        synchDisk->ReadSector(sectorToRead,
                              &buf[(i - firstSector) * SECTOR_SIZE]);
    }

    // Copy the part we want.
    memcpy(into, &buf[position - firstSector * SECTOR_SIZE], numBytes);
    DEBUG('f', "READAT DELETE BUF IN\n");
 //   delete [] buf;
    DEBUG('f', "READAT DELETE BUF OUT\n");
    return numBytes;
}

int
OpenFile::WriteAt(const char *from, unsigned numBytes, unsigned position)
{
    ASSERT(from != nullptr);
    ASSERT(numBytes > 0);

    unsigned fileLength = hdr->FileLength();
    unsigned firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if (position >= fileLength)
        return 0;

    if (position + numBytes > fileLength)
        numBytes = fileLength - position;

    DEBUG('f', "Writing %u bytes at %u, from file of length %u.\n",
          numBytes, position, fileLength);

    firstSector = DivRoundDown(position, SECTOR_SIZE);
    lastSector  = DivRoundDown(position + numBytes - 1, SECTOR_SIZE);
    numSectors  = 1 + lastSector - firstSector;

    buf = new char [numSectors * SECTOR_SIZE];

    firstAligned = position == firstSector * SECTOR_SIZE;
    lastAligned  = position + numBytes == (lastSector + 1) * SECTOR_SIZE;

    // Read in first and last sector, if they are to be partially modified.
    if (!firstAligned) {
        ReadAt(buf, SECTOR_SIZE, firstSector * SECTOR_SIZE);
        DEBUG('f', "READAT OF BUF 1\n");
    }
    if (!lastAligned && (firstSector != lastSector || firstAligned)) {
        ReadAt(&buf[(lastSector - firstSector) * SECTOR_SIZE],
               SECTOR_SIZE, lastSector * SECTOR_SIZE);
               DEBUG('f', "READAT OF BUF 1\n");
    }

    // Copy in the bytes we want to change.
    memcpy(&buf[position - firstSector * SECTOR_SIZE], from, numBytes);

    // Write modified sectors back.
    for (unsigned i = firstSector; i <= lastSector; i++) {
        synchDisk->WriteSector(hdr->ByteToSector(i * SECTOR_SIZE),
                               &buf[(i - firstSector) * SECTOR_SIZE]);
    }
    delete [] buf;
    return numBytes;
}

/// Return the number of bytes in the file.
unsigned
OpenFile::Length() const
{
    return hdr->FileLength();
}

int
OpenFile::GetSector()
{
    return sector;
}
