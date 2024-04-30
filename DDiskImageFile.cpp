
//#define DEBUG_OUT std::cerr
#define DEBUG_OUT dBug
#define ERR_OUT std::cerr
//#define DEBUG_OUT *aNullStream
//#define CONS_OUT std::cout
#define CONS_OUT *diskLibOutputStream
#define CONS_IN std::cin
//#define CONS_IN *inStream


#ifdef DEBUG_VERBOSE
#undef DEBUG_VERBOSE
#endif


#include "DDiskImageFile.h"


#ifdef ALIB_HAVE_UNISTD_H
#include <unistd.h>
#endif // ALIB_HAVE_UNISTD_H

#ifdef ALIB_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef ALIB_HAVE_STAT_H
#include <stat.h>
#endif

#ifdef ALIB_HAVE_DIRENT_H
#include <dirent.h>
#endif // ALIB_HAVE_DIRENT_H

#ifdef ALIB_HAVE_DIRECT_H
#include <direct.h>
#endif // ALIB_HAVE_DIRECT_H

#ifdef ALIB_HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif // ALIB_HAVE_SYS_DIR_H


#ifdef ASYS_MAC
#ifndef __GNUC__
#define ALIB_MAC_MOREFILES 1
#include <IterateDirectory.h>
#endif // __GNUC__
#endif // ASYS_MAC


////////////////////////////////////////////////////////////////////////////////
//  DDiskImageFile Class
////////////////////////////////////////////////////////////////////////////////

DDiskImageFile::DDiskImageFile(const char *fname) : DDiskImage(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageFile::DDiskImageFile(...,'"); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  cachesData=false;
  //Mount(fname);
}


DDiskImageFile::~DDiskImageFile()
{
  // TODO: flush files if cached
  freeDirCache();
  if(theFile) fclose(theFile);
  theFile=(FILE *)NULL;
  diskMounted=false;
}


/* STATIC */
bool DDiskImageFile::recognize(const char *fname)
{
  debug_msg("DDiskImageFile::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


/* STATIC */
bool DDiskImageFile::recognizeFileExtension(const char *fname)
{
  debug_msg("DDiskImageFile::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


const char *DDiskImageFile::guessSystem(const char *fname)
{
  debug_msg("DDiskImageFile::guessSystem('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return NULL;
}


void DDiskImageFile::init()
{
  DDiskImage::init();
  debug_msg("DDiskImageFile::init()\n");
  theFile=(FILE *)NULL;
  imageDataSize=0;
  theFileName=NULL;
}


bool DDiskImageFile::openTheFile()
{
  bool ret=false;
#ifdef USE_FAKE_CHDIR
#endif // USE_FAKE_CHDIR
  theFile=fopen(theFileName,"rb");
  if(theFile) ret=true;
  return ret;
}


bool DDiskImageFile::readBlock(UINT blk)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageFile::readBlock("); debug_int(blk); debug_msg(")\n");
#endif
  if(err.getError()) return false;
  if(!theFile) { if(!openTheFile()) return false; }
  fseek(theFile,(blk*blockSize)+skipOffset,SEEK_SET);
  fread(buf,blockSize,1,theFile);
  return true;
}


bool DDiskImageFile::Mount(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageFile::Mount("); debug_msg(fname); debug_msg(")\n");
#endif
  theFileName=fname;
  if(openTheFile()) {
    // That's really all...I think.
  }
  else {
    debug_msg(DDISK_ERR_COULDNT_OPEN);
    diskError=true;
    return false;
  }
  return true;
}


bool DDiskImageFile::Unmount()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageFile::Unmount()\n");
#endif
  return false;
}


bool DDiskImageFile::readDirEntry(long off)
{
  debug_msg("DDiskImageFile::readDirEntry("); debug_long(off); debug_msg(") ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDiskImageFile::readDirectory()
{
  debug_msg("DDiskImageFile::readDirectory() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDiskImageFile::readGeometry()
{
  debug_msg("DDiskImageFile::readGeometry() not implemented\n");
  bool ret=false;
  return ret;
}


void DDiskImageFile::debugDump()
{
  DDiskImage::debugDump();
}


