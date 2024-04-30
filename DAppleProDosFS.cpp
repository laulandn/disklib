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


#include "DAppleProDosFS.h"


////////////////////////////////////////////////////////////////////////////////
//  DAppleProDosFS Class
////////////////////////////////////////////////////////////////////////////////

DAppleProDosFS::DAppleProDosFS(const char *fname) : DApple2FS(fname)
{
  debug_msg("DAppleProDosFS::DAppleProDosFS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


const char *DAppleProDosFS::guessSystem(const char *fname)
{
  return "apple2e";
}


void DAppleProDosFS::init()
{
#ifdef DEBUG_VERBOSE
  //debug_msg("DAppleProDosFS disk image.\n");
#endif
  nibble=false;
  allTracksSameSectors=true;
  maxTrack=34;  maxSector=15;  maxSide=1;
  noTrackZero=false;  noSectorZero=false;
  blockSize=256;
  cachesDir=true;
}


DAppleProDosFS::~DAppleProDosFS()
{
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


/* STATIC */
bool DAppleProDosFS::recognize(const char *fname)
{
  //if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  if(!fname) return ret;
  FILE *f=fopen(fname,"rb");
  if(!f) {
#ifdef DEBUG_VERBOSE
    debug_msg("DAppleProDosFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN);
#endif
    return false;
  }
  size_t size=getSize(fname);
  /*
  if((size!=143360)&&(size!=143363)&&(size!=232960)&&(size!=819271)) {
    //fclose(f); f=NULL;
//#ifdef DEBUG_VERBOSE
    //debug_msg("(DAppleProDosFS::recognize('"); debug_msg(fname); debug_msg("'): size was wrong for an apple2 prodos image)\n");
//#endif
    return ret;
  }
  */
  char *tbuf=(char *)malloc(size);
  if(!tbuf) {
    /*fclose(f); f=NULL;*/ debug_msg("DAppleProDosFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret;
  }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  //
  if(f) fclose(f);
  //
  // Just about anything is okay with us...we'll check for errors later
  // size==143360
  //
  if(((tbuf[0]==(char)0x01)&&(tbuf[0x1]==(char)0x38))) {
#ifdef DEBUG_VERBOSE
    debug_msg("Start of Apple ProDOS boot block found...probably...\n");
#endif
    ret=true;
  }
  if(tbuf[0]==0x0b) ret=true;  // Maybe this is a good test?
  char *tstring=NULL;
  tstring=(char *)"2IMGWOOF";
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) ret=true;
  // TODO: Is this really a fixed string or should I be looking someplace else?
  tstring=(char *)"PRODOS";
  if(!strncmp((char *)tbuf+259,tstring,strlen(tstring))) ret=true;
  if(!strncmp((char *)tbuf+289,tstring,strlen(tstring))) ret=true;
  if(!strncmp((char *)tbuf+353,tstring,strlen(tstring))) ret=true;
  if(ret) debug_msg("DAppleProDosFS::recognize found ProDOS sig of some kind\n");
  //
#ifdef DEBUG_VERBOSE
  dumpBufHex((uint8_t *)tbuf,256);
#endif
  //
  //debug_msg("DAppleProDosFS::recognize was successful!\n");
  free(tbuf);
  //ret=true;
  return ret;
}


/* STATIC */
bool DAppleProDosFS::recognizeFileExtension(const char *fname)
{
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DApple2FS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,".po")) return true;
  if(!strcmp(ext,".PO")) return true;
  if(!strcmp(ext,"2mg")) return true;
  return false;
}


// Isn't this just a copy of the DOS3 version?
// NOTE: Is perfectly legit for this to fail...
bool DAppleProDosFS::readDirectory()
{
  if(cpm) return readDirectoryCpm();
  //else debug_msg("not cpm\n");
  debug_msg("DAppleProDosFS::readDirectory...\n");
  freeDirCache();
  UINT t=0,tt=0,ss=0;
  //debug_msg("Reading VTOC...\n");
  noFileSystem=false;
  readTrackSector(17,0);
  if(diskError) {
    debug_msg("DAppleProDosFS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
    noFileSystem=true;
    diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
    return false;
  }
  for(t=0;t<256;t++) diskName[t]=0;
  bool ww=true;
  while(ww) {
    tt=buf[1];  ss=buf[2];
#ifdef DEBUG_VERBOSE
    debug_msg("Next dir block at t="); debug_int(tt); debug_msg(" s="); debug_int(ss); debug_msg("\n");
#endif
    if((tt==0)&&(ss==0)) ww=false;
    else {
      readTrackSector(tt,ss);
      if(diskError) {
        debug_msg("DAppleProDosFS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
        noFileSystem=true;
        diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
        return false;
      }
      uint32_t off;
      bool theResult=false;
      for(t=0;t<6;t++) {
        off=t*35+11;
        theResult=readAppleProDosEntry(off);
        if(!theResult) return true;
      }
    }
  }
  return true;
}


bool DAppleProDosFS::readAppleProDosEntry(long off)
{
  if(cpm) return readDirEntryCpm(off);
  //else debug_msg("not cpm\n");
  debug_msg("readAppleProDosEntry("); debug_long(off); debug_msg(")\n");
  UINT t=0;
  UINT fsize=buf[off+33]+(buf[off+34]*256);
  if(fsize) {
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<256;t++) theFile->name[t]=0;
      for(t=3;t<32;t++) {
        theFile->name[t-3]=buf[off+t]-128;
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
    else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  else { debug_msg("DAppleProDosFS "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
  return true;
}


bool DAppleProDosFS::cacheFile(void *fp)
{
  debug_msg("DAppleProDosFS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DAppleProDosFS::readGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleProDosFS::readGeometry()...\n");
#endif
  bool ret=false;
  //
  // TODO: Detect this...in case 3.5 in, etc.
  ret=true;
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


bool DAppleProDosFS::readBootBlock()
{
  debug_msg("DAppleProDosFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DAppleProDosFS::detectSubFormat()
{
  debug_msg("DAppleProDosFS::detectSubFormat()...\n");
  subFormat=DAPPLE2_FORMAT_RAW;  // assume by default...
  char *tstring=NULL;
  tstring=(char *)"2IMGWOOF";
  if(!strncmp((char *)imageData,tstring,strlen(tstring))) { subFormat=DAPPLE2_FORMAT_2MG; twoMG=true; }
  tstring=(char *)"WOZ";
  if(!strncmp((char *)imageData,tstring,strlen(tstring))) { subFormat=DAPPLE2_FORMAT_WOZ; }
  // TODO: Is this really a fixed string or should I be looking someplace else?
  tstring=(char *)"PRODOS";
  if(!strncmp((char *)imageData+259,tstring,strlen(tstring))) { subFormat=DAPPLE2_FORMAT_RAW; }
  if(!strncmp((char *)imageData+289,tstring,strlen(tstring))) { subFormat=DAPPLE2_FORMAT_RAW; }
  if(!strncmp((char *)imageData+353,tstring,strlen(tstring))) { subFormat=DAPPLE2_FORMAT_RAW; }
  // By default
  return true;
}


bool DAppleProDosFS::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleProDosFS::sanityCheck()...\n");
#endif
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default guess it's ok...
  return true;
}

