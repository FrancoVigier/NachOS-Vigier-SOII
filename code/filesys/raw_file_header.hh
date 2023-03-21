/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_RAWFILEHEADER__HH
#define NACHOS_FILESYS_RAWFILEHEADER__HH

#include "machine/disk.hh"

static const unsigned NUM_DIRECT
  = ((SECTOR_SIZE - 2 * sizeof (int)) / sizeof (int)) - 1;

/**
*Al tener nivel de indireccion el tamaño maximo de archivo cambia (offset), Sabemos que para los 128bytes el
*Datasector es NUM_DIRECT - 2 (desde antes), cada fh tiene NUM_DIRECT -2 sectores... Entonces:
*En NUMSECTORS entran (NUM_SECTORS/NUM_DIRECT - 1) fileheaders (A)
*Los secttores data de un archivo son los NUM_SECTORES - A (B), basicamente es el total de sectores menos la cantidad de sectores que ocupan los FH
*El espacio en byte de (B) es B * SECTOR_SIZE, por lo tanto esta es EL TAMAÑO MAXIMO DE UN ARCHIVO, es el maximo espacio data que puede tener un file
*/
const unsigned MAX_FILE_SIZE = (NUM_SECTORS - int(NUM_SECTORS / (NUM_DIRECT + 1))) * SECTOR_SIZE;

const unsigned MAX_DIR_ENTRIES = unsigned(MAX_FILE_SIZE / 32); ///TableSize, Hacemos las indirecciones. Las Maximas entradas a table
///El 32 viene de 128kib / 4 kib
struct RawFileHeader {
    unsigned numBytes;  ///< Number of bytes in the file.
    unsigned numSectors;  ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                       ///< block in the file.
    unsigned nextFileHeader;

};
typedef struct RawFileHeader* RawFileHeaderNode;

#endif
