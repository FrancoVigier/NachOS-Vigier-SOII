/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// â€œopenâ€ continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "threads/system.hh"
#include "threads/channel.hh"
#include "file_system.hh"
#include "directory.hh"
#include "file_header.hh"
#include "lib/bitmap.hh"

#include <stdio.h>
#include <string.h>


/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        Bitmap     *freeMap = new Bitmap(NUM_SECTORS);
        Directory  *dir     = new Directory(NUM_DIR_ENTRIES);
        FileHeader *mapH    = new FileHeader;
        FileHeader *dirH    = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapH->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        mapH->GetRaw()->nextFileHeader = 0; // FAT convention for indicating the end of the current N blocks of a file
        ASSERT(dirH->Allocate(freeMap, DIRECTORY_FILE_SIZE));
        dirH->GetRaw()->nextFileHeader = 0;

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapH->WriteBack(FREE_MAP_SECTOR);
        dirH->WriteBack(DIRECTORY_SECTOR);

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);

        // Once we have the files â€œopenâ€, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile);     // flush changes to disk
        dir->WriteBack(directoryFile, true);
///
        unsigned result = dir->FetchFrom(directoryFile, true); //Obtenemos el tamaÃ±o actual del DIR, lo necesitamos para el ej 3. para soportar extensibilidad en los archivos
        directorySize = result;
///

        if (debug.IsEnabled('f')) {
            freeMap->Print();
            dir->Print();

            delete freeMap;
            delete dir;
            delete mapH;
            delete dirH;
        }
    } else {
        // If we are not formatting the disk, just open the files
        // representing the bitmap and directory; these are left open while
        // Nachos is running.
        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);
///
        Directory  *dir = new Directory(MAX_DIR_ENTRIES);
        unsigned result = dir->FetchFrom(directoryFile, true); //Obtenemos el tamaÃ±o actual del DIR, lo necesitamos para el ej 3. para soportar extensibilidad en los archivos
        directorySize = result;
        delete dir;
///
    }
}

FileSystem::~FileSystem()
{
    delete freeMapFile;
    delete directoryFile;
}

