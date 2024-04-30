
//#define DEBUG_OUT std::cerr
#define DEBUG_OUT dBug
#define ERR_OUT std::cerr
//#define DEBUG_OUT *aNullStream
//#define CONS_OUT std::cout
#define CONS_OUT *diskLibOutputStream
#define CONS_IN std::cin
//#define CONS_IN *inStream


#ifdef DEBUG_VERBOSE
//#undef DEBUG_VERBOSE
#endif


#include <string.h>


#include "DTrs80FS.h"


////////////////////////////////////////////////////////////////////////////////
//  DTrs80FS Class
////////////////////////////////////////////////////////////////////////////////


DTrs80FS::DTrs80FS(const char *fname) : DDiskImageMem(fname)
{
  debug_msg("DTrs80FS::DTrs80FS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
}


void DTrs80FS::init()
{
  dmkImage=false;
  for(UINT t=0;t<256;t++) { gat[t]=0; hit[t]=0; }
}


DTrs80FS::~DTrs80FS()
{
  freeDirCache();
  diskMounted=false;
}


/* STATIC */
bool DTrs80FS::recognize(const char *fname)
{
  UINT ret=DTRS80_FORMAT_UNKNOWN;
  //debug_msg("DTrs80FS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  size_t size=getSize(fname);
  if((size==228496)||(size==261136)||(size==267664)) {
    // possibly a dmk...should check more...
    ret=true;
  }
  if(size==204800) {
    // possibly an 80ssd jv1...should check more...
    ret=true;
  }
  if(size==91408) {
    // possibly...
    ret=true;
  }
  // TODO: Some more checking here before we recognize it
  return ret;
}


/* STATIC */
bool DTrs80FS::recognizeFileExtension(const char *fname)
{
  //debug_msg("DTrs80FS::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
/*
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DTrs80FS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  //if(!strcmp(ext,"atr")) return true;
*/
  return false;
}


const char *DTrs80FS::guessSystem(const char *fname)
{
  return "trs80";
}


bool DTrs80FS::readDirectory()
{
  debug_msg("DTrs80FS::readDirectory...\n");
  freeDirCache();
  switch(imageDataSize)
  {
    case 204800:
      // Let's guess an 80 track jv1 image...
      maxTrack=80;
      debug_msg("DTrs80FS::readDirectory guessing an 80 track sssd jv1 image...\n");
      break;
    case 228496:
    case 261136:
    case 267664:
      dmkImage=true;
      debug_msg("DTrs80FS::readDirectory DMK images not implemented!\n");
      break;
    default:
      debug_msg("DTrs80FS::readDirectory can't figure out image format!\n");
      diskError=true;
      diskStatus=DDISK_STATUS_BAD_FORMAT;
      break;
  }
  UINT t=0;
  for(t=0;t<256;t++) diskName[t]=0;
  strcpy(diskName,"NO NAME");
  //
  // Read GAT
  debug_msg("DTrs80FS::readDirectory reading GAT...\n");
  readTrackSector(17,0);
  for(t=0;t<256;t++) gat[t]=buf[t];
  // TODO: Use info here...
  //
  // Read HIT
  debug_msg("DTrs80FS::readDirectory reading HHIT...\n");
  readTrackSector(17,1);
  for(t=0;t<256;t++) hit[t]=buf[t];
  // TODO: Use info here...
  //
  // Now read directory entries
  // Sectors 2-10
  for(t=0;t<8;t++) {
    readTrackSector(17,2+t);
    //dumpBufHex((uint8_t *)buf,blockSize);
    // 8 entries per block
    for(UINT i=0;i<8;i++) {
      readDirEntry(i*32);
    }
  }
  return true;
}


bool DTrs80FS::cacheFile(void *fp)
{
  debug_msg("DTrs80FS::cacheFile not implemented!\n");
  diskError=true;
  return false;
}


// TODO: filenames have trailing spaces...
bool DTrs80FS::readDirEntry(long off)
{
  debug_msg("DTrs80FS::readDirEntry("); debug_long(off); debug_msg(")...\n");
  // Only valid if type is non-zero
  if(buf[off]) {
    UINT t=0;
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<DDISK_MAX_NAMELEN;t++) theFile->name[t]=0;
      theFile->name[DDISK_MAX_NAMELEN-1]=0;
      for(t=5;t<13;t++) theFile->name[t-5]=buf[off+t];
      theFile->name[8]='.';
      for(t=13;t<16;t++) theFile->name[t-4]=buf[off+t];
      debug_msg("name is '"); debug_msg(theFile->name); debug_msg("'\n");
      theFile->sizeBlocks=0;  // TODO: use GAT and HIT?!?
      theFile->nativeType=buf[0];
      debug_msg("nativeType="); debug_int(theFile->nativeType); debug_msg("\n");
      theFile->type=DDISK_TYPE_BINARY; // TODO
      theFile->track=0xff; // TODO: use GAT and HIT?!?
      theFile->sector=0xff; // TODO: use GAT and HIT?!?
      theFile->data=NULL;
      theFile->size=theFile->sizeBlocks*blockSize;
      theFile->block=0;
      theFile->curSeek=0;
      theDir.append((void *)theFile);
    }
    else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  /*else { debug_msg("DTrs80FS "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }*/
  return true;
}


bool DTrs80FS::readGeometry()
{
  debug_msg("DTrs80FS::readGeometry()...\n");
  bool ret=false;
  // guessing for now...
  allTracksSameSectors=true;
  noSectorZero=false;
  noTrackZero=false;
  maxTrack=80;
  maxSector=9;
  maxSide=1;
  blockSize=256;
  ret=true;
  return ret;
}


bool DTrs80FS::readBootBlock()
{
  debug_msg("DTrs80FS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  // TODO!
  return ret;
}


bool DTrs80FS::detectSubFormat()
{
  if(!basicSanityCheck()) return false;
  // By default
  return false;
}


bool DTrs80FS::sanityCheck()
{
  debug_msg("DTrs80FS::sanityCheck()...\n");
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

/*
NOTES:

0xeb 0x34 0x90 first three bytes?
0xeb 0x3c 0x90 first three bytes?

0x03-0x0a creator dos name?
0x2b-0x35 disk name?
0x36-0x3d fat name?

*/


/*
NOTES:

first sector is bootstrap
sectors 2-3 are "encoded copyright message"
sectors 4-5 are "tables"

directory is track 17
offset for that is 17*256*10=43520
sector 0 is "granuile allocation table"
sector 1 is "hash index table"
Remaining 8 are directory
32 bytes each, 8 per block, thus 64 files max

"hit" format
each filename has a byte
only first 8 bytes of each 32 byte segment is used
last hex digit plus 2 is sector of dir track where entry is
first hex digit times 16 is byte offset into that sector

32 byte entry format
either "file primary directory entry" or "file extention directory entry"

"fpde" format
0: type
1-2: unused
3: end of file
4: logical record length (only newer doses)
5-c: filename
d-f: file extension
10-13: "passwords"
14-15: "eof" relative sector
16-1f: five "extents"

Total of 89600 bytes 5.25in 35*10*256
Total of 509184 for 8in disk 77*26*256

*/
