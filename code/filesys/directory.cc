/// Routines to manage a directory of file names.
///
/// The directory is a table of fixed length entries; each entry represents a
/// single file, and contains the file name, and the location of the file
/// header on disk.  The fixed size of each directory entry means that we
/// have the restriction of a fixed maximum size for file names.
///
/// The constructor initializes an empty directory of a certain size; we use
/// ReadFrom/WriteBack to fetch the contents of the directory from disk, and
/// to write back any modifications back to disk.
///
/// Also, this implementation has the restriction that the size of the
/// directory cannot expand.  In other words, once all the entries in the
/// directory are used, no more files can be created.  Fixing this is one of
/// the parts to the assignment.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "directory.hh"
#include "directory_entry.hh"
#include "file_header.hh"
#include "lib/utility.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>


/// Initialize a directory; initially, the directory is completely empty.  If
/// the disk is being formatted, an empty directory is all we need, but
/// otherwise, we need to call FetchFrom in order to initialize it from disk.
///
/// * `size` is the number of entries in the directory.
Directory::Directory(unsigned size)
{
    ASSERT(size > 0);
    raw.table = new DirectoryEntry [size];
    raw.tableSize = size;
    for (unsigned i = 0; i < raw.tableSize; i++) {
        raw.table[i].inUse = false;
        raw.table[i].isDirectory = false;
    }
}

/// De-allocate directory data structure.
Directory::~Directory()
{
    delete [] raw.table;
}

/// Read the contents of the directory from disk.
///
/// * `file` is file containing the directory contents.
unsigned
Directory::FetchFrom(OpenFile *file, bool cantDirEntries)
{
    ASSERT(file != nullptr);
    ///Leo el la table del dir del disco y debuelvo su size

    int result = file->Read((char *) raw.table, raw.tableSize * sizeof (DirectoryEntry), true);

    if(!cantDirEntries)
        return (unsigned)result;

    ///Si necesito las cantidad de entries de esa tabla, divido result por el sizeof de una dir entry
    raw.tableSize = unsigned((unsigned) result/ sizeof (DirectoryEntry));
    return raw.tableSize;

}

/// Write any modifications to the directory back to disk.
///
/// * `file` is a file to contain the new directory contents.
void
Directory::WriteBack(OpenFile *file, bool fstBack)
{
    ASSERT(file != nullptr);
    if(fstBack){
        file->WriteAt((char *) raw.table,raw.tableSize * sizeof (DirectoryEntry), 0);
        return;
    }
    unsigned bytesToWrite = raw.tableSize * sizeof (DirectoryEntry);///Necesito guardar o escribir en disco todo el tamaño de la table * tamaño de dir entry
    file->Write((char *) raw.table, bytesToWrite, true);
}

/// Look up file name in directory, and return its location in the table of
/// directory entries.  Return -1 if the name is not in the directory.
///
/// * `name` is the file name to look up.
int
Directory::FindIndex(const char *name)
{
    ASSERT(name != nullptr);

    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse && !strncmp(raw.table[i].name, name, FILE_NAME_MAX_LEN)) {
            return i;
        }
    }
    return -1;
}

/// Look up file name in directory, and return the disk sector number where
/// the file's header is stored.  Return -1 if the name is not in the
/// directory.
///
/// * `name` is the file name to look up.
int
Directory::Find(const char *name)
{
    ASSERT(name != nullptr);
    int index = FindIndex(name);
    if (index != -1)
        return raw.table[index].sector;

    return -1;
}

bool
Directory::FindDir(const char *name)
{
    ASSERT(name != nullptr);
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse)
            if(!strncmp(raw.table[i].name, name, FILE_NAME_MAX_LEN)&& raw.table[i].isDirectory)
                return true;
    }
    return false;
}