/// Debug function for the raw table
void
PrintTable(RawDirectory raw) {
    DEBUG('w', "DEBUG TABLEE\n\n", raw.tableSize);
    DEBUG('w', "table size: %u\n", raw.tableSize);
    for (unsigned i = 100; i < raw.tableSize; i++)
        DEBUG('w', "Index: %u, InUse? %d Name: %s\n",i,raw.table[i].inUse,raw.table[i].name);
    DEBUG('w', "\n\n");
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// `Create` the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created. <-- Deprecated
/// The created file always has initial size equal to 0
int
LastSlash(const char *name){
    int lastSlashFound = 0;
    int i = 1;
    for(; name[i] != '\0'; ++i)
        if(name[i] == '/')
            lastSlashFound = i;
    return lastSlashFound;
}



bool
FileSystem::Create(const char *name, unsigned noUso) // Dummy param Para mantener la Create (char, unsigned)
{
    ASSERT(name != nullptr);

    DEBUG('f', "Create EXTENSIBLE FILE %s, size 0 INITIAL\n", name);

    bool success = true;
    bool changeDir = false;

    OpenFile* directoryFileBackup;
    filesysCreateLock->Acquire();
    char* nameCopy;
///
    if(name[0] == '/') {

        if(name[1] == '\0') { ///Error 1 no se puede crear el ./
            filesysCreateLock->Release();
            return false;
        }

        nameCopy = new char[FILE_PATH_MAX_LEN];
        sprintf(nameCopy, "%s", name);

        int lastSlashFound = LastSlash(name);

        nameCopy[lastSlashFound] = '\0';

        directoryFileBackup = directoryFile;
        ///Si el nombre copiado es inalcanzable chau todo
        if(!( (strlen(nameCopy) == 0 && ChangeDir("/", false)) || (strlen(nameCopy) > 0 && ChangeDir(nameCopy, false)) )){
            delete [] nameCopy;
            filesysCreateLock->Release();
            return false;
        }
        name = &nameCopy[lastSlashFound + 1];
        DEBUG('f', "Create file: %s\n", name);
        changeDir = true;
    }
///
    Directory *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);

    ///Si en el directorio current hay un arch con mismo nombre, no creamos ningun file
    if (dir->Find(name) != -1)
        success = false;
    else {

        Bitmap *freeMap = new Bitmap(NUM_SECTORS); ///Si no esta en el directorio entonecess para crear el archivo tengo que  agarrar alguna direntry libre en el dir current y tomarla
        freeMap->FetchFrom(freeMapFile);

        int sector = freeMap->Find();
        ///Si no hay dir entry libre no hay espacio :(
        if (sector == -1){
            success = false;
        }else{
         if(!dir->Add(name, sector)) ///Error de agregrado
                success = false;
            else if(success) { ///Si todo va bien Creo el file header para el nuevo archivo

            FileHeader *firstHeader = new FileHeader;
            firstHeader->GetRaw()->nextFileHeader = 0;
            firstHeader->GetRaw()->numBytes = 0;
            firstHeader->GetRaw()->numSectors = 0;

            firstHeader->WriteBack(sector);
            freeMap->WriteBack(freeMapFile);
            dir->WriteBack(directoryFile);

            tableDeArchivosAbierta[sector]->removed = false;
            tableDeArchivosAbierta[sector]->removing = false;
            tableDeArchivosAbierta[sector]->removeLock = new Lock("Remove Lock");
            tableDeArchivosAbierta[sector]->writeLock = nullptr;
            tableDeArchivosAbierta[sector]->closeLock = nullptr;
            tableDeArchivosAbierta[sector]->count = 0;

            DEBUG('f',"File creation OK!\n");
            delete firstHeader;
        }
        }
        delete freeMap;
    }
    ///Si para crear el archivo tuvimos que viajar en el directorio, volvemos a donde estabamos
    if( changeDir ) {
        OpenFile* tmpOpen = directoryFile; ///Vuelvo al dir original
        directoryFile = directoryFileBackup;
        delete tmpOpen;

        Directory *dummyForTableSize = new Directory(MAX_DIR_ENTRIES);
        unsigned tableSize = dummyForTableSize->FetchFrom(directoryFile, 1); ///Y obtengp eÃ± tamaÃ±o original
        delete dummyForTableSize;

        directorySize = tableSize;
        delete [] nameCopy;
    }

    filesysCreateLock->Release();
    delete dir;
    return success;
}


