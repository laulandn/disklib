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


#include "DAppleDos3FS.h"


////////////////////////////////////////////////////////////////////////////////
//  DAppleDos3FS Class
////////////////////////////////////////////////////////////////////////////////

DAppleDos3FS::DAppleDos3FS(const char *fname) : DApple2FS(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleDos3FS::DAppleDos3FS("); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  thirteenSector=false;
  Mount(fname);
}


const char *DAppleDos3FS::guessSystem(const char *fname)
{
  return "apple2";
}


void DAppleDos3FS::init()
{
#ifdef DEBUG_VERBOSE
  //debug_msg("DAppleDos3FS disk image.\n");
#endif
  thirteenSector=false;
  nibble=false;
  allTracksSameSectors=false;
  maxTrack=0;  maxSector=0;  maxSide=0;
  noTrackZero=false;  noSectorZero=false;
  blockSize=0;
  cachesDir=false;
}


DAppleDos3FS::~DAppleDos3FS()
{
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


/* STATIC */
bool DAppleDos3FS::recognize(const char *fname)
{
  if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  if(!fname) return ret;
  FILE *f=fopen(fname,"rb");
  if(!f) {
#ifdef DEBUG_VERBOSE
    debug_msg("DAppleDos3FS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN);
#endif
    return false;
  }
  size_t size=getSize(fname);
  if((size!=143360)&&(size!=143363)&&(size!=232960)) {
    //fclose(f); f=NULL;
#ifdef DEBUG_VERBOSE
    //debug_msg("(DAppleDos3FS::recognize('"); debug_msg(fname); debug_msg("'): size was wrong for an apple2 dos3 image)\n");
#endif
    return ret;
  }
  char *tbuf=(char *)malloc(size);
  if(!tbuf) {
    fclose(f); f=NULL; debug_msg("DAppleDos3FS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret;
  }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  if(f) fclose(f);
  // Just about anything is okay with us...we'll check for errors later
  // size==143360
  //
  if(((tbuf[0]==(char)0x01)&&(tbuf[0x1]==(char)0xa5))) {
#ifdef DEBUG_VERBOSE
    debug_msg("Start of Apple DOS boot block found...probably...\n");
#endif
    ret=true;
  }
  //
  //debug_msg("DAppleDos3FS::recognize was successful!\n");
  free(tbuf);
  return ret;
}


/* STATIC */
bool DAppleDos3FS::recognizeFileExtension(const char *fname)
{
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DApple2FS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,".do")) return true;
  if(!strcmp(ext,".DO")) return true;
  if(!strcmp(ext,"nib")) return true;
  if(!strcmp(ext,"dsk")) return true;
  if(!strcmp(ext,"DSK")) return true;
  return false;
}


// NOTE: Is perfectly legit for this to fail...
bool DAppleDos3FS::readDirectory()
{
  if(cpm) return readDirectoryCpm();
  //else debug_msg("not cpm\n");
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleDos3FS::readDirectory...\n");
#endif
  freeDirCache();
  UINT t=0,tt=0,ss=0;
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleDos3FS::readDirectory() reading VTOC...\n");
#endif
  noFileSystem=false;
  readTrackSector(17,0);
  if(diskError) {
    debug_msg("DAppleDos3FS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
    noFileSystem=true;
    diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
    return false;
  }
  for(t=0;t<256;t++) diskName[t]=0;
  bool firstSector=true;
  bool ww=true;
  if((buf[1]<1)||(buf[1]>35)) {
    debug_msg("Bad track for first dir entry...this is probably cpm!\n");
    return false;
  }
  if((buf[2]<1)||(buf[2]>15)) {
    debug_msg("Bad sector for first dir entry...this is probably cpm!\n");
    return false;
  }
  while(ww) {
    tt=buf[1];  ss=buf[2];
#ifdef DEBUG_VERBOSE
    debug_msg("DAppleDos3FS::readDirectory() next dir block at t="); debug_int(tt); debug_msg(" s="); debug_int(ss); debug_msg("\n");
#endif
    if((tt==0)&&(ss==0)) ww=false;
    else {
      readTrackSector(tt,ss);
      if(diskError) {
        if(firstSector) {
          debug_msg("DAppleDos3FS "); debug_msg(DDISK_ERR_NON_DOS);
        }
        else {
          debug_msg("DAppleDos3FS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
        }
        noFileSystem=true;
        diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
        return false;
      }
      uint32_t off;
      bool theResult=false;
      for(t=0;t<6;t++) {
        off=t*35+11;
        theResult=readAppleDos3Entry(off);
        if(!theResult) return true;
      }
    }
    firstSector=false;
  }
  return true;
}


bool DAppleDos3FS::readAppleDos3Entry(long off)
{
  if(cpm) return readDirEntryCpm(off);
  //else debug_msg("not cpm\n");
#ifdef DEBUG_VERBOSE
  debug_msg("readAppleDos3Entry("); debug_int(off); debug_msg(")\n");
#endif
  UINT t=0;
  UINT fsize=buf[off+33]+(buf[off+34]*256);
  if(fsize) {
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<256;t++) theFile->name[t]=0;
      for(t=3;t<32;t++) {
        char c=buf[off+t]-128;
        if(c==' ') c=0;  // Really stupid hack for now...
        theFile->name[t-3]=c;
      }
      theFile->nativeType=buf[off+2]&0x7f;
      theFile->track=buf[off];
      theFile->sector=buf[off+1];
      theFile->sizeBlocks=fsize;
      theFile->data=NULL;
      theFile->size=theFile->sizeBlocks*256;
      theFile->block=0;
      theFile->curSeek=0;
      theFile->type=DDISK_TYPE_TEXT;
      if(theFile->nativeType&1) {
        theFile->type=DDISK_TYPE_BASIC;
      }
      if(theFile->nativeType&2) {
        theFile->type=DDISK_TYPE_BASIC;
      }
      if(theFile->nativeType&4) {
        theFile->type=DDISK_TYPE_BINARY;
      }
      theDir.insert((void *)theFile);
    }
    //else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  //else { debug_msg("DAppleDos3FS at "); debug_long(off); debug_msg(" "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
#ifdef DEBUG_VERBOSE
  //debug_msg("DAppleDos3FS at "); debug_long(off); debug_msg(" good file entry!\n");
#endif
  return true;
}


/*
void DAppleDos3FS::ls(const char *dname)
{
  if(noFileSystem) { debug_msg("DAppleDos3FS "); debug_msg(DDISK_ERR_NON_DOS); return; }
  //w->startMore();
  //w->dec();
  debug_msg("Disk:\n");
  theDir.jumpToHead();
  DDirEntry *aFile=(DDirEntry *)theDir.info();
  while(aFile) {
    char tchar='T';
    if(aFile->nativeType&1) tchar='I';
    if(aFile->nativeType&2) tchar='A';
    if(aFile->nativeType&4) tchar='B';
    debug_msg(tchar); debug_msg(" ";
    UINT afsize=aFile->sizeBlocks;
    if(afsize<100) debug_msg("0";
    if(afsize<10) debug_msg("0";
    debug_msg(afsize;
    debug_msg(" "); debug_msg(aFile->name;
    debug_msg("\n");
    theDir.advance();
    aFile=(DDirEntry *)theDir.info();
  }
  //w->hex();
  //w->endMore();
}
*/


bool DAppleDos3FS::cacheFile(void *fp)
{
  debug_msg("DAppleDos3FS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DAppleDos3FS::readGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleDos3FS::readGeometry()...\n");
#endif
  bool ret=false;
  //
  // TODO: Detect this...in case 3.5 in, etc.
  ret=true;
  thirteenSector=false;
  nibble=false;
  allTracksSameSectors=true;
  maxTrack=34;  maxSector=15;  maxSide=1;
  noTrackZero=false;  noSectorZero=false;
  blockSize=256;
  cachesDir=true;
  //
  detectSubFormat();
  //
  return ret;
}


bool DAppleDos3FS::readBootBlock()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleDos3FS::readBootBlock()...\n");
#endif
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DAppleDos3FS::detectSubFormat()
{
  debug_msg("DAppleDos3FS::detectSubFormat()...\n");
  subFormat=DAPPLE2_FORMAT_RAW;
  //
  cpm=detectCpm();
  if(cpm) {
    debug_msg("Probably a cpm disk!\n");
  }
  // By default
  return true;
}


bool DAppleDos3FS::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleDos3FS::sanityCheck()...\n");
#endif
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default guess it's ok...
  return true;
}