/// Add a file into the directory.  Return true if successful; return false
/// if the file name is already in the directory, or if the directory is
/// completely full, and has no more space for additional file names.
///
/// * `name` is the name of the file being added.
/// * `newSector` is the disk sector containing the added file's header.
bool
Directory::Add(const char *name, int newSector, bool isDirectory)
{
    ASSERT(name != nullptr);

    if (FindIndex(name) != -1)
        return false;

    unsigned i = 0;
    for (; i < raw.tableSize; i++) {
        if (!raw.table[i].inUse) { ///Si tengo una entrada libre en mi table de dir
            raw.table[i].inUse = true; ///La marco como en uso

            if(isDirectory)
                raw.table[i].isDirectory = true;

            strncpy(raw.table[i].name, name, FILE_NAME_MAX_LEN);///Asigno a la tabla el archivo, sea o no un directorio y le doy un sector
            raw.table[i].sector = newSector;
            return true;
        }
    }

    ///Resize
    unsigned newTableSize = raw.tableSize + 1;
    DirectoryEntry* newTable = new DirectoryEntry[newTableSize]; ///Creamos la nueva tabla

    memcpy(newTable, raw.table, raw.tableSize * sizeof(DirectoryEntry)); ///Copiamos la memoria de la vieja a la nueva

    ///Piso lo viejo con lo nuevo
    raw.table =  newTable; ///Nueva table
    raw.tableSize = newTableSize;///Nuevo tamaño

    ///Podemos usar el indice de entrada a tabla "i" para adicionar la entrada faltante ADD
    raw.table[i].inUse = true;
    if(isDirectory)
        raw.table[i].isDirectory = true;

    strncpy(raw.table[i].name, name, FILE_NAME_MAX_LEN);
    raw.table[i].sector = newSector; ///Adiciono el file a la tabla y le asigno su sector

    fileSystem->SetDirectorySize(newTableSize); ///Actualizo en el filesys con el tamaño de la nueva tabla

    return true;
}


/// Remove a file name from the directory.   Return true if successful;
/// return false if the file is not in the directory.
///
/// * `name` is the file name to be removed.
bool
Directory::Remove(const char *name)
{
    ASSERT(name != nullptr);

    int indice = FindIndex(name);
    if (indice == -1) {
        return false;
    }
    raw.table[indice].inUse = false;
    raw.table[indice].isDirectory = false;
    return true;
}

/// List all the file names in the directory.
void
Directory::List() const
{
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            printf("%s\n", raw.table[i].name);
        }
    }
}

/// List all the file names in the directory, their `FileHeader` locations,
/// and the contents of each file.  For debugging.
void
Directory::Print() const
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            printf("\nDirectory entry:\n"
                   "    name: %s\n"
                   "    sector: %u\n",
                   raw.table[i].name, raw.table[i].sector);
            hdr->FetchFrom(raw.table[i].sector);
            hdr->Print(nullptr);

            unsigned nextSector = hdr->GetRaw()->nextFileHeader;
            while(nextSector) { ///Print de la lista enlazada de FH
                hdr->FetchFrom(nextSector);
                hdr->Print(nullptr);
                nextSector = hdr->GetRaw()->nextFileHeader;
            }
        }
    }
    printf("\n");
    delete hdr;
}
///LS(?
unsigned
Directory::PrintNames(char* into) const
{
    char* temp = into;
    unsigned result = 0;
    unsigned b = 0;

    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            b++;
            if(raw.table[i].isDirectory ) {
                sprintf(temp, "%s/\n", raw.table[i].name);

                unsigned bytesToAdd = strlen(raw.table[i].name) + 2;
                temp+= bytesToAdd;
                result += bytesToAdd;
            }
            else {
                sprintf(temp, "%s\n", raw.table[i].name);

                unsigned bytesToAdd = strlen(raw.table[i].name) + 1;
                temp+= bytesToAdd;
                result += bytesToAdd;
            }
        }
    }

    if(b == 0){
        sprintf(temp,"\n");
        temp += 1;
        result += 1;
    }

    return result;
}

const RawDirectory *
Directory::GetRaw() const
{
    return &raw;
}
