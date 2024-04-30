
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


#include <string.h>


#include "DDiskImage.h"


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
//  DDiskImage Class
////////////////////////////////////////////////////////////////////////////////


DDiskImage::DDiskImage(const char *fname) : DDisk(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImage::DDiskImage(...,'"); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  //myVFS=parent;
  //if(!parent) debug_msg("NOTE: DDiskImage without an DVfs for parent...\n");
  //Mount(fname);
}


DDiskImage::~DDiskImage()
{
  // TODO: flush files if cached
  freeDirCache();
  diskMounted=false;
}


/* STATIC */
bool DDiskImage::recognizeFileExtension(const char *fname)
{
  debug_msg("DDiskImage::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


const char *DDiskImage::guessSystem(const char *fname)
{
  debug_msg("DDiskImage::guessSystem('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return NULL;
}


void DDiskImage::init()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImage::init()\n");
#endif
  curLoc=0;
  // These next 3 might not be used, depending on the format...
  track=0; sector=0; side=0;
  diskError=false;
  const char *tname="no name";
  UINT t=0;
  for(t=0;t<strlen(tname);t++) diskName[t]=tname[t];
  diskMounted=false;
  writeProtected=true;
  skipOffset=0;
  for(t=0;t<EDISK_BUF_SIZE;t++) buf[t]=0;
  trackOffset=(UINT *)NULL;
  numSectors=(UINT *)NULL;
  imageDataSize=0;
  cachesData=true;
  cachesFiles=true;
  cachesDir=true;
}


bool DDiskImage::readBlock(UINT blk)
{
  debug_msg("DDiskImage::readBlock() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDiskImage::readTrackSector(UINT tt,UINT ss)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImage::readTrackSector("); debug_int(tt); debug_msg(","); debug_int(ss); debug_msg(")\n");
#endif
  diskError=true;
  track=tt;  sector=ss;  side=0;
  if(!diskMounted) {
    debug_msg("DDiskImage::readTrackSector "); debug_msg(DDISK_ERR_NO_DISK);
    return false;
  }
  if(track>maxTrack) {
    debug_msg("DDiskImage::readTrackSector "); debug_msg(DDISK_ERR_BAD_TRACK);
    return false;
  }
  if((!track)&&noTrackZero) {
    debug_msg("DDiskImage::readTrackSector "); debug_msg(DDISK_ERR_BAD_TRACK);
    return false;
  }
  if((!sector)&&noSectorZero) {
    debug_msg("DDiskImage::readTrackSector "); debug_msg(DDISK_ERR_BAD_SECTOR);
    return false;
  }
  UINT blk=0;
  UINT ttrack=track,tsector=sector,tMaxSector=maxSector;
  if(noTrackZero) ttrack--;
  if(noSectorZero) tsector--;
  else tMaxSector++;
  if(allTracksSameSectors) {
    if(tsector>maxSector) {
      debug_msg(DDISK_ERR_BAD_SECTOR);
      return false;
    }
    blk=tsector+((tMaxSector)*ttrack);
  }
  else {
    if(!numSectors) { debug_msg("DDiskImage::readTrackSector "); debug_msg("No numSectors!\n"); return false; }
    if(!trackOffset) { debug_msg("DDiskImage::readTrackSector "); debug_msg("No trackOffset!\n"); return false; }
    if(numSectors[ttrack]<tsector) {
      debug_msg(DDISK_ERR_BAD_SECTOR); return false;
    }
    blk=tsector+trackOffset[ttrack];
  }
  //debug_msg("Reading t="); debug_msg(track); debug_msg(" s="); debug_msg(sector); debug_msg("\n");
  readBlock(blk);
  diskError=false;
  return true;
}


void *DDiskImage::Fopen(const char *filename,const char *mode)
{
  diskError=false;
  void *ret=NULL;
  if(!diskMounted) {
    debug_msg(DDISK_ERR_NO_DISK);
    diskError=true;
    return ret;
  }
  if(noFileSystem) {
    debug_msg("DDiskImage::fopen "); debug_msg(DDISK_ERR_NON_DOS);
    return NULL;
  }
  const char *thename=searchForFirst(filename);
  if(!thename) {
    debug_msg("DDiskImage::fopen searchForFirst failed!\n");
    return NULL;
  }
  DDirEntry *aFile=searchForFirstFile(thename);
  if(!aFile) {
    debug_msg("DDiskImage::fopen searchForFirstFile failed!\n");
    return NULL;
  }
  cacheFile(aFile);
  aFile->curSeek=0;
  return (void *)aFile;
}


UINT DDiskImage::Fclose(void *fp)
{
  DDirEntry *aFile=(DDirEntry *)fp;
  aFile->curSeek=0;
  return 0;
}


size_t DDiskImage::Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
  if(!fp) return 0;
  if(!ptr) return 0;
  DDirEntry *f=(DDirEntry *)fp;
  if(!f->data) cacheFile(fp);
  if(!f->data) return 0;  // we tried...
  char *p=(char *)ptr;
  size_t rsize=sizelem*n;
  //if(w) *w); debug_msg("Going to fread "); debug_msg(rsize); debug_msg("\n");
  if(!rsize) return 0;
  for(uint32_t t=0;t<rsize;t++) {
    p[t]=f->data[f->curSeek+t];
  }
  f->curSeek+=rsize;
  return n;
  // TODO: Check that we actually read that much and return accordingly
}


size_t DDiskImage::Fwrite(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
  // TODO: writing isn't implemented!
  return 0;
}


int DDiskImage::Fseek(void *fp,long offset,int origin)
{
  DDirEntry *aFile=(DDirEntry *)fp;
  size_t seekOffset=0;
  switch(origin) {
    case SEEK_SET:
      seekOffset=offset;
      break;
    case SEEK_END:
      seekOffset=aFile->size+offset;
      break;
    case SEEK_CUR:
      seekOffset=aFile->curSeek+offset;
      break;
    default:
      break;
  }
  aFile->curSeek=seekOffset;
  return 0;
}


long DDiskImage::Ftell(void *fp)
{
  DDirEntry *aFile=(DDirEntry *)fp;
  return aFile->curSeek;
}


int DDiskImage::Fstat(int fd,struct awstat_struct *st)
{
  // TODO: implement this
  return -1;
}


int DDiskImage::Ferror(void *fp)
{
  // TODO: implement this
  return -1;
}


int DDiskImage::Feof(void *fp)
{
  // TODO: implement this
  return -1;
}


bool DDiskImage::Mount(const char *fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImage::Mount("); debug_msg(fname); debug_msg(")\n");
#endif
  return false;
}


bool DDiskImage::Unmount()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDiskImage::Unmount()\n");
#endif
  return false;
}


bool DDiskImage::readDirEntry(long off)
{
  debug_msg("DDiskImage::readDirEntry("); debug_long(off); debug_msg(") ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDiskImage::readDirectory()
{
  debug_msg("DDiskImage::readDirectory() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDiskImage::readGeometry()
{
  debug_msg("DDiskImage::readGeometry() not implemented\n");
  bool ret=false;
  return ret;
}


void DDiskImage::debugDump()
{
  DDisk::debugDump();
  debug_msg("skipOffset="); debug_long(skipOffset); debug_msg("\n");
  debug_msg("imageDataSize="); debug_long(imageDataSize); debug_msg("\n");
}