bool
FileSystem::CreateDir(const char *name)
{
    ASSERT(name != nullptr);

    DEBUG('f', "Create DIR %s\n", name);

    bool success = true;
    bool changeDir = false;

    filesysCreateLock->Acquire(); /// Tomo el lockk de creacion
    OpenFile* directoryFileBackup;
    char* nameCopy;
    if(name[0] == '/') {
        if(name[1] == '\0') { ///Similar a Create No Puedo crear en al raiz
            filesysCreateLock->Release();
            return false;
        }

        nameCopy = new char[FILE_PATH_MAX_LEN];
        sprintf(nameCopy, "%s", name);

        int lastSlashFound = LastSlash(name);

        nameCopy[lastSlashFound] = '\0';

        directoryFileBackup = directoryFile;

        if(!( (strlen(nameCopy) == 0 && ChangeDir("/", false)) || (strlen(nameCopy) > 0 && ChangeDir(nameCopy, false)) )){
            delete [] nameCopy;
            filesysCreateLock->Release();
            return false;
        }
        name = &nameCopy[lastSlashFound + 1];
        changeDir = true;
    }

    Directory *dir = new Directory(directorySize); ///Como tengo que crear un dir que esta mas abajo en la jerarquia tengo que guardar mipos para volver
    dir->FetchFrom(directoryFile);

    if (dir->Find(name) != -1)
        success = false;
    else {
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);

        int sector = freeMap->Find();
        if (sector == -1)
            success = false;

        if(!dir->Add(name, sector, true)) {
            success = false;
        } else if(success) {

            FileHeader *dirHeader = new FileHeader;
            dirHeader->GetRaw()->nextFileHeader = 0;
            dirHeader->GetRaw()->numBytes = 0;
            dirHeader->GetRaw()->numSectors = 0;

            success = dirHeader->Allocate(freeMap, DIRECTORY_FILE_SIZE);

            if(!success){
                DEBUG('f', "There is not enough space for a new directory\n");
                delete freeMap;
                delete dirHeader;
                delete dir;
                filesysCreateLock->Release();
                return false;
            }

            dirHeader->WriteBack(sector);
            freeMap->WriteBack(freeMapFile);

            // Create the new directory
            Directory  *newDir = new Directory(NUM_DIR_ENTRIES); ///Creo el nuevo dir
            OpenFile* newDirectoryFile = new OpenFile(sector);

            newDir->WriteBack(newDirectoryFile, true);///Flush de la tabla de entry

            dir->WriteBack(directoryFile);///Actualizo co la nueva tabla

            tableDeArchivosAbierta[sector]->removed = false;
            tableDeArchivosAbierta[sector]->removing = false;
            tableDeArchivosAbierta[sector]->removeLock = new Lock("Remove Lock");
            tableDeArchivosAbierta[sector]->writeLock = nullptr;
            tableDeArchivosAbierta[sector]->closeLock = nullptr;
            tableDeArchivosAbierta[sector]->count = 0;

            DEBUG('f',"Creation Dir ok!\n");
            delete dirHeader;
        }
        delete freeMap;
    }

    if(changeDir) {
        OpenFile* tmpOpen = directoryFile;
        directoryFile = directoryFileBackup;
        delete tmpOpen;

        Directory *dummyForTableSize = new Directory(MAX_DIR_ENTRIES);
        unsigned tableSize = dummyForTableSize->FetchFrom(directoryFile, 1);
        delete dummyForTableSize;

        directorySize = tableSize;
        delete [] nameCopy;
    }

    filesysCreateLock->Release();
    delete dir;

    return success;
}


unsigned int
FileSystem::SeparateDir(const char *name, char buffer[MAX_DIR_LEVEL][FILE_PATH_MAX_LEN]) const
{
	char nameCopy[FILE_PATH_MAX_LEN];
	sprintf(nameCopy, "%s", name);

	char* threadSafe;
	char* token = strtok_r(nameCopy, "/", &threadSafe);

	sprintf(buffer[0], "%s", token);

	int count;
	for(count = 1; token != NULL && count < MAX_DIR_LEVEL; count++) {
		token = strtok_r(NULL, "/", &threadSafe);
		sprintf(buffer[count], "%s", token);
	}

	return count;
}

