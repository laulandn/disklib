

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


#include "DApple2FS.h"


////////////////////////////////////////////////////////////////////////////////
//  DApple2FS Class
////////////////////////////////////////////////////////////////////////////////

DApple2FS::DApple2FS(const char *fname) : DDiskImageMem(fname)
{
  //debug_msg("DApple2FS::DApple2FS(...,'"); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


void DApple2FS::init()
{
#ifdef DEBUG_VERBOSE
  //debug_msg("DApple2FS disk image.\n");
#endif
  //
  nibble=false;
  twoMG=false;
  cpm=false;
  interleave=EDISK2_INTERLEAVE0;  // Assume no interleave for now...
  subFormat=DAPPLE2_FORMAT_RAW;
  //
  allTracksSameSectors=true;
  maxTrack=34;  maxSector=15;  maxSide=1;
  noTrackZero=false;  noSectorZero=false;
  blockSize=256;
}


DApple2FS::~DApple2FS()
{
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


bool DApple2FS::detectCpm()
{
  debug_msg("DApple2FS::detectCpm()...\n");
  bool ret=false;
  //
  // Look for apple dos 3 valid dir
  readTrackSector(17,0);
  if((buf[1]<1)||(buf[1]>35)) {
    debug_msg("Bad track for first dir entry...this is probably cpm!\n");
    ret=true;
  }
  if((buf[2]<1)||(buf[2]>15)) {
    debug_msg("Bad sector for first dir entry...this is probably cpm!\n");
    ret=true;
  }
  //
  if(ret) debug_msg("Disk is an apple2 CP/M disk...\n"); //else debug_msg("was not cpm\n");
  return ret;
}


// NOTE: This is actually a pretty terrible hack using a 2nd disk!
bool DApple2FS::readDirectoryCpm()
{
  debug_msg("DApple2FS::readDirectoryCpm()...\n");
  freeDirCache();
  bool ret=false;
  if(!cpmDisk) {
    cpmDisk=new DCpmFS(theFileName);
    if(cpmDisk->readDirectory()) {
      debug_msg("Copying cpm file entries...\n");
      bool reading=true;
      theDir.jumpToHead();
      cpmDisk->getTheDir()->jumpToHead();
      DDirEntry *theFile=NULL;
      while(reading) {
        debug_msg("reading...\n");
        theFile=(DDirEntry *)cpmDisk->getTheDir()->info();
        if(!theFile) reading=false;
        else {
          debug_msg("Cpm filename is '"); debug_msg(theFile->name); debug_msg("'\n");
          theDir.append(theFile);
        }
        cpmDisk->getTheDir()->advance();
      }
      ret=true;
    }
    else debug_msg("cpmDisk->readDirectory() failed!\n");
  }
  return ret;
}


// NOTE: This is actually a pretty terrible hack using a 2nd disk!
bool DApple2FS::readDirEntryCpm(long offset)
{
  debug_msg("DApple2FS::readDirEntryCpm()...\n");
  return cpmDisk->readDirEntry(offset);
}


////////////////////////////////////////////////////////////////////////////////
//  DApple2FSNibbleHelper Class
////////////////////////////////////////////////////////////////////////////////

bool DApple2FSNibbleHelper::encodeNibs(UINT tnum,uint8_t *src,UINT ssize,uint8_t *dest,UINT dsize)
{
  if(ssize!=EDISK2_TRACK_SIZE) {
    debug_msg("DApple2FSNibbleHelper::encodeNibs, source "); debug_msg(DDISK_ERR_BAD_BUFFER);
    return false;
  }
  if(dsize!=EDISK2_NIB_TRACK_SIZE) {
    debug_msg("DApple2FSNibbleHelper::encodeNibs, dest "); debug_msg(DDISK_ERR_BAD_BUFFER);
    return false;
  }
  if(tnum>35) {
    debug_msg("DApple2FSNibbleHelper::encodeNibs, "); debug_msg(DDISK_ERR_BAD_TRACK);
    return false;
  }
  UINT soffset=0,doffset=0;
  for(UINT t=0;t<16;t++) {
    encodeNibSector(tnum,t,src+soffset,dest+doffset);
    soffset+=EDISK2_DECODED_SECTOR_SIZE;
    doffset+=EDISK2_ENCODED_SECTOR_SIZE;
  }
  return true;
}


bool DApple2FSNibbleHelper::decodeNibs(UINT tnum,uint8_t *src,UINT ssize,uint8_t *dest,UINT dsize)
{
  if(ssize!=EDISK2_NIB_TRACK_SIZE) {
    debug_msg("DApple2FSNibbleHelper::encodeNibs, source "); debug_msg(DDISK_ERR_BAD_BUFFER);
    return false;
  }
  if(dsize!=EDISK2_TRACK_SIZE) {
    debug_msg("DApple2FSNibbleHelper::encodeNibs, dest "); debug_msg(DDISK_ERR_BAD_BUFFER);
    return false;
  }
  if(tnum>35) {
    debug_msg("DApple2FSNibbleHelper::decodeNibs, "); debug_msg(DDISK_ERR_BAD_TRACK);
    return false;
  }
  UINT soffset=0,doffset=0;
  for(UINT t=0;t<16;t++) {
    decodeNibSector(tnum,t,src+soffset,dest+doffset);
    soffset+=EDISK2_ENCODED_SECTOR_SIZE;
    doffset+=EDISK2_DECODED_SECTOR_SIZE;
  }
  return true;
}


bool DApple2FSNibbleHelper::encodeNibSector(UINT tnum,UINT snum,uint8_t *src,uint8_t *dest)
{
  // NOTE: we write a gap BEFORE the address field, but none after the
  // data field...this is completely arbitrary.
  UINT t;
  // write gap 1, 40 bytes
  uint32_t offset=0;
  for(t=0;t<40;t++) {
    dest[offset+t]=0xff;  // TODO: Is this right?  No 0 bits?
  }
  // write address field, 15 bytes
  offset=40;
  // prolog
  dest[offset]=0xd5; dest[offset+1]=0xaa; dest[offset+2]=0x96; offset+=3;
  // disk volume
  offset+=2;
  // track address
  offset+=2;
  // sector address
  offset+=2;
  // checksum
  offset+=2;
  // epilog
  dest[offset]=0xde; dest[offset+1]=0xaa; dest[offset+2]=0xeb; offset+=3;
  // write gap 2, 8 bytes
  offset=55;
  for(t=0;t<8;t++) {
    dest[offset+t]=0xff;  // TODO: Is this right?  No 0 bits?
  }
  // write data field, 349 bytes
  offset=63;
  // prolog
  dest[offset]=0xd5; dest[offset+1]=0xaa; dest[offset+2]=0xad; offset+=3;
  // the actual encoded data...
  for(t=0;t<342;t++) {
    offset++;
  }
  // checksum
  offset+=1;
  // epilog
  dest[offset]=0xde; dest[offset+1]=0xaa; dest[offset+2]=0xeb; offset+=3;
  return false;
}


bool DApple2FSNibbleHelper::decodeNibSector(UINT tnum,UINT snum,uint8_t *src,uint8_t *dest)
{
  debug_msg("DApple2FSNibbleHelper::decodeNibSector, "); debug_msg(DDISK_ERR_NO_IMPL);
  // TODO: maybe read gap 1, ? bytes
  // TODO: read address field, 15 bytes
  // TODO: read gap 2, ? bytes
  // TODO: read data field, 349 bytes
  // TODO: maybe read gap 3, ? bytes
  return false;
}


////////////////////////////////////////////////////////////////////////////////
//  DApplePascalFS Class
////////////////////////////////////////////////////////////////////////////////

DApplePascalFS::DApplePascalFS(const char *fname) : DApple2FS(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DApplePascalFS::DApplePascalFS("); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  Mount(fname);
}


const char *DApplePascalFS::guessSystem(const char *fname)
{
  return "apple2e";
}


void DApplePascalFS::init()
{
#ifdef DEBUG_VERBOSE
  //debug_msg("DApplePascalFS disk image.\n");
#endif
  nibble=false;
  allTracksSameSectors=true;
  maxTrack=34;  maxSector=15;  maxSide=1;
  noTrackZero=false;  noSectorZero=false;
  blockSize=256;
  cachesDir=true;
}


DApplePascalFS::~DApplePascalFS()
{
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


/* STATIC */
bool DApplePascalFS::recognize(const char *fname)
{
  if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  if(!fname) return ret;
  FILE *f=fopen(fname,"r");
  if(!f) { debug_msg("DApplePascalFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN); return ret; }
  size_t size=getSize(fname);
  if(size!=143360) {
    fclose(f); f=NULL;
#ifdef DEBUG_VERBOSE
    debug_msg("(DApplePascalFS::recognize('"); debug_msg(fname); debug_msg("'): size was wrong for an apple2 prodos image)\n");
#endif
    return ret;
  }
  char *tbuf=(char *)malloc(size);
  if(!tbuf) {
    /*fclose(f); f=NULL;*/
    debug_msg("DApplePascalFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER);
    return ret;
  }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  if(f) fclose(f);
  //
  if(!strncmp((char *)(tbuf+0xd7),"SYSTEM.APPLE",12)) ret=true;
#ifdef DEBUG_VERBOSE
  if(ret) debug_msg("Found pascal signature!\n");
#endif
  //
#ifdef DEBUG_VERBOSE
  dumpBufHex((uint8_t *)tbuf,256);
#endif
  //
  //debug_msg("DAppleProDosFS::recognize was successful!\n");
  free(tbuf);
  return ret;
}


/* STATIC */
bool DApplePascalFS::recognizeFileExtension(const char *fname)
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


// Isn't this just a copy of the DOS3 version?
// NOTE: Is perfectly legit for this to fail...
bool DApplePascalFS::readDirectory()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DApplePascalFS::readDirectory...\n");
#endif
  freeDirCache();
  UINT t=0,tt=0,ss=0;
  //debug_msg("Reading VTOC...\n");
  noFileSystem=false;
  readTrackSector(0,11);
  if(diskError) {
    debug_msg("DApplePascalFS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
    noFileSystem=true;
    diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
    return false;
  }
  for(t=0;t<256;t++) diskName[t]=0;
  for(t=0;t<16;t++) {
    if((buf[t+7]>0x2d)&&(buf[t+7]<128)) {
      diskName[t]=buf[t+7];
#ifdef DEBUG_VERBOSE
      debug_char(diskName[t]);
#endif
    }
  }
#ifdef DEBUG_VERBOSE
  debug_msg("\n");
#endif
  bool ww=true;
  while(ww) {
    /*
    tt=buf[1];  ss=buf[2];
#ifdef DEBUG_VERBOSE
    debug_msg("Next dir block at t="); debug_int(tt); debug_msg(" s="); debug_int(ss); debug_msg("\n");
#endif
    if((tt==0)&&(ss==0)) ww=false;
    else {
      readTrackSector(tt,ss);
      if(diskError) {
        debug_msg("DApplePascalFS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
        noFileSystem=true;
        diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
        return false;
      }
      */
      uint32_t off;
      bool theResult=false;
      for(t=0;t<32;t++) {
        off=t*26+32;
        theResult=readApplePascalEntry(off);
        if(!theResult) return true;
      }
      ww=false;
    //}
  }
  return true;
}


bool DApplePascalFS::readApplePascalEntry(long off)
{
#ifdef DEBUG_VERBOSE
  debug_msg("readApplePascalEntry("); debug_long(off); debug_msg(")\n");
#endif
  UINT t=0;
  UINT fsize=1;//buf[off+33]+(buf[off+34]*256);
  if(fsize) {
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<256;t++) theFile->name[t]=0;
      UINT nameSize=buf[off];
      if(!nameSize) return false;
      if(nameSize>32) return false;
#ifdef DEBUG_VERBOSE
      debug_msg("nameSize is "); debug_int(nameSize); debug_msg("\n");
#endif
      for(t=0;t<nameSize;t++) {
        theFile->name[t]=buf[off+t+1];
#ifdef DEBUG_VERBOSE
        debug_char(theFile->name[t]);
#endif
      }
#ifdef DEBUG_VERBOSE
      debug_msg("\n");
#endif
      theFile->nativeType=0;//buf[off+2]&0x7f;
      theFile->track=0;//buf[off];
      theFile->sector=0;//buf[off+1];
      theFile->sizeBlocks=fsize;
      theFile->data=NULL;
      theFile->size=theFile->sizeBlocks*256;
      theFile->block=0;
      theFile->curSeek=0;
      theFile->type=DDISK_TYPE_TEXT;
      /*
      if(theFile->nativeType&1) {
        theFile->type=DDISK_TYPE_BASIC;
      }
      if(theFile->nativeType&2) {
        theFile->type=DDISK_TYPE_BASIC;
      }
      if(theFile->nativeType&4) {
        theFile->type=DDISK_TYPE_BINARY;
      }
      */
      theDir.insert((void *)theFile);
    }
    else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  else { debug_msg("DApplePascalFS "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
  return true;
}


bool DApplePascalFS::cacheFile(void *fp)
{
  debug_msg("DApplePascalFS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DApplePascalFS::readGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DApplePascalFS::readGeometry()...\n");
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
  return ret;
}


bool DApplePascalFS::readBootBlock()
{
  debug_msg("DApplePascalFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DApplePascalFS::detectSubFormat()
{
  // By default
  return false;
}


bool DApplePascalFS::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DApplePascalFS::sanityCheck()...\n");
#endif
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default guess it's ok...
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//  DAppleCpmFS Class
////////////////////////////////////////////////////////////////////////////////

DAppleCpmFS::DAppleCpmFS(const char *fname) : DApple2FS(fname)
{
  debug_msg("DAppleCpmFS::DAppleCpmFS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


const char *DAppleCpmFS::guessSystem(const char *fname)
{
  return "apple2e";
}


void DAppleCpmFS::init()
{
#ifdef DEBUG_VERBOSE
  //debug_msg("DAppleCpmFS disk image.\n");
#endif
  nibble=false;
  allTracksSameSectors=true;
  maxTrack=34;  maxSector=15;  maxSide=1;
  noTrackZero=false;  noSectorZero=false;
  blockSize=256;
  cachesDir=true;
}


DAppleCpmFS::~DAppleCpmFS()
{
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


/* STATIC */
bool DAppleCpmFS::recognize(const char *fname)
{
  if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  if(!fname) return ret;
  FILE *f=fopen(fname,"rb");
  if(!f) { debug_msg("DAppleCpmFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN); return ret; }
  size_t size=getSize(fname);
  if((size!=143360)&&(size!=143363)&&(size!=232960)&&(size!=819271)) {
    //fclose(f); f=NULL;
//#ifdef DEBUG_VERBOSE
    //debug_msg("(DAppleCpmFS::recognize('"); debug_msg(fname); debug_msg("'): size was wrong for an apple2 prodos image)\n");
//#endif
    return ret;
  }
  char *tbuf=(char *)malloc(size);
  if(!tbuf) {
    /*fclose(f); f=NULL;*/ debug_msg("DAppleCpmFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret;
  }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  if(f) fclose(f);
  // Just about anything is okay with us...we'll check for errors later
  // size==143360
  //
  dumpBufHex((uint8_t *)tbuf,256);
  //
  //debug_msg("DAppleProDosFS::recognize was successful!\n");
  free(tbuf);
  ret=true;
  return ret;
}


/* STATIC */
bool DAppleCpmFS::recognizeFileExtension(const char *fname)
{
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DApple2FS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,".do")) return true;
  if(!strcmp(ext,"nib")) return true;
  if(!strcmp(ext,"dsk")) return true;
  return false;
}


// Isn't this just a copy of the DOS3 version?
// NOTE: Is perfectly legit for this to fail...
bool DAppleCpmFS::readDirectory()
{
  debug_msg("DAppleCpmFS::readDirectory...\n");
  freeDirCache();
  UINT t=0,tt=0,ss=0;
  //debug_msg("Reading VTOC...\n");
  noFileSystem=false;
  readTrackSector(17,0);
  if(diskError) {
    debug_msg("DAppleCpmFS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
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
        debug_msg("DAppleCpmFS Can't read dir, "); debug_msg(DDISK_ERR_DISK);
        noFileSystem=true;
        diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
        return false;
      }
      uint32_t off;
      bool theResult=false;
      for(t=0;t<6;t++) {
        off=t*35+11;
        theResult=readAppleCpmEntry(off);
        if(!theResult) return true;
      }
    }
  }
  return true;
}


bool DAppleCpmFS::readAppleCpmEntry(long off)
{
  debug_msg("readAppleCpmEntry("); debug_long(off); debug_msg(")\n");
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
  else { debug_msg("DAppleCpmFS "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
  return true;
}


bool DAppleCpmFS::cacheFile(void *fp)
{
  debug_msg("DAppleCpmFS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  return false;
}


bool DAppleCpmFS::readGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleCpmFS::readGeometry()...\n");
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
  return ret;
}


bool DAppleCpmFS::readBootBlock()
{
  debug_msg("DAppleCpmFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DAppleCpmFS::detectSubFormat()
{
  // By default
  return false;
}


bool DAppleCpmFS::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DAppleCpmFS::sanityCheck()...\n");
#endif
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default guess it's ok...
  return true;
}
