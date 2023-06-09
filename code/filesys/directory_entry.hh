/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_DIRECTORYENTRY__HH
#define NACHOS_FILESYS_DIRECTORYENTRY__HH


/// For simplicity, we assume file names are <= 64 characters long.
const unsigned FILE_NAME_MAX_LEN = 15;

///Maxima profundidad de Path
const unsigned MAX_DIR_LEVEL = 10;

///Tama�o Calculado del espacio entre la profundidad y el Level
const unsigned FILE_PATH_MAX_LEN = FILE_NAME_MAX_LEN * MAX_DIR_LEVEL;

/// The following class defines a "directory entry", representing a file in
/// the directory.  Each entry gives the name of the file, and where the
/// file's header is to be found on disk.
///
/// Internal data structures kept public so that Directory operations can
/// access them directly.
class DirectoryEntry {
public:
    /// Is this directory entry in use?
    bool inUse;
    /// Location on disk to find the `FileHeader` for this file.
    unsigned sector;
    /// Text name for file, with +1 for the trailing `'\0'`.
    char name[FILE_NAME_MAX_LEN + 1];

    bool isDirectory; ///Para saber si estoy operando sobre un dir o no
};


#endif