bool
FileSystem::ChangeDirRootPath(const char* name, bool onlyChange)
{
	char splittedName[MAX_DIR_LEVEL][FILE_PATH_MAX_LEN];
	unsigned int count = SeparateDir(name + 1, splittedName);
	ASSERT(count > 0);

	OpenFile* backupDirFile = directoryFile;
	unsigned backupDirSize = directorySize;

	DEBUG('f', "The directory count is: %d\n", count);

	OpenFile* rootDirFile = new OpenFile(DIRECTORY_SECTOR); ///Cargamos el root

	Directory *dummyForRootTableSize = new Directory(MAX_DIR_ENTRIES);
	unsigned tableSize = dummyForRootTableSize->FetchFrom(rootDirFile, 1);
	delete dummyForRootTableSize;

	Directory* currDir = new Directory(tableSize);
	currDir->FetchFrom(rootDirFile);
/////////////////////INICIO_VIAJE_POR_EL_PATH///////////////
	directoryFile = rootDirFile;
	directorySize = tableSize;
	for(int i = 0; i < count - 1; i++) {
		DEBUG('f',"Directory to search: %s\n", splittedName[i]);
		if (!currDir->FindDir(splittedName[i])) { ///El directorio especificado en alguno de los token no existe
			delete currDir;

			directoryFile = backupDirFile;
			directorySize = backupDirSize;
			return false;
		}
		///Si el dir en el splitteado existe tengo que entrar en el

		OpenFile *tmpOpen = directoryFile; ///Cargo el siguiente dir que me lo da splittedname
		directoryFile = Open(splittedName[i]);///Abrimos el siguiente dir para buscar en el, al final de todo son arch
		delete tmpOpen;

		Directory *dummyForTableSize = new Directory(MAX_DIR_ENTRIES);
		tableSize = dummyForTableSize->FetchFrom(directoryFile, 1);
		delete dummyForTableSize;

		directorySize = tableSize;

		Directory *tmpDir = currDir; ///Avanzamo al siguiente dir

		currDir = new Directory(tableSize);
		currDir->FetchFrom(directoryFile);

		delete tmpDir;
	}

	delete currDir;
	if (onlyChange) {
		delete backupDirFile;
	}
	return true;
}

bool
FileSystem::ChangeDirRelativePath(const char* name, bool onlyChange)
{
	Directory *dir = new Directory(directorySize);
	dir->FetchFrom(directoryFile);

	if (!dir->FindDir(name)) {
		DEBUG('f', "The directory does not exists...\n");
		delete dir;
		return false;
	}

	OpenFile *currentDirectoryFile = Open(name);
	OpenFile *openFileToDelete = directoryFile;

	directoryFile = currentDirectoryFile;

	delete openFileToDelete;

	Directory *dummyForTableSize = new Directory(MAX_DIR_ENTRIES);
	unsigned tableSize = dummyForTableSize->FetchFrom(directoryFile, 1);
	directorySize = tableSize;
	delete dummyForTableSize;
	delete dir;
	return true;
}

bool
FileSystem::ChangeDir(const char* name, bool onlyChange)
{///CD, con onlyChange, forzamos que se tenga que eliminar el dir backup para no volver
    ASSERT(name != nullptr);

    if(name[0] == '/')
        return ChangeDirRootPath(name, onlyChange);
    else ///Si no es el ./ entonces copio el current dir
	    return ChangeDirRelativePath(name, onlyChange);
}

void
FileSystem::PrintDir() {
    Directory  *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);
    dir->Print();
}

unsigned
FileSystem::Ls(char* into) {
    Directory  *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);
    unsigned bytesRead = dir->PrintNames(into);
    return bytesRead;
}


OpenFile*
FileSystem::GetFreeDirEntries() {
    return freeMapFile;
};

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *
FileSystem::Open(const char *name)
{
	ASSERT(name != nullptr);

    Directory *dir = new Directory(directorySize);
    OpenFile  *openFile = nullptr;

    dir->FetchFrom(directoryFile);
    int sector = dir->Find(name);

    if (sector >= 0 && !tableDeArchivosAbierta[sector]->removing)
        openFile = new OpenFile(sector);

    //delete dir;

    return openFile;
}

unsigned
FileSystem::GetDirectorySize() {
    return directorySize;
}

void
FileSystem::SetDirectorySize(unsigned newDirectorySize) {
    directorySize = newDirectorySize;
}

Directory *
FileSystem::Find(const char *name) const
{
	Directory *dir = new Directory(directorySize); ///Cargo el dir
	dir->FetchFrom(directoryFile);

	if(dir->FindDir(name)) {  ///No es un archivo
		delete dir; ///Como no esta el file, chau todo
		return NULL;
	}

	return dir;
}


