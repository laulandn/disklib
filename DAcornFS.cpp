
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


#include "DAcornFS.h"


////////////////////////////////////////////////////////////////////////////////
//  DAcornFS Class
////////////////////////////////////////////////////////////////////////////////

DAcornFS::DAcornFS(const char *fname) : DDiskImageMem(fname)
{
  debug_msg("DAcornFS::DAcornFS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


void DAcornFS::init()
{
  noFileSystem=false;
  allTracksSameSectors=true;
  // Init for standard original disk
  maxTrack=40;
  maxSector=10;
  maxSide=1;
  blockSize=256;
  adfs=false;
}


DAcornFS::~DAcornFS()
{
  freeDirCache();
  diskMounted=false;
}


/* STATIC */
UINT DAcornFS::checkFilesystem(unsigned char *tbuf,FILE *f,UINT fsize)
{
  debug_msg("DAcornFS::checkFilesystem checking disk image...\n");
  UINT t=0;
  // The first 7 chars of disk name must not have bit 8 set...
  for(t=0;t<7;t++) if(tbuf[t]&0x80) return DACORN_FORMAT_UNKNOWN;
  debug_msg("(passed name test)\n");
  UINT oneZeroSix=tbuf[0x106];
  UINT totalSectors=tbuf[0x107]+((oneZeroSix<<8)&0xf00);
  size_t totalSize=totalSectors*256;
  // TODO: Check file size against this, but we can't because we don't have the file handle!
  UINT numEntries=tbuf[0x105]/8;
  // Check num entries was divisible by 8, fail if not
  if((numEntries*8)!=tbuf[0x105]) return DACORN_FORMAT_UNKNOWN;
  debug_msg("(passed numEntries test)\n");
  // Check all dir entries for non-null...
  for(t=0;t<numEntries;t++) {
    UINT doff=0x100+(t*8);
    UINT loadAddress=(tbuf[doff+1]*256)+tbuf[doff];
    loadAddress+=(tbuf[doff+6]<<14)&0x30000;
    UINT execAddress=(tbuf[doff+3]*256)+tbuf[doff+2];
    execAddress+=(tbuf[doff+6]<<10)&0x30000;
    size_t length=(tbuf[doff+5]*256)+tbuf[doff+4];
    length+=(tbuf[doff+6]<<12)&0x30000;
    UINT startSector=tbuf[doff+7];
    startSector+=(tbuf[doff+6]<<8)&0x0300;
    if((!loadAddress)&&(!startSector)&&(!execAddress)&&(!length)) return DACORN_FORMAT_UNKNOWN;
  }
  debug_msg("(passed dir entries test)\n");
  // If we made it here, it passed all our tests...
  return DACORN_FORMAT_ACORN;
}


/* STATIC */
bool DAcornFS::recognize(const char *fname)
{
  if(!recognizeFileExtension(fname)) return false;
  UINT theFormat=DACORN_FORMAT_UNKNOWN;
  bool ret=false;
  char *tstring=NULL;
  //debug_msg("DAcornFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  //
  // TODO: Some more checking here before we recognize it
  return ret;
}


/* STATIC */
bool DAcornFS::recognizeFileExtension(const char *fname)
{
  //debug_msg("DAcornFS::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
/*
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DAcornFS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  //if(!strcmp(ext,"atr")) return true;
*/
  // actually anything is ok...
  return true;
  return false;
}


const char *DAcornFS::guessSystem(const char *fname)
{
  return "acorn";
}


bool DAcornFS::readDirectory()
{
  debug_msg("DAcornFS::readDirectory...\n");
  freeDirCache();
  UINT t=0;
  UINT oneZeroSix=imageData[0x106];
  switch((oneZeroSix>>2)&0x3) {
    default:
      // NOTE: If these are non-null then it means this is an extended DFS...
      debug_msg("Byte 0x106 b3 and b2 are: "); debug_int(((oneZeroSix>>2)&0x3)); debug_msg("\n");
      break;
  }
  for(t=0;t<256;t++) diskName[t]=0;
  strcpy(diskName,"NO NAME");
  // NOTE: bit 7 of byte 0 is bit 10 of total size in extended DFS's...
  for(t=0;t<8;t++) diskName[t]=imageData[t]&0x7f;
  for(t=0;t<4;t++) diskName[t+8]=imageData[t+0x100]&0x7f;
  //
  UINT totalEntries=imageData[0x105]/8;
  debug_msg("totalEntries: "); debug_int(totalEntries); debug_msg("\n");
  UINT bootOption=(oneZeroSix>>4)&0x3;
  debug_msg("bootOption: "); debug_int(bootOption); debug_msg("\n");
  UINT totalSectors=imageData[0x107]+((oneZeroSix<<8)&0xf00);
  debug_msg("totalSectors: "); debug_int(totalSectors); debug_msg("\n");
  //
  debug_msg("maxTrack="); debug_int(maxTrack); debug_msg(" maxSector="); debug_int(maxSector); debug_msg(" maxSide="); debug_int(maxSide); debug_msg(" blockSize="); debug_int(blockSize); debug_msg("\n");
  //
  for(t=1;t<totalEntries+1;t++) {
    readDirEntry(t*8);
  }
  return true;
}


bool DAcornFS::cacheFile(void *fp)
{
  debug_msg("DAcornFS::cacheFile not implemented!\n");
  diskError=true;
  return false;
}


// TODO: uses imageData, not buf, since dir entries are spread over two sectors
bool DAcornFS::readDirEntry(long off)
{
  debug_msg("DAcornFS::readDirEntry("); debug_long(off); debug_msg(")...\n");
  long doff=off+0x100;
  UINT loadAddress=(imageData[doff+1]*256)+imageData[doff];
  loadAddress+=(imageData[doff+6]<<14)&0x30000;
  UINT execAddress=(imageData[doff+3]*256)+imageData[doff+2];
  execAddress+=(imageData[doff+6]<<10)&0x30000;
  size_t length=(imageData[doff+5]*256)+imageData[doff+4];
  length+=(imageData[doff+6]<<12)&0x30000;
  UINT startSector=imageData[doff+7];
  startSector+=(imageData[doff+6]<<8)&0x0300;
  // NOTE: We assume a valid entry if first char of name is non-null
  if(imageData[off]) {
    UINT t=0;
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<DDISK_MAX_NAMELEN;t++) theFile->name[t]=0;
      theFile->name[DDISK_MAX_NAMELEN-1]=0;
      // NOTE: Extended DFS's use bit 7 of some of the filename bytes for extra data like perms, etc...
      for(t=0;t<7;t++) theFile->name[t]=imageData[off+t]&0x7f;
      debug_msg("name is '"); debug_msg(theFile->name); debug_msg("'\n");
      debug_msg("directory prefix is "); debug_char((char)(imageData[off+7]&0x7f)); debug_msg("\n");
      theFile->nativeType=0;  // TODO:
      debug_msg("nativeType="); debug_int(theFile->nativeType); debug_msg("\n");
      theFile->type=DDISK_TYPE_BINARY; // TODO
      theFile->setPrivateVal1(loadAddress);  // NOTE: using private val in DDirEntry!
      debug_msg("loadAddress="); debug_int(loadAddress); debug_msg("\n");
      theFile->setPrivateVal2(execAddress);  // NOTE: using private val in DDirEntry!
      debug_msg("execAddress="); debug_int(execAddress); debug_msg("\n");
      theFile->track=0xff; // TODO:
      theFile->sector=0xff; // TODO:
      theFile->data=NULL;
      theFile->sizeBlocks=length/blockSize;
      theFile->size=length;
      debug_msg("size is "); debug_long(theFile->size); debug_msg(" bytes\n");
      debug_msg("size is "); debug_long(theFile->sizeBlocks); debug_msg(" blocks\n");
      theFile->block=startSector;
      debug_msg("startSector="); debug_int(startSector); debug_msg("\n");
      theFile->curSeek=0;
      theDir.append((void *)theFile);
    }
    else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  else { debug_msg("DAcornFS "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
  return true;
}


bool DAcornFS::readGeometry()
{
  debug_msg("DAcornFS::readGeometry()...\n");
  bool ret=false;
  blockSize=0;
  maxTrack=0;
  maxSector=0;
  maxSide=0;
  return ret;
}


bool DAcornFS::readBootBlock()
{
  debug_msg("DAcornFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DAcornFS::detectSubFormat()
{
  // By default
  return false;
}


bool DAcornFS::sanityCheck()
{
  debug_msg("DAcornFS::sanityCheck()...\n");
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default
  return false;
}
