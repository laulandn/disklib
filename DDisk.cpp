
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


//#define DDISK_DBH_READABLE 1


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "DPartitionTable.h"
#include "DDiskImageFile.h"
#include "DDiskImageMem.h"

#include "DLocalDrive.h"


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
//  DDisk Class
////////////////////////////////////////////////////////////////////////////////

// NOTE: fname not used
DDisk::DDisk(const char *filename)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDisk::DDisk()\n");
#endif
  init();
}


DDisk::~DDisk()
{
#ifdef USE_FAKE_CHDIR
  if(prefixPath) free(prefixPath);
  prefixPath=NULL;
#endif // USE_FAKE_CHDIR
}


/* STATIC */
bool DDisk::recognize(const char *filename)
{
  debug_msg("DDisk::recognize('"); debug_msg(filename); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


/* STATIC */
bool DDisk::recognizeFileExtension(const char *filename)
{
  debug_msg("DDisk::recognizeFileExtension('"); debug_msg(filename); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


const char *DDisk::guessSystem(const char *filename)
{
  debug_msg("DDisk::guessSystem('"); debug_msg(filename); debug_msg("') ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return NULL;
}


void DDisk::init()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDisk::init()\n");
#endif
  //myVFS=(DVfs *)NULL;
  curLoc=0;
  track=0; sector=0; side=0;
  diskError=false;
  writeProtected=true;  // For now everything is read-only
  allTracksSameSectors=true;
  noTrackZero=false;  noSectorZero=false;
  maxTrack=0;  maxSector=0;  maxSide=0;
  hasPartitions=false;
  partitionTable=(DPartitionTable *)NULL;
  partitionNumber=0;
  UINT t=0;
  for(t=0;t<DDISK_MAX_NAMELEN;t++) diskName[t]=0;
  for(t=0;t<DDISK_MAX_NAMELEN;t++) diskID[t]=0;
  for(t=0;t<DDISK_MAX_NAMELEN;t++) matchName[t]=0;
  for(t=0;t<DDISK_MAX_NAMELEN;t++) curDir[t]=0;
#ifdef USE_FAKE_CHDIR
  prefixPath=NULL;
#endif // USE_FAKE_CHDIR
  blockSize=256;  // Just a default...
  diskError=false;
  diskStatus=DDISK_STATUS_GOOD;
  noFileSystem=false;
  cachesData=false;
  cachesFiles=false;
  cachesDir=false;
  doNotCacheDir=false;
  doNotCacheFiles=false;
#ifdef USE_FINDER_DAT
  ignoreFinderDat=false;
#endif // USE_FINDER_DAT
  for(t=0;t<DDISK_MSG_SIZE;t++) messageBuffer[t]=0;
  //outStream=NULL;
}


// NOTE: Remember to override this if you're caching the dir in a subclass!
bool DDisk::readDirectory()
{
  debug_msg("DDisk::readDirectory() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  cachesDir=false;
  return false;
}


// Write back to store
void DDisk::flushBuffers()
{
  debug_msg("DDisk::flushBuffers() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
}


// Reread from store
bool DDisk::refreshBuffers()
{
  // TODO: When we support writes to images, need to do more here...
  if(cachesDir) {
    freeDirCache();
    if(!readDirectory()) { debug_msg("DDisk::refreshBuffers, readDirectory failed!!!\n"); return false; }

  }
  return true;
}


bool DDisk::isDir(const char *filename)
{
  DDirEntry *de=searchForFirstFile(filename);
  if(de) {
    if(de->type==DDISK_TYPE_DIR) return true;
  }
  return false;
}


bool DDisk::isSpecial(const char *filename)
{
  DDirEntry *de=searchForFirstFile(filename);
  if(de) {
    if(de->type==DDISK_TYPE_DIR) return true;
    if(de->type==DDISK_TYPE_DEVICE) return true;
  }
  return false;
}


// NOTE: See below function, which is the version for real files on the host fs
size_t DDisk::getFileSize(const char *filename)
{
  bool done=false;
  size_t rval=0;
#ifdef ALIB_HAVE_STAT
  struct awstat_struct s;
  if(!Stat(filename,&s)) {
    rval=s.st_size;
    done=true;
  }
#endif // ALIB_HAVE_STAT
  if(!done) {
    FILE *fd=(FILE *)Fopen(filename,"rb");
    if(fd) {
      Fseek(fd,0,SEEK_END);
      rval=Ftell(fd);
      Fclose(fd);
    }
  }
  return rval;
}


// NOTE: See above function, which is the version for files on this emulated fs
size_t DDisk::getSize(const char *filename)
{
  bool done=false;
  size_t rval=0;
  // TODO: use stat or equiv instead of seeking if we can here.
  if(!done) {
#ifdef USE_FAKE_CHDIR
    debug_msg("Warning! Couldn't fake path to find file size!!!\n");
    //filename=fakeFullPath(filename);
#endif // USE_FAKE_CHDIR
    FILE *fd=fopen(filename,"rb");
    if(fd) {
      fseek(fd,0,SEEK_END);
      rval=ftell(fd);
      fclose(fd);
    }
  }
  return rval;
}


UINT DDisk::getFileVersionMajor(const char *filename)
{
  DDirEntry *theFile=findDirEntry(filename);
  if(!theFile) { /*debug_msg("No such file as '"); debug_msg(filename); debug_msg("'\n");*/ return 0; }
  debug_msg("DDisk::getFileVersionMajor ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  UINT ret=0;
  bool foundIt=false;
  /*
  // First try Mac style 'vers' resource
  RResourceFile mres(filename,dos);
  if(!mres.err.getError()) {
    UINT rcount=mres.count("vers");
    if(rcount) {
      foundIt=true;
#ifdef DEBUG_VERBOSE
      debug_msg("Got a Mac vers resource...\n");
#endif
      UINT num=0;
      if(rcount>1) num=2;  // look at 2nd vers if there's more than 1
      UINT rsize=mres.getSizeFromNum("vers"),num);
#ifdef DEBUG_VERBOSE
      debug_msg("size is "); debug_msg(rsize); debug_msg("\n");
#endif
      char *buf=(char *)malloc(rsize*sizeof(char));
      mres.readFromNum("vers"),num,buf);
      if(err.getError()) debug_msg("Something went wrong...\n");
#ifdef DEBUG_VERBOSE
      else debug_msg("Looks like it read fine.\n");
#endif
      free(buf);
    }
  }
  */
  // TODO: now try a Windows style version resource
  return ret;
}


UINT DDisk::getFileVersionMinor(const char *filename)
{
  DDirEntry *theFile=findDirEntry(filename);
  if(!theFile) { /*debug_msg("No such file as '"); debug_msg(filename); debug_msg("'\n");*/ return false; }
  debug_msg("DDisk::getFileVersionMinor ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  UINT ret=0;
  //
  return ret;
}


const char *DDisk::searchForFirst(const char *filename)
{
  return searchForFirstCached(filename);
}


const char *DDisk::searchForFirstCached(const char *filename)
{
  debug_msg("DDisk::searchForFirstCached('"); debug_msg(filename); debug_msg("')...\n");
  if(!cachesDir) {
    debug_msg("DDisk::searchForFirstCached can't work if dir isn't cached!\n");
    debugDump();
    exit(EXIT_FAILURE);
    return NULL;
  }
  theDir.jumpToHead();
  if(!filename) filename="*";
  // TODO: problem here if filename is longer than DDISK_MAX_NAMELEN chars
  strcpy(matchName,filename);
  return searchForNext();
}


const char *DDisk::searchForNext()
{
  return searchForNextCached();
}


// NOTE: don't TOUCH the dir list if you expect to use this...
const char *DDisk::searchForNextCached()
{
  debug_msg("DDisk::searchForNextCached()...\n");
  if(!cachesDir) {
    debug_msg("DDisk::searchForNextCached can't work if dir isn't cached!\n");
    debugDump();
    exit(EXIT_FAILURE);
    return NULL;
  }
  DDirEntry *de=searchForNextFile();
  if(de) return de->name;
  else return NULL;
}


DDirEntry *DDisk::searchForFirstFile(const char *filename)
{
  return searchForFirstFileCached(filename);
}


DDirEntry *DDisk::searchForFirstFileCached(const char *filename)
{
  debug_msg("DDisk::searchForFirstFileCached('"); debug_msg(filename); debug_msg("')...\n");
  if(!cachesDir) {
    debug_msg("DDisk::searchForFirstFileCached can't work if dir isn't cached!\n");
    debugDump();
    exit(EXIT_FAILURE);
    return (DDirEntry *)NULL;
  }
  theDir.jumpToHead();
  if(!filename) filename="*";
  // TODO: problem here if filename is longer than DDISK_MAX_NAMELEN chars
  strcpy(matchName,filename);
  return searchForNextFile();
}


DDirEntry *DDisk::searchForNextFile()
{
  return searchForNextFileCached();
}


// NOTE: don't TOUCH the dir list if you expect to use this...
DDirEntry *DDisk::searchForNextFileCached()
{
  debug_msg("DDisk::searchForNextFileCached()...\n");
  if(!cachesDir) {
    debug_msg("DDisk::searchForNextFileCached can't work if dir isn't cached!\n");
    debugDump();
    exit(EXIT_FAILURE);
    return (DDirEntry *)NULL;
  }
  DDirEntry *de=(DDirEntry *)NULL;
  DDirEntry *foundDe=(DDirEntry *)NULL;
  bool done=false;
  while(!done) {
    de=(DDirEntry *)theDir.info();
    if(de) {
      //debug_msg("findNextFile looking at "); debug_msg(de->name); debug_msg("\n");
      UINT t=0;
      bool c=true,bad=false,wild=false;
      while(c) {
        //debug_msg(t); debug_msg(" comparing "); debug_msg(matchName[t]); debug_msg(" and "); debug_msg(de->name[t]); debug_msg("\n");
        if(matchName[t]!=de->name[t]) {
          // the letters didn't match, but we might have a wild card
          bad=true;  wild=false;
          if(matchName[t]=='*') { bad=false; wild=true; }
          if(matchName[t]=='?') { bad=false; wild=true; }
          if(bad) c=false;
        }
        t++;
        if(t==DDISK_MAX_NAMELEN) c=false;  // hit max file len
        if(t==strlen(matchName)) {
          // we're past the end of the name we're using to match
          if(strlen(de->name)>strlen(matchName)) {
            // if the last letter we looked at wasn't wild, we failed
            if(!wild) bad=true;
          }
          c=false;
        }
        if(t==strlen(de->name)) {
          // we're past the end of the name we're looking at
          if(strlen(de->name)<strlen(matchName)) {
            bad=true;
            // if the letter past what we looked at wasn't wild, we failed
            if(matchName[t]=='*') { bad=false; wild=true; }
            if(matchName[t]=='?') { bad=false; wild=true; }
          }
          c=false;
        }
      }
      if(!bad) { foundDe=de; done=true; }
      theDir.advance();
      if(theDir.atEnd()) done=true;
    }
    else done=true;
  }
  //if(foundDe) debug_msg(foundDe->name); debug_msg(" looks good!\n");
  return foundDe;
}


bool DDisk::Chdir(const char *dname)
{
  if(!dname) return false;
#ifdef USE_FAKE_CHDIR
  if(prefixPath) free(prefixPath);
  prefixPath=strdup(dname);
#ifdef DEBUG_VERBOSE
  debug_msg("CWD is now: "); debug_msg(prefixPath); debug_msg("\n");
#endif
  return true;
#else
  //if(w) *w); debug_msg("cd not implemented for this format!\n");
  debug_msg("cd "); debug_msg(DDISK_ERR_NO_IMPL);
#endif // USE_FAKE_CHDIR
  return false;
}


const char *DDisk::Getcwd(char *buf,size_t len)
{
  debug_msg("(DDisk::Getcwd()...)\n");
#ifdef USE_FAKE_CHDIR
  awstrncpy(buf,prefixPath,len);
  return prefixPath;
#else
  debug_msg("getcwd "); debug_msg(DDISK_ERR_NO_IMPL);
  return curDir;
#endif // USE_FAKE_CHDIR
  return NULL;
}


void *DDisk::Fopen(const char *filename,const char *mode)
{
  DDirEntry *theFile=findDirEntry(filename);
  if(!theFile) { debug_msg("No such file as '"); debug_msg(filename); debug_msg("'\n"); return NULL; }
  debug_msg("DDisk::Fopen() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return (void *)NULL;
}


UINT DDisk::Fclose(void *fp)
{
  debug_msg("DDisk::Fclose() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


size_t DDisk::Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
  debug_msg("DDisk::Fread() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


size_t DDisk::Fwrite(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
  debug_msg("DDisk::Fwrite() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


size_t DDisk::readMany(void *fp,uint8_t *buffer,size_t n)
{
  //if(!myVFS) return 0;
  size_t num=0;
  uint8_t c;
  for(size_t t=0;t<n;t++) {
    num+=Fread(&c,1,1,(FILE *)fp);
    buffer[t]=c;
  }
  return num;
}


int DDisk::Fseek(void *fp,long offset,int origin)
{
  debug_msg("DDisk::Fseek() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


long DDisk::Ftell(void *fp)
{
  debug_msg("DDisk::Ftell() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


int DDisk::Ferror(void *fp)
{
  debug_msg("DDisk::Ferror() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


int DDisk::Feof(void *fp)
{
  debug_msg("DDisk::Feof() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return 0;
}


int DDisk::Fstat(int fd,struct awstat_struct *st)
{
  debug_msg("DDisk::Fstat() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  // TODO: implement this
  return -1;
}


int DDisk::Stat(const char *fname,struct awstat_struct *st)
{
  DDirEntry *theFile=findDirEntry(fname);
  if(!theFile) { return false; }
  debug_msg("DDisk::Stat() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  // TODO: implement this
  return -1;
}


void *DDisk::Open(const char *fname,UINT mode)
{
  const char *cmode="rb";
  // TODO: We don't actually use mode, since we're read-only right now...
  return Fopen(fname,cmode);
}


UINT DDisk::Close(void *fp)
{
  return Fclose(fp);
}


size_t DDisk::Read(void *fp,uint8_t *ptr,size_t n)
{
  return Fread(ptr,n,1,fp);
}


size_t DDisk::Write(void *fp,uint8_t *ptr,size_t n)
{
  return Fwrite(ptr,n,1,fp);
}


int DDisk::Lseek(void *fp,long offset,int origin)
{
  return Fseek(fp,offset,origin);
}


UINT DDisk::popCount(char q)
{
  UINT ret=0;
  if(q&0x80) ret++;
  if(q&0x40) ret++;
  if(q&0x20) ret++;
  if(q&0x10) ret++;
  if(q&0x08) ret++;
  if(q&0x04) ret++;
  if(q&0x02) ret++;
  if(q&0x01) ret++;
  return ret;
}


void DDisk::freeDirCache()
{
  theDir.jumpToHead();
  DDirEntry *e=(DDirEntry *)NULL;
  bool reading=true;
  while(reading) {
    if(theDir.atEnd()) reading=false;
    e=(DDirEntry *)theDir.info();
    //debug_msg("freeDirCache removing "); debug_msg(e->name); debug_msg(" from theDir...\n");
    if(e) theDir.remove();
    else reading=false;
    delete e;
  }
}


void DDisk::freeFileCache(void *fp)
{
  debug_msg("DDisk::freeFileCache() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
}


bool DDisk::cacheFile(void *fp)
{
  debug_msg("DDisk::cacheFile() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


const char *DDisk::getShortName(const char *longName)
{
  DDirEntry *theFile=findDirEntry(longName);
  if(!theFile) { debug_msg("No such file as '"); debug_msg(longName); debug_msg("'\n"); return NULL; }
  debug_msg("DDisk::getShortName() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return longName;
}


#ifdef USE_FINDER_DAT
bool DDisk::setIgnoreFinderDat(bool t)
{
  ignoreFinderDat=t;
  //refresh();
  return true;
}
#endif // USE_FINDER_DAT


int DDisk::Rename(const char *oldname,const char *newname)
{
  DDirEntry *theFile=findDirEntry(oldname);
  if(!theFile) { /*debug_msg("No such file as '"); debug_msg(oldname); debug_msg("'\n");*/ return false; }
  debug_msg("DDisk::Rename() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return -1;
}


bool DDisk::myCopyFile(const char *oldname,const char *newname)
{
  DDirEntry *theFile=findDirEntry(oldname);
  if(!theFile) { /*debug_msg("No such file as '"); debug_msg(oldname); debug_msg("'\n");*/ return false; }
  debug_msg("DDisk::myCopyFile() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDisk::Unlink(const char *filename)
{
  DDirEntry *theFile=findDirEntry(filename);
  if(!theFile) { /*debug_msg("No such file as '"); debug_msg(filename); debug_msg("'\n");*/ return false; }
  debug_msg("DDisk::Unlink() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDisk::removeAllFilesinFolder(const char *foldername)
{
  bool retVal=false;
  if(!isDir(foldername)) return false;
  bool ok=Chdir(foldername);
  if(!ok) {
    debug_msg("Couldn't cd to "); debug_msg(foldername); debug_msg("\n");
    return false;
  }
  AList_disk *theDir=getTheDir();
  if(!theDir) {
    debug_msg("Couldn't getTheDir!\n");
    return false;
  }
#ifdef DEBUG_VERBOSE
  debug_msg("Going to start...\n");
#endif
  theDir->jumpToHead();
  bool working=true;
  DDirEntry *e=(DDirEntry *)NULL;
  while(working) {
    if(theDir->atEnd()) working=false;
    else {
      e=(DDirEntry *)theDir->info();
      debug_msg(e->name);
      bool ignore=false;
      if((e->type==DDISK_TYPE_DIR)||(e->type==DDISK_TYPE_FS)) {
        debug_msg("/");
        debug_msg("\n");
        bool doit=true;
        if(!strcmp(".",e->name)) doit=false;
        if(!strcmp("..",e->name)) doit=false;
        if(doit) {
#ifdef DEBUG_VERBOSE
          debug_msg("Descending into sub-dir...\n");
#endif
          Unlink(e->name);
        }
        else ignore=true;
      }
      if(!ignore) {
        Unlink(e->name);
      }
      debug_msg("\n");
      theDir->advance();
    }
  }
#ifdef DEBUG_VERBOSE
  debug_msg("Done deleting files.\n");
#endif
  ok=Chdir("..");
  return retVal;
}


#ifdef USE_FAKE_CHDIR
const char *DDisk::fakeFullPath(const char *name)
{
  if(!prefixPath) return name;
#ifdef DEBUG_VERBOSE
  debug_msg("Building fullPathName:\n");
#endif
  fullPathName[0]=0;
  AApp::fillInString(fullPathName,prefixPath);
  AApp::fillInString(fullPathName,"/"));
  AApp::fillInString(fullPathName,name);
#ifdef DEBUG_VERBOSE
  debug_msg("fullPathName is '"); debug_msg(fullPathName); debug_msg("\n");
#endif
  return fullPathName;
}
#endif // USE_FAKE_CHDIR


bool DDisk::readBlock(UINT bl)
{
  debug_msg("DDisk::readBlock() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDisk::readPhysTrackSector(UINT tt,UINT ss)
{
  debug_msg("DDisk::readPhysTrackSector() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDisk::readLogicalTrackSector(UINT tt,UINT ss)
{
  // This is totally wrong!
  UINT ptt=transLogToPhysTrack(tt);
  UINT pss=transLogToPhysSector(ss);
  return readPhysTrackSector(ptt,pss);
}


bool DDisk::writePhysTrackSector(UINT tt,UINT ss)
{
  debug_msg("DDisk::writePhysTrackSector() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDisk::writeLogicalTrackSector(UINT tt,UINT ss)
{
  UINT ptt=transLogToPhysBlock(tt);
  UINT pss=transLogToPhysBlock(ss);
  return writePhysTrackSector(ptt,pss);
}


bool DDisk::readPhysBlock(UINT blk)
{
  debug_msg("DDisk::readPhysBlock() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDisk::readLogicalBlock(UINT blk)
{
  UINT pblock=transLogToPhysBlock(blk);
  return readPhysBlock(pblock);
}


bool DDisk::writePhysBlock(UINT blk)
{
  debug_msg("DDisk::writePhysBlock() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  diskStatus=DDISK_STATUS_WRITE_ERROR;
  return false;
}


bool DDisk::writeLogicalBlock(UINT blk)
{
  UINT pblock=transLogToPhysBlock(blk);
  return writePhysBlock(pblock);
}


bool DDisk::markPhysBlockUsed(UINT blk)
{
  debug_msg("DDisk::markPhysBlockUsed() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDisk::markLogicalBlockUsed(UINT blk)
{
  UINT pblock=transLogToPhysBlock(blk);
  return markPhysBlockUsed(pblock);
}


bool DDisk::markPhysBlockUnused(UINT blk)
{
  debug_msg("DDisk::markPhysBlockUnused() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DDisk::markLogicalBlockUnused(UINT blk)
{
  UINT pblock=transLogToPhysBlock(blk);
  return markPhysBlockUnused(pblock);
}


UINT DDisk::getLogicalBlockSize()
{
  return getPhysBlockSize();
}


bool DDisk::readBootBlock()
{
  debug_msg("DDisk::readBootBlock()...\n");
  bool ret=false;
  return ret;
}


bool DDisk::Mount(const char *filename)
{
  DDirEntry *theFile=findDirEntry(filename);
  if(!theFile) { /*debug_msg("No such file as '"); debug_msg(filename); debug_msg("'\n");*/ return false; }
  debug_msg("DDisk::Mount() ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDisk::Unmount()
{
  //debug_msg("DDisk::Unmount() "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DDisk::readDirEntry(long off)
{
  debug_msg("DDisk::readDirEntry("); debug_long(off); debug_msg(") ("); debug_msg(getName()); debug_msg("), "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


DDirEntry *DDisk::findDirEntry(const char *filename)
{
  DDirEntry *ret=NULL;
  DDirEntry *theFile=NULL;
  //
  AList_disk *theDir=getTheDir();
  theDir->jumpToHead();
  bool working=true;
  while(working) {
    theFile=(DDirEntry *)theDir->info();
    if(theFile) {
    /*
      debug_msg("found '";
      if(theFile->name) debug_msg(theFile->name; else debug_msg("No name";
      debug_msg("'\n");
      */
      if(!strcmp(filename,theFile->name)) ret=theFile;
    }
    else { /*debug_msg("(found missing file)\n");*/ }
    if(theDir->atEnd()) working=false;
    else theDir->advance();
  }
  //
  return ret;
}


// TODO: not possible...need to handle as block!
UINT DDisk::transPhysToLogTrack(UINT track)
{
  return track;
}


// TODO: not possible...need to handle as block!
UINT DDisk::transLogToPhysTrack(UINT track)
{
  return track;
}


// TODO: not possible...need to handle as block!
UINT DDisk::transPhysToLogSector(UINT sector)
{
  return sector;
}


// TODO: not possible...need to handle as block!
UINT DDisk::transLogToPhysSector(UINT sector)
{
  return sector;
}


//TODO: This is more possible...
UINT DDisk::transPhysToLogBlock(UINT block)
{
  return block;
}


//TODO: This is more possible...
UINT DDisk::transLogToPhysBlock(UINT block)
{
  return block;
}


// TODO: not possible...need to handle as block!
UINT DDisk::transPhysToLogSide(UINT side)
{
  return side;
}


// TODO: not possible...need to handle as block!
UINT DDisk::transLogToPhysSide(UINT side)
{
  return side;
}


// TODO: Above functions will neded this
UINT DDisk::transLogTrackSector2Block(UINT ttrack,UINT tsector)
{
  return 0;
}


// TODO: Above functions will neded this
UINT DDisk::transPhysTrackSector2Block(UINT ttrack,UINT tsector)
{
  return 0;
}


bool DDisk::detectSubFormat()
{
  subFormat=DDISK_FORMAT_DEFAULT;
  return true;
}


bool DDisk::getDiskError()
{
  return diskError;
}


const char *DDisk::getDiskErrorString()
{
  return "Unknown disk error";
}


UINT DDisk::getDiskStatus()
{
  return diskStatus;
}


const char *DDisk::getStatusString()
{
      switch(diskStatus) {
        case DDISK_STATUS_GOOD:
          return "good";
          break;
        case DDISK_STATUS_READ_ERROR:
          return "read error";
          break;
        case DDISK_STATUS_WRITE_ERROR:
          return "write error";
          break;
        case DDISK_STATUS_BAD_FORMAT:
          return "bad format";
          break;
        case DDISK_STATUS_BAD_BLOCKNUM:
          return "bad block num";
          break;
        case DDISK_STATUS_NO_FS:
          return "no filesystem";
          break;
        default:
          return "unknown";
          break;
      }
  return "unknown";
}


AList_disk *DDisk::getTheDir() {
  if(!readDirectory()) {
    debug_msg("DDisk::getTheDir() readDirectory failed!!!\n");
    return NULL;
  }
  return &theDir;
}


void DDisk::dumpBufHex(uint8_t *theBuffer,UINT theSize)
{
  debug_msg("DDisk::dumpBufHex()...\n");
  UINT rows=16, columns=16;  // Default is 256 bytes
  rows=theSize/16;
  UINT offset=0;
  for(UINT r=0;r<rows;r++) {
    offset=r*columns;
    debug_hex4(offset); debug_msg(" : ");
    for(UINT c=0;c<columns;c++) {
      char b=theBuffer[offset];
      debug_hex2(b); debug_msg(" ");
      offset++;
    }
    debug_msg(" ");
    offset=r*columns;
    for(UINT c=0;c<columns;c++) {
      int b=theBuffer[offset];
#ifdef DDISK_DBH_READABLE
      b=b&0x7f;
      if(b<32) b+='@';
#else
      if(b<32) b=' ';
      if(b>127) b='.';
#endif
      if((b>31)&&(b<128)) debug_char(b);
      //else debug_char(' ');
      offset++;
    }
    debug_nl();
  }
}


bool DDisk::readGeometry()
{
  debug_msg("DDisk::readGeometry() not implemented\n");
  bool ret=false;
  return ret;
}


// This is to remove spaces from MS-DOS or CP/M style 8.3 filenames...
// NOTE: This code is really fugly and hacky and couple probably be done a lot better
// TODO: This removes legit spaces in the MIDDLE of names or extensions, not just at the end!
void DDisk::removeSpacesFromFilenames()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDisk::removeSpacesFromFilenames()...\n");
#endif
  theDir.jumpToHead();
  bool working=true;
  char tname[16];
  while(working) {
    DDirEntry *theFile=(DDirEntry *)theDir.info();
    if(!theFile) working=false;
    else {
      //debug_msg("Looking at '"); debug_msg(theFile->name); debug_msg("'...\n");
      unsigned int t,last=0;
      for(t=0;t<16;t++) tname[t]=0;
      for(t=0;t<8;t++) if(theFile->name[t]!=' ') { tname[t]=theFile->name[t]; last=t; }
      tname[last+1]='.';
      for(t=0;t<3;t++) if(theFile->name[t+9]!=' ') { tname[t+last+2]=theFile->name[t+9]; }
      //debug_msg("New name will be '"); debug_msg(tname); debug_msg("'.\n");
      for(unsigned int t=0;t<16;t++) {
        theFile->name[t]=tname[t];
        if((theFile->name[t]<='Z')&&(theFile->name[t]>='A')) theFile->name[t]=theFile->name[t]-'A'+'a';
      }
    }
    theDir.advance();
    if(theDir.atEnd()) working=false;
  }
}


bool DDisk::sanityCheck()
{
//#ifdef DEBUG_VERBOSE
  debug_msg("DDisk::sanityCheck()...\n");
//#endif
  if(!basicSanityCheck()) return false;
  // By default
  return false;
}


bool DDisk::basicSanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DDisk::basicSanityCheck()...\n");
#endif
  bool ret=true;
  // Arbitrary...
  if(!maxTrack) { debug_msg("(maxTrack was zero!)\n"); ret=false; }
  if(maxTrack<20) { debug_msg("(maxTrack<20!)\n"); ret=false; }
  //if(maxTrack>100) { debug_msg("(maxTrack>100!)\n"); ret=false; }
  if(maxTrack>1024) { debug_msg("(maxTrack>1024!)\n"); ret=false; }
  if(!ret) { debug_msg("maxTrack was bad)\n"); return ret; }
  // Arbitrary...
  if(!maxSector) { debug_msg("(maxSector was zero!)\n"); ret=false; }
  if(maxSector<8) { debug_msg("(maxSector<8!)\n"); ret=false; }
  if(maxSector>100) { debug_msg("(maxSector>100!)\n"); ret=false; }
  if(!ret) { debug_msg("(maxSector was bad)\n"); return ret; }
  //
  if(!maxSide) { debug_msg("(maxSide was zero!)\n"); ret=false; }
  //if(maxSide>2) ret=false;
  if(!ret) { debug_msg("(maxSide was bad)\n"); return ret; }
  //
  if(!blockSize) { debug_msg("(blockSize was zero!)\n"); ret=false; }
  if(!ret) { debug_msg("(blockSize was bad)\n"); return ret; }
  //
  if(!ret) debug_msg("DDisk::basicSanityCheck() failed!!!\n");
  //
  return ret;
}



void DDisk::debugDump()
{
  debug_msg("DDisk::debugDump()...\n");
  if(!sanityCheck()) { debug_msg("DDisk failed sanityCheck!!!\n"); }
  debug_msg("format is"); debug_msg(getName()); debug_nl();
  debug_msg("curLoc="); debug_long(curLoc); debug_msg("\n");
  //debug_msg("diskError="); debug_msg(diskError); debug_msg("\n");
  //debug_msg("writeProtected="); debug_msg(writeProtected); debug_msg("\n");
  debug_msg("track="); debug_int(track); debug_msg(" sector="); debug_int(sector); debug_msg(" side="); debug_int(side); debug_msg("\n");
  debug_msg("maxTrack="); debug_int(maxTrack); debug_msg(" maxSector="); debug_int(maxSector); debug_msg(" maxSide="); debug_int(maxSide); debug_msg(" blockSize="); debug_int(blockSize); debug_msg("\n");
  debug_msg("allTracksSameSectors="); debug_int(allTracksSameSectors); debug_msg("\n");
  debug_msg("noTrackZero="); debug_int(noTrackZero); debug_msg(" noSectorZero="); debug_int(noSectorZero); debug_msg("\n");
  debug_msg("noFileSystem="); debug_int(partitionNumber); debug_msg("\n");
  debug_msg("cachesData="); debug_int(cachesData); debug_msg(" cachesFiles="); debug_int(cachesFiles); debug_msg(" cachesDir="); debug_int(cachesDir); debug_msg("\n");
  debug_msg("doNotCacheDir="); debug_int(doNotCacheDir); debug_msg(" doNotCacheFiles="); debug_int(doNotCacheFiles); debug_msg("\n");
  debug_msg("matchName='"); debug_msg(matchName); debug_msg("'\n");
  debug_msg("diskName='"); debug_msg(diskName); debug_msg("'\n");
  debug_msg("diskID='"); debug_msg(diskID); debug_msg("'\n");
  debug_msg("curDir='"); debug_msg(curDir); debug_msg("'\n");
}