/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool
FileSystem::Remove(const char *name) {
	ASSERT(name != nullptr);

	bool changeDir = false;
	OpenFile *directoryFileBackup;
	char *nameCopy;
	if (name[0] == '/') {
		if (name[1] == '\0') { ///Similar a Create No Puedo crear en al raiz
			return false;
		}

		nameCopy = new char[FILE_PATH_MAX_LEN];
		sprintf(nameCopy, "%s", name);

		int lastSlashFound = LastSlash(name);

		nameCopy[lastSlashFound] = '\0';

		directoryFileBackup = directoryFile;

		if (!((strlen(nameCopy) == 0 && ChangeDir("/", false)) ||
		      (strlen(nameCopy) > 0 && ChangeDir(nameCopy, false)))) {
			delete[] nameCopy;
			return false;
		}
		name = &nameCopy[lastSlashFound + 1];
		changeDir = true;
	}

	///

	Directory *dir = new Directory(directorySize);///Copio mi posicion el current
	dir->FetchFrom(directoryFile);
	const int sector = dir->Find(name);

	if (sector == -1) { ///No se encuentra el file en el current file
		Directory *dummyForTableSize = new Directory(MAX_DIR_ENTRIES);
		delete dir;
		DEBUG('f', "No sector for directory %s", name);

		///____________________
		if (changeDir) {
			OpenFile *tmpOpen = directoryFile;
			directoryFile = directoryFileBackup;
			delete tmpOpen;

			unsigned tableSize2 = dummyForTableSize->FetchFrom(directoryFile, 1);
			delete dummyForTableSize;

			directorySize = tableSize2;
			delete[] nameCopy;
		}
		///____________________

		return false;
	}
	Directory *dummyForTableSize = new Directory(MAX_DIR_ENTRIES);

	///Si llegamos aca entonces debo eliminar el archivo por lo tanto tomar el removeLock
	if (tableDeArchivosAbierta[sector]->removeLock != nullptr)
		tableDeArchivosAbierta[sector]->removeLock->Acquire();

	if (tableDeArchivosAbierta[sector]->removed) { ///si ya estaba removido segun la tabla de archivos abierta chau todo
		delete dir;

		///____________________
		if (changeDir) {
			OpenFile *tmpOpen = directoryFile;
			directoryFile = directoryFileBackup;
			delete tmpOpen;

			unsigned tableSize2 = dummyForTableSize->FetchFrom(directoryFile, 1);
			delete dummyForTableSize;

			directorySize = tableSize2;
			delete[] nameCopy;
		}
		///____________________
		tableDeArchivosAbierta[sector]->removeLock->Release();
		DEBUG('f', "Already deleted %s", name);
		return false;
	}

	FileHeader *fileH = new FileHeader;
	fileH->FetchFrom(sector);

	Bitmap *freeMap = new Bitmap(NUM_SECTORS);
	freeMap->FetchFrom(freeMapFile);

	tableDeArchivosAbierta[sector]->removing = true; ///REMOVIENDO EL FILE
	tableDeArchivosAbierta[sector]->removerSpaceId = currentThread->tId;

	if (tableDeArchivosAbierta[sector]->count > 0) { ///Tengo que esperar parapor el hilo que tiene abierto el archivo
		int *msg = new int;
		currentThread->removeChannel->Receive(msg);
		DEBUG('f', "Received %d\n", *msg);
		ASSERT(!*msg);
		delete msg;///Recibo que termino el hilo que usaba el file, joya
	}
	///Tengo que liberar los sectores de memoria que ocupaba el archivo y el file header
	fileH->Deallocate(freeMap);
	freeMap->Clear(sector);
	unsigned nextSector = fileH->GetRaw()->nextFileHeader;
	while (nextSector) {
		fileH->FetchFrom(nextSector);
		fileH->Deallocate(freeMap);
		freeMap->Clear(nextSector);
		nextSector = fileH->GetRaw()->nextFileHeader;
	}

	dir->Remove(name);

	freeMap->WriteBack(freeMapFile);  ///Actualizo luego de liberar el bloque de direntries libres
	dir->WriteBack(directoryFile);

	tableDeArchivosAbierta[sector]->removed = true; ///El archivo fue removido
	///Como fue removido entonces tengo que soltar el lock de remocion
	if (tableDeArchivosAbierta[sector]->removeLock != nullptr)
		tableDeArchivosAbierta[sector]->removeLock->Release();

	delete fileH;
	delete dir;
	delete freeMap;

	///Como el archivo fue removido, no esta en proceso de remocion
	tableDeArchivosAbierta[sector]->removing = false;

	///____________________
	if (changeDir) {
		OpenFile *tmpOpen = directoryFile;
		directoryFile = directoryFileBackup;
		delete tmpOpen;

		unsigned tableSize2 = dummyForTableSize->FetchFrom(directoryFile, 1);
		delete dummyForTableSize;

		directorySize = tableSize2;
		delete[] nameCopy;
	}
	///____________________

	DEBUG('f', "File remove OK\n");
	return true;
}

