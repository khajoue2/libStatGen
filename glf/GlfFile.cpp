/*
 *  Copyright (C) 2010  Regents of the University of Michigan
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdexcept>
#include <stdlib.h>
#include "GlfFile.h"
#include "GlfException.h"

// Constructor, init variables.
GlfFile::GlfFile()
    : myFilePtr(NULL)
{
    resetFile();
}


// Constructor, init variables and open the specified file based on the
// specified mode (READ/WRITE).  Default is READ..
GlfFile::GlfFile(const char* filename, OpenType mode)
    : myFilePtr(NULL)
{
    resetFile();

    bool openStatus = true;
    if(mode == READ)
    {
        // open the file for read.
        openStatus = openForRead(filename);
    }
    else
    {
        // open the file for write.
        openStatus = openForWrite(filename);
    }
    if(!openStatus)
    {
        // Failed to open the file - print error and abort.
        fprintf(stderr, "%s\n", getStatusMessage());
        std::cerr << "FAILURE - EXITING!!!" << std::endl;
        exit(-1);
    }
}

GlfFile::~GlfFile()
{
    resetFile();
}


// Open a glf file for reading with the specified filename.
bool GlfFile::openForRead(const char * filename)
{
    // Reset for any previously operated on files.
    resetFile();

    myFilePtr = ifopen(filename, "rb");
   
    if (myFilePtr == NULL)
    {
        std::string errorMessage = "Failed to Open ";
        errorMessage += filename;
        errorMessage += " for reading";
        myStatus.setStatus(GlfStatus::FAIL_IO, errorMessage.c_str());
        throw(GlfException(myStatus));
        return(false);
    }

    myIsOpenForRead = true;
    // Successfully opened the file.
    myStatus = GlfStatus::SUCCESS;
    return(true);
}


// Open a glf file for reading with the specified filename and read the
// header into the specified header.
bool GlfFile::openForRead(const char * filename, GlfHeader& header)
{
    if(!openForRead(filename))
    {
        return(false);
    }

    // Read the header
    if(!readHeader(header))
    {
        return(false);
    }
    return(true);
}


// Open a glf file for writing with the specified filename.
bool GlfFile::openForWrite(const char * filename)
{
    // Reset for any previously operated on files.
    resetFile();

    myFilePtr = ifopen(filename, "wb", InputFile::BGZF);
    
    if (myFilePtr == NULL)
    {
        std::string errorMessage = "Failed to Open ";
        errorMessage += filename;
        errorMessage += " for writing";
        myStatus.setStatus(GlfStatus::FAIL_IO, errorMessage.c_str());
        throw(GlfException(myStatus));
        return(false);
    }
   
    myIsOpenForWrite = true;

    // Successfully opened the file.
    myStatus = GlfStatus::SUCCESS;
    return(true);
}


// Close the file if there is one open.
void GlfFile::close()
{
    // Resetting the file will close it if it is open, and
    // will reset all other variables.
    resetFile();
}


// Returns whether or not the end of the file has been reached.
// return: int - true = EOF; false = not eof.
bool GlfFile::isEOF()
{
    if (myFilePtr != NULL)
    {
        // File Pointer is set, so return if eof.
        return(ifeof(myFilePtr));
    }
    // File pointer is not set, so return true, eof.
    return true;
}


// Read the header from the currently opened file.
bool GlfFile::readHeader(GlfHeader& header)
{
    if(myIsOpenForRead == false)
    {
        // File is not open for read
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read header since the file is not open for reading");
        throw(GlfException(myStatus));
        return(false);
    }

    if(myHasHeader == true)
    {
        // The header has already been read.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read header since it has already been read.");
        throw(GlfException(myStatus));
        return(false);
    }

    if(header.read(myFilePtr))
    {
        // The header has now been successfully read.
        myHasHeader = true;
        myStatus = GlfStatus::SUCCESS;
        return(true);
    }
    myStatus.setStatus(GlfStatus::UNKNOWN, 
                       "Failed to read the header.");
    throw(GlfException(myStatus));
    return(false);
}


// Write the header to the currently opened file.
bool GlfFile::writeHeader(GlfHeader& header)
{
    if(myIsOpenForWrite == false)
    {
        // File is not open for write
        // -OR-
        // The header has already been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write header since the file is not open for writing");
        throw(GlfException(myStatus));
        return(false);
    }

    if(myHasHeader == true)
    {
        // The header has already been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write header since it has already been written");
        throw(GlfException(myStatus));
        return(false);
    }

    if(header.write(myFilePtr))
    {
        // The header has now been successfully written.
        myHasHeader = true;
        myStatus = GlfStatus::SUCCESS;
        return(true);
    }

    // return the status.
    myStatus.setStatus(GlfStatus::UNKNOWN, 
                       "Failed to write the header.");
    throw(GlfException(myStatus));
    return(false);
}


// Gets the next reference section from the file & stores it in the
// passed in section.  It will read until a new section is found.
bool GlfFile::getNextRefSection(GlfRefSection& refSection)
{
    if(myIsOpenForRead == false)
    {
        // File is not open for read
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read reference section since the file is not open for reading");
        throw(GlfException(myStatus));
        return(false);
    }

    if(myHasHeader == false)
    {
        // The header has not yet been read.
        // TODO - maybe just read the header.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read reference section since the header has not been read.");
        throw(GlfException(myStatus));
        return(false);
    }

    // Check for end of file.  If end of file, return false.
    if(isEOF())
    {
        return(false);
    }
    // TODO - move to the section if not in the right spot...

    // Ready to read the section:
    if(refSection.read(myFilePtr))
    {
        myStatus = GlfStatus::SUCCESS;
        myHasRefSection = true;
        return(true);
    }

    // If it is the EOF, just return false.
    if(isEOF())
    {
        return(false);
    }
    myStatus.setStatus(GlfStatus::UNKNOWN, 
                       "Failed reading a reference section from the file.");
    throw(GlfException(myStatus));
    return(false);
}


// Write the reference section to the file.
bool GlfFile::writeRefSection(const GlfRefSection& refSection)
{
    if(myIsOpenForWrite == false)
    {
        // File is not open for write
        // -OR-
        // The header has already been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write reference section since the file is not open for writing");
        throw(GlfException(myStatus));
        return(false);
    }

    if(myHasHeader == false)
    {
        // The header has not been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write reference section since the header has not been written");
        throw(GlfException(myStatus));
       return(false);
    }

    if(refSection.write(myFilePtr))
    {
        myStatus = GlfStatus::SUCCESS;
        // A reference section has now been successfully written.
        myHasRefSection = true;
        return(true);
    }

    // return the status.
    myStatus.setStatus(GlfStatus::UNKNOWN, 
                       "Failed writing a reference section to the file.");
    throw(GlfException(myStatus));
    return(false);    
}


// Gets the next reference section from the file & stores it in the
// passed in record.
bool GlfFile::getNextRecord(GlfRecord& record)
{
    if(myIsOpenForRead == false)
    {
        // File is not open for read
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read reference section since the file is not open for reading");
        throw(GlfException(myStatus));
        return(false);
    }

    if(myHasHeader == false)
    {
        // The header has not yet been read.
        // TODO - maybe just read the header.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read reference section since the header has not been read.");
        throw(GlfException(myStatus));
        return(false);
    }
    
    if(myHasRefSection == false)
    {
        // The reference section has not yet been read.
        // TODO - maybe just read the reference section.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot read record since a reference section has not been read.");
        throw(GlfException(myStatus));
        return(false);
    }

    // Read the record.
    if(record.read(myFilePtr))
    {
        myStatus = GlfStatus::SUCCESS;
        if(record.getRecordType() != 0)
        {
            return(true);
        }
        else
        {
            // Not an error, so no exception thrown, but no more records.
            return(false);
        }
    }
    
    myStatus.setStatus(GlfStatus::UNKNOWN, 
                       "Failed reading a record from the file.");
    throw(GlfException(myStatus));
    return(false);
}


// Write the reference section to the file.
bool GlfFile::writeRecord(const GlfRecord& record)
{
    if(myIsOpenForWrite == false)
    {
        // File is not open for write
        // -OR-
        // The header has already been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write record since the file is not open for writing");
        throw(GlfException(myStatus));
       return(false);
    }

    if(myHasHeader == false)
    {
        // The header has not been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write record since the header has not been written");
        throw(GlfException(myStatus));
        return(false);
    }

    if(myHasRefSection == false)
    {
        // The header has not been written.
        myStatus.setStatus(GlfStatus::FAIL_ORDER, 
                           "Cannot write record since a reference section has not been written");
        throw(GlfException(myStatus));
        return(false);
    }

    if(record.write(myFilePtr))
    {
        myStatus = GlfStatus::SUCCESS;
        // The record has now been successfully written.
        return(true);
    }

    // return the status.
    myStatus.setStatus(GlfStatus::UNKNOWN, 
                       "Failed writing a record to the file.");
    throw(GlfException(myStatus));
    return(false);    
}


// Return the number of records that have been read/written so far.
uint32_t GlfFile::getCurrentRecordCount()
{
    return(myRecordCount);
}


// Reset variables for each file.
void GlfFile::resetFile()
{
    // Close the file.
    if (myFilePtr != NULL)
    {
        // If we already have an open file, close it.
        ifclose(myFilePtr);
        myFilePtr = NULL;
    }

    myIsOpenForRead = false;
    myIsOpenForWrite = false;
    myHasHeader = false;
    myRecordCount = 0;
    myStatus = GlfStatus::SUCCESS;
}


// Default Constructor.
GlfFileReader::GlfFileReader()
{
}


// Constructor that opens the specified file for read.
GlfFileReader::GlfFileReader(const char* filename)
{
    if(!openForRead(filename))
    {
        // Failed to open for reading - print error and abort.
        fprintf(stderr, "%s\n", getStatusMessage());
        std::cerr << "FAILURE - EXITING!!!" << std::endl;
        exit(-1);
    }
}


GlfFileReader::~GlfFileReader()
{
}


// Default Constructor.
GlfFileWriter::GlfFileWriter()
{
}


// Constructor that opens the specified file for write.
GlfFileWriter::GlfFileWriter(const char* filename)
{
    if(!openForWrite(filename))
    {
        // Failed to open for reading - print error and abort.
        fprintf(stderr, "%s\n", getStatusMessage());
        std::cerr << "FAILURE - EXITING!!!" << std::endl;
        exit(-1);
    }
}


GlfFileWriter::~GlfFileWriter()
{
}