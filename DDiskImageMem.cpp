
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


#include <stdlib.h>
#include <string.h>


#include "DDiskImageMem.h"


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
//  DDiskImageMem Class
////////////////////////////////////////////////////////////////////////////////


DDiskImageMem::DDiskImageMem(const char *fname) : DDiskImage(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::DDiskImageMem(...,'"); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  //myVFS=parent;
  //debug_msg("NOTE: DDiskImageMem without an DVfs for parent...\n");
  //Mount(fname);
}


DDiskImageMem::DDiskImageMem(unsigned char *b,UINT offset) : DDiskImage("DDiskImageMem from buffer")
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::DDiskImageMem(...,"); debug_int(offset); debug_msg(")\n");
#endif
  init();
  debug_msg("DDiskImageMem from buffer not implemented yet!\n");
}


void DDiskImageMem::init()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::init()\n");
#endif
  imageData=NULL;
}


DDiskImageMem::~DDiskImageMem()
{
  // TODO: flush files if cached
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


/* STATIC */
bool DDiskImageMem::recognize(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
#endif
  return false;
}


/* STATIC */
unsigned char *DDiskImageMem::recognizeHelper(const char *fname)
{
  bool ret=false;
  unsigned int bufSize=512; // TODO: Does this need to be larger?
  unsigned char *buffer=(unsigned char *)malloc(bufSize);
  FILE *f=fopen(fname,"rb");
  if(!f) {
#ifdef DEBUG_VERBOSE
    debug_msg("DDiskImageMem::recognizeHelper('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN);
#endif
    return NULL;
  }
  size_t nread=::fread(buffer,1,bufSize,f);
  if(nread!=bufSize) {
    // NOTE: If the file is under 512 bytes this is normal...
#ifdef DEBUG_VERBOSE
    debug_msg("DDiskImageMem::recognizeHelper('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_READ_ERROR);
#endif
    return NULL;
  }
  fclose(f);
  //
  //dumpBufHex((uint8_t *)buffer,bufSize);
  //
  //free(buffer);
  return buffer;
}


/* STATIC */
bool DDiskImageMem::recognizeFileExtension(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
#endif
  return false;
}


const char *DDiskImageMem::guessSystem(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::guessSystem('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
#endif
  return NULL;
}


bool DDiskImageMem::readBlock(UINT blk)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::readBlock("); debug_int(blk); debug_msg(")\n");
#endif
  diskError=true;
  if(!imageData) {
     debug_msg(DDISK_ERR_NO_BUFFER);
     return false;
  }
  curLoc=blk*blockSize+skipOffset;
  if(curLoc>imageDataSize) { debug_msg(DDISK_ERR_BAD_SIZE); return false; }
#ifdef DEBUG_VERBOSE
  debug_msg("disk curLoc="); debug_int(curLoc); debug_msg("\n");
#endif
  for(UINT t=0;t<blockSize;t++) {
    buf[t]=imageData[curLoc+t];
    //debug_msg((int)buf[t]); debug_msg(",";
    //if(!(t&0xf)) debug_msg("\n");
  }
  //debug_msg("\n");
  diskError=false;
  return true;
}


bool DDiskImageMem::Mount(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::Mount('"); debug_msg(fname); debug_msg("')\n");
#endif
  theFileName=fname;
  if(imageData) free(imageData);
  diskMounted=false;
  UINT t=0;
  size_t nread=0;
  imageDataSize=getSize(fname);
#ifdef USE_FAKE_CHDIR
#endif // USE_FAKE_CHDIR
  FILE *fd=fopen(fname,"rb");
  if(fd) {
    //debug_msg("Going to allocate "); debug_msg(imageDataSize); debug_msg(" sized chunk...\n");
    imageData=(unsigned char *)malloc(imageDataSize*sizeof(char));
    if(imageData) {
      bool ww=true;
      char c;
      t=0;
      while(ww) {
        nread=fread(&c,1,1,fd);
        if(nread!=1) ww=false;
        else {
          imageData[t]=c;
          t++;
        }
      }
      diskMounted=true;
      for(t=0;t<DDISK_MAX_NAMELEN;t++) diskName[t]=0;
      for(t=0;t<strlen(fname);t++) diskName[t]=fname[t];
#ifdef DEBUG_VERBOSE
      debug_msg("Mounted disk image '"); debug_msg(diskName); debug_msg("'.\n");
#endif
      // (Have to do this outside of DDiskImage due to overloaded readDir's)
      // Oops...probably not, but its working so leave it silly...
      //readDir();
      fclose(fd);
    }
    else {
      debug_msg(DDISK_ERR_NO_BUFFER);  return false;
    }
  }
  else {
    debug_msg(DDISK_ERR_COULDNT_OPEN);  return false;
  }
#ifdef DEBUG_VERBOSE
  debug_msg("Type is "); debug_msg(getName());  debug_msg("\n");
#endif
  readGeometry();
  if(!readDirectory()) { debug_msg("DDiskImageMem::Mount, readDirectory failed!!!\n"); return false; }
  return true;
}


bool DDiskImageMem::Unmount()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::Unmount()\n");
#endif
  diskMounted=false;
  return false;
}


bool DDiskImageMem::readDirEntry(long off)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::readDirEntry() "); debug_int(off); debug_msg(" "); debug_msg(DDISK_ERR_NO_IMPL);
#endif
  return false;
}


bool DDiskImageMem::readDirectory()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImageMem::readDirectory() "); debug_msg(DDISK_ERR_NO_IMPL);
#endif
  return false;
}


bool DDiskImageMem::readGeometry()
{
  debug_msg("DDiskImageMem::readGeometry() not implemented\n");
  bool ret=false;
  return ret;
}


bool DDiskImageMem::handleEmuFormat()
{
  // TO BE DONE...
  return false;
}


void DDiskImageMem::debugDump()
{
  DDiskImage::debugDump();
}