/// List files
void
FileSystem::List()
{
    Directory *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);
    dir->List();
    delete dir;
}

static bool
AddToShadowBitmap(unsigned sector, Bitmap *map)
{
    ASSERT(map != nullptr);

    if (map->Test(sector)) {
        DEBUG('f', "Sector %u was already marked.\n", sector);
        return false;
    }
    map->Mark(sector);
    DEBUG('f', "Marked sector %u.\n", sector);
    return true;
}

static bool
CheckForError(bool value, const char *message)
{
    if (!value) {
        DEBUG('f', "Error: %s\n", message);
    }
    return !value;
}

static bool
CheckSector(unsigned sector, Bitmap *shadowMap)
{
    if (CheckForError(sector < NUM_SECTORS,
                      "sector number too big.  Skipping bitmap check.")) {
        return true;
    }
    return CheckForError(AddToShadowBitmap(sector, shadowMap),
                         "sector number already used.");
}

static bool
CheckFileHeader(const RawFileHeader *rh, unsigned num, Bitmap *shadowMap)
{
    ASSERT(rh != nullptr);

    bool error = false;

    DEBUG('f', "Checking file header %u.  File size: %u bytes, number of sectors: %u.\n",
          num, rh->numBytes, rh->numSectors);
    error |= CheckForError(rh->numSectors >= DivRoundUp(rh->numBytes,
                                                        SECTOR_SIZE),
                           "sector count not compatible with file size.");
    error |= CheckForError(rh->numSectors < NUM_DIRECT,
                           "too many blocks.");
    for (unsigned i = 0; i < rh->numSectors; i++) {
        unsigned s = rh->dataSectors[i];
        error |= CheckSector(s, shadowMap);
    }
    return error;
}

static bool
CheckBitmaps(const Bitmap *freeMap, const Bitmap *shadowMap)
{
    bool error = false;
    for (unsigned i = 0; i < NUM_SECTORS; i++) {
        DEBUG('f', "Checking sector %u. Original: %u, shadow: %u.\n",
              i, freeMap->Test(i), shadowMap->Test(i));
        error |= CheckForError(freeMap->Test(i) == shadowMap->Test(i),
                               "inconsistent bitmap.");
    }
    return error;
}

static bool
CheckDirectory(const RawDirectory *rd, Bitmap *shadowMap)
{
    ASSERT(rd != nullptr);
    ASSERT(shadowMap != nullptr);

    bool error = false;
    unsigned nameCount = 0;
    const char *knownNames[NUM_DIR_ENTRIES];

    for (unsigned i = 0; i < NUM_DIR_ENTRIES; i++) {
        DEBUG('f', "Checking direntry: %u.\n", i);
        const DirectoryEntry *e = &rd->table[i];

        if (e->inUse) {
            if (strlen(e->name) > FILE_NAME_MAX_LEN) {
                DEBUG('f', "Filename too long.\n");
                error = true;
            }

            // Check for repeated filenames.
            DEBUG('f', "Checking for repeated names.  Name count: %u.\n",
                  nameCount);
            bool repeated = false;
            for (unsigned j = 0; j < nameCount; j++) {
                DEBUG('f', "Comparing \"%s\" and \"%s\".\n",
                      knownNames[j], e->name);
                if (strcmp(knownNames[j], e->name) == 0) {
                    DEBUG('f', "Repeated filename.\n");
                    repeated = true;
                    error = true;
                }
            }
            if (!repeated) {
                knownNames[nameCount] = e->name;
                DEBUG('f', "Added \"%s\" at %u.\n", e->name, nameCount);
                nameCount++;
            }

            // Check sector.
            error |= CheckSector(e->sector, shadowMap);

            // Check file header.
            FileHeader *h = new FileHeader;
            const RawFileHeader *rh = h->GetRaw();
            h->FetchFrom(e->sector);
            error |= CheckFileHeader(rh, e->sector, shadowMap);
            delete h;
        }
    }
    return error;
}

bool
FileSystem::Check()
{
    DEBUG('f', "Performing filesystem check\n");
    bool error = false;

    Bitmap *shadowMap = new Bitmap(NUM_SECTORS);
    shadowMap->Mark(FREE_MAP_SECTOR);
    shadowMap->Mark(DIRECTORY_SECTOR);

    DEBUG('f', "Checking bitmap's file header.\n");

    FileHeader *bitH = new FileHeader;
    const RawFileHeader *bitRH = bitH->GetRaw();
    bitH->FetchFrom(FREE_MAP_SECTOR);
    DEBUG('f', "  File size: %u bytes, expected %u bytes.\n"
               "  Number of sectors: %u, expected %u.\n",
          bitRH->numBytes, FREE_MAP_FILE_SIZE,
          bitRH->numSectors, FREE_MAP_FILE_SIZE / SECTOR_SIZE);
    error |= CheckForError(bitRH->numBytes == FREE_MAP_FILE_SIZE,
                           "bad bitmap header: wrong file size.");
    error |= CheckForError(bitRH->numSectors == FREE_MAP_FILE_SIZE / SECTOR_SIZE,
                           "bad bitmap header: wrong number of sectors.");
    error |= CheckFileHeader(bitRH, FREE_MAP_SECTOR, shadowMap);
    delete bitH;

    DEBUG('f', "Checking directory.\n");

    FileHeader *dirH = new FileHeader;
    const RawFileHeader *dirRH = dirH->GetRaw();
    dirH->FetchFrom(DIRECTORY_SECTOR);
    error |= CheckFileHeader(dirRH, DIRECTORY_SECTOR, shadowMap);
    delete dirH;

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);
    Directory *dir = new Directory(directorySize);
    const RawDirectory *rdir = dir->GetRaw();
    dir->FetchFrom(directoryFile);
    error |= CheckDirectory(rdir, shadowMap);
    delete dir;

    // The two bitmaps should match.
    DEBUG('f', "Checking bitmap consistency.\n");
    error |= CheckBitmaps(freeMap, shadowMap);
    delete shadowMap;
    delete freeMap;

    DEBUG('f', error ? "Filesystem check failed.\n"
                     : "Filesystem check succeeded.\n");

    return !error;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void
FileSystem::Print()
{
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);
    FileHeader *bitH    = new FileHeader;
    FileHeader *dirH    = new FileHeader;
    Directory  *dir     = new Directory(directorySize);
    dir->FetchFrom(directoryFile);

    printf("--------------------------------\n");
    bitH->FetchFrom(FREE_MAP_SECTOR);
    bitH->Print("Bitmap");

    printf("--------------------------------\n");
    dirH->FetchFrom(DIRECTORY_SECTOR);
    dirH->Print("Directory");

    printf("--------------------------------\n");
    freeMap->Print();

    printf("--------------------------------\n");
    dir->Print();
    printf("--------------------------------\n");

    delete bitH;
    delete dirH;
    delete freeMap;
    delete dir;
}
