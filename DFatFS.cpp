
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


#include "DFatFS.h"


////////////////////////////////////////////////////////////////////////////////
//  DFatFS Class
////////////////////////////////////////////////////////////////////////////////

DFatFS::DFatFS(const char *fname) : DDiskImageMem(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatFS::DFatFS("); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  Mount(fname);
}


void DFatFS::init()
{
#ifdef DEBUG_VERBOSE
  //debug_msg("DFatFS::init()\n");
#endif
  noFileSystem=false;
  allTracksSameSectors=true;
  imageFormat=DFAT_FORMAT_UNKNOWN;
  // NOTE: 1.4m floppy geometry for starters
  //maxTrack=80; maxSector=18; maxSide=2; blockSize=512;
  maxTrack=0; maxSector=0; maxSide=0; blockSize=0;
  //
  header=NULL;
}


DFatFS::~DFatFS()
{
  freeDirCache();
  diskMounted=false;
}


/* STATIC */
bool DFatFS::recognize(const char *fname)
{
  UINT theFormat=DFAT_FORMAT_UNKNOWN;
  if(!recognizeFileExtension(fname)) {
#ifdef DEBUG_VERBOSE
    debug_msg("DFatFS::recognize bad file extension?\n");
#endif
    //return false;
  }
  bool ret=false;
  /*
  size_t size=0;
  size=getSize(fname);
  bool sizeMaybe=false;
  if(size==1228800) sizeMaybe=true;  // ???
  if(size==737280) sizeMaybe=true;  // ???
  if(size==1474560) sizeMaybe=true;  // ???
  if(size==368540) sizeMaybe=true;  // ???
  if(size==163840) sizeMaybe=true;  // ???
  if(size==184320) sizeMaybe=true;  // IMA???
#ifdef DEBUG_VERBOSE
  if(!sizeMaybe) {
    //debug_msg("DFatFS::recognize disk image size is "); debug_int(size); debug_msg(" which doesn't look right for an msdos disk...\n");
    //return false;
  }
  else ret=true;
#endif
  //
  */
  unsigned char *tbuf=recognizeHelper(fname);
  if(!tbuf) { return false; }
  //
  if((tbuf[0]==(unsigned char)0xeb)&&(tbuf[2]==(unsigned char)0x90)) { ret=true; theFormat=DFAT_FORMAT_RAW; }
  char *tstring=(char *)"FAT12   ";
  if(!strncmp((char *)(tbuf+0x36),tstring,strlen(tstring))) { ret=true; theFormat=DFAT_FORMAT_RAW; }
  tstring=(char *)"MSXDOS  SYS";
  if(!strncmp((char *)(tbuf+0x7a),tstring,strlen(tstring))) { ret=true; theFormat=DFAT_FORMAT_RAW; }
  tstring=(char *)"MFM_DISK";
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) { ret=true; theFormat=DFAT_FORMAT_MFM_DISK; } // Oric disk image?
#ifdef DEBUG_VERBOSE
  if(theFormat!=DFAT_FORMAT_UNKNOWN) debug_msg("(theFormat was set)\n");
#endif
  //
  bool oem=false;
  tstring=(char *)"MSDOS";
  if(!strncmp((char *)tbuf+3,tstring,strlen(tstring))) oem=true;
  tstring=(char *)"MSWIN";
  if(!strncmp((char *)tbuf+3,tstring,strlen(tstring))) oem=true;
  tstring=(char *)"IBM ";
  if(!strncmp((char *)tbuf+3,tstring,strlen(tstring))) oem=true;
  tstring=(char *)"mkdosfs";
  if(!strncmp((char *)tbuf+3,tstring,strlen(tstring))) oem=true;
  tstring=(char *)"FreeDOS ";
  if(!strncmp((char *)tbuf+3,tstring,strlen(tstring))) oem=true;
  tstring=(char *)"IHC";
  if(!strncmp((char *)tbuf+8,tstring,strlen(tstring))) oem=true;
#ifdef DEBUG_VERBOSE
  if(oem) debug_msg("(oem was set)\n");
#endif
  if(!oem) {
    //debug_msg("DFatFS::recognize OEM name failed!\n");
    //return false;
  }
  else ret=true;
  //
  bool goodHeader=false;
  unsigned offset=0xb;
  DFatFSHeader *header=new DFatFSHeader(tbuf,offset);
  //dumpBufHex((uint8_t *)imageData,512);
  //header->debugDump();
  if(header->fatBytesPerSector!=512) {
#ifdef DEBUG_VERBOSE
    debug_msg("DFatFS::recognize fatBytesPerSector not 512 but was "); debug_int(header->fatBytesPerSector); debug_msg("\n");
#endif
    //return false;
  }
  else goodHeader=true;
  /*
  if(header->fatSignature!=41) {
    debug_msg("DFatFS::recognize fatSignature not 41 but was "); debug_int(header->fatSignature); debug_msg("\n");
    //return false;
  }
  else goodHeader=true;
  */
  if(header->fatSignature==41) goodHeader=true;
  if(header->fatSignature==42) goodHeader=true;
#ifdef DEBUG_VERBOSE
  debug_msg("fatSignature was "); debug_int(header->fatSignature); debug_msg("\n");
#endif
#ifdef DEBUG_VERBOSE
  if(goodHeader) debug_msg("(goodHeader was set)\n");
 #endif
  //
  if(goodHeader) ret=true;
  delete header;
  header=NULL;
  //
  //if(fatHiddenSectors>1000) return false;
  //
  free(tbuf);
  return ret;
}


/* STATIC */
bool DFatFS::recognizeFileExtension(const char *fname)
{
  bool ret=false;
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DFatFS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,"img")) ret=true;
  if(!strcmp(ext,"dsk")) ret=true;
  if(!strcmp(ext,"dmg")) ret=true;
#ifdef DEBUG_VERBOSE
  if(ret) debug_msg("(recognizeFileExtension returning true)\n");
#endif
  return ret;
}


const char *DFatFS::guessSystem(const char *fname)
{
  return "msdos";
}


bool DFatFS::cacheFile(void *fp)
{
  debug_msg("DFatFS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


bool DFatFS::readDirectory()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatFS::readDirectory()...\n");
#endif
  freeDirCache();
  UINT t=0;
  for(t=0;t<256;t++) diskName[t]=0;
  strcpy(diskName,"NO NAME");
  //
  /*
  imageFormat=recognize(imageData,NULL,imageDataSize);
  switch(imageFormat) {
    case DFAT_FORMAT_RAW:
      break;
    case DFAT_FORMAT_MFM_DISK:
      debug_msg("Unimplemented DFAT_FORMAT_MFM_DISK, sorry!\n");
      break;
    default:
      debug_msg("Unimplemented format, sorry!\n");
      diskError=true;
      return;
      break;
  }
  */
  //
  readGeometry();
  //
  /*
  debug_msg("numTracks="); debug_msg(numTracks); debug_msg(" numSectors="); debug_msg(numSectors); debug_msg(" numSides="); debug_msg(numSides); debug_msg(" blockSize="); debug_msg(blockSize); debug_msg("\n");
  if(!numSectors) {
    debug_msg("numSectors is zero, we messed up someplace, bailing!\n");
    diskError=true;
    exit(EXIT_FAILURE);
  }
  */
  //
  if(!sanityCheck()) { debug_msg("DFatFS failed sanityCheck!!!\n"); return false;}
  //
  // NOTE: This is only valid for FAT12 and FAT16!!! (floppies only, right?)
  dirOffSector=header->fatReservedSectors+(header->fatTotalFats*header->fatSectorsPerFat);
  dirOffBytes=dirOffSector*blockSize;
  numDirSectors=(header->fatMaxRootEntries*32)/blockSize;
#ifdef DEBUG_VERBOSE
  debug_msg("dirOffSector is "); debug_int(dirOffSector); debug_msg(" sectors\n");
  debug_msg("dirOffBytes is "); debug_int(dirOffBytes); debug_msg(" bytes\n");
  debug_msg("fatMaxRootEntries is "); debug_int(header->fatMaxRootEntries); debug_msg("\n");
  debug_msg("blockSize is "); debug_int(blockSize); debug_msg(" bytes\n");
  debug_msg("numDirSectors is "); debug_int(numDirSectors); debug_msg(" sectors\n");
#endif
  if(!header->fatMaxRootEntries) { debug_msg("This is probably FAT32, not implemented yet!\n"); return false; }
  UINT blockOff=0;
  bool result=false;
  bool reading=true;
  t=0;
  while(reading) {
#ifdef DEBUG_VERBOSE
    debug_msg("readBlock("); debug_int(dirOffSector+blockOff); debug_msg(")...\n");
#endif
    bool rret=readBlock(dirOffSector+blockOff);
    if(!rret) { debug_msg("readBlock failed!\n"); return false; }
    if(!buf[0]) reading=false;
    else {
      for(UINT i=0;i<(blockSize/32);i++) {
        result=readDirEntry(i*32);
        if(!result) return true;
      }
    }
    blockOff++;
    t++;
    if(t==numDirSectors) reading=false;
  }
  removeSpacesFromFilenames();
  debug_msg("DFatFS::readDirectory done\n");
  return true;
}


bool DFatFS::readDirEntry(long where)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatFS::readDirEntry("); debug_long(where); debug_msg(")...\n");
#endif
    if(buf[where]==0xe5) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping deleted entry at "); debug_long(where); debug_msg("...\n");
#endif
      return true;
    }
    if(!buf[where]) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping empty entry at "); debug_long(where); debug_msg("...\n");
#endif
      return true;
    }
    //
    int nativeType=buf[where+13];
    int theSize=(buf[where+0x1f]<<24)+(buf[where+0x1e]<<16)+(buf[where+0x1d]<<8)+buf[where+0x1c];
    UINT entryCluster=(buf[where+0x1b]<<8)+buf[where+0x1a];
    ///
    if(!entryCluster) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping zero cluster at "); debug_long(where); debug_msg("...\n");
#endif
      return true;
    }
    if(!theSize) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping zero size at "); debug_long(where); debug_msg("...\n");
#endif
      return true;
    }
    /*
    if(!(nativeType&0x32)) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping non-normal at "); debug_long(where); debug_msg("...\n");
#endif
      return true;
    }
    */
    UINT t=0,off=0;
    DDirEntry *theFile=NULL;
    theFile=new DDirEntry;  // Only create file if non-zero name
    if(theFile) {
#ifdef DEBUG_VERBOSE
      debug_msg("Creating file entry...\n");
#endif
      theFile->signature=0xface;
      //debug_msg("off="); debug_int(off); debug_msg("\n");
      for(t=0;t<DDISK_MAX_NAMELEN;t++) theFile->name[t]=0;
      theFile->name[DDISK_MAX_NAMELEN-1]=0;
      // MAYBE skip spaces in name in line below?
      for(t=0;t<8;t++) { theFile->name[t]=buf[where+off]; off++; }
      //debug_msg("off="); debug_int(off); debug_msg("\n");
      theFile->name[8]='.';
      // Am I skipping spaces correctly below?
      for(t=0;t<3;t++) { if(buf[where+off]!=32) { theFile->name[t+9]=buf[where+off]; } off++; }
      //debug_msg("off="); debug_int(off); debug_msg("\n");
#ifdef DEBUG_VERBOSE
      debug_msg("name is '"); debug_msg(theFile->name); debug_msg("'\n");
#endif
      theFile->nativeType=buf[where+off]; off++;
      //debug_msg("off="); debug_int(off); debug_msg("\n");
#ifdef DEBUG_VERBOSE
      debug_msg("nativeType="); debug_int(theFile->nativeType); debug_msg("\n");
#endif
      theFile->type=DDISK_TYPE_BINARY; // TODO
      entryCluster=(buf[where+0x1b]<<8)+buf[where+0x1a]; off+=2;
      //debug_msg("off="); debug_int(off); debug_msg("\n");
#ifdef DEBUG_VERBOSE
      debug_msg("entryCluster is "); debug_int(entryCluster); debug_msg("\n");
#endif
      theFile->track=0xff; // TODO:
      theFile->sector=0xff; // TODO:
      theFile->data=NULL;
      theFile->size=theSize; off+=4;
      //debug_msg("off="); debug_int(off); debug_msg("\n");
      theFile->sizeBlocks=theFile->size/512;
#ifdef DEBUG_VERBOSE
      debug_msg("size is "); debug_int(theFile->size); debug_msg(" bytes\n");
      debug_msg("size is "); debug_int(theFile->sizeBlocks); debug_msg(" blocks\n");
#endif
      theFile->block=0;
      theFile->curSeek=0;
      theDir.append((void *)theFile);
      if(!theFile->checkDiskSanity(this)) return false;
    }
    //else { debug_msg("Couldn't alloc file entry!\n"); return false; }
  return true;
}


bool DFatFS::readGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatFS::readGeometry()...\n");
#endif
  if((maxTrack)&&(maxSector)&&(maxSide)&&(blockSize)) return true;
  bool ret=true;
  //
  blockSize=0;
  maxTrack=0;
  maxSector=0;
  maxSide=0;
  //
  bool rret=readBlock(0);
  if(!rret) { debug_msg("readBlock failed!\n"); return false; }
  unsigned int offset=0xb;
  header=new DFatFSHeader(imageData,offset);
  blockSize=header->fatBytesPerSector;
  header->debugDump();
  //
  switch(header->fatMediaDescriptor&0xff) {
    case 0xe5: maxSide=1; maxTrack=77; maxSector=26; blockSize=128; break;
    case 0xed: maxSide=2; maxTrack=80; maxSector=9; blockSize=128; break;
    //case 0xee: maxSide=1; maxTrack=77; maxSector=26; blockSize=128; break;
    //case 0xef: maxSide=1; maxTrack=77; maxSector=26; blockSize=128; break;
    case 0xf0: maxSide=2; maxTrack=80; maxSector=18; blockSize=512; break;  // 1.4m floppy
    //case 0xf4: maxSide=1; maxTrack=77; maxSector=26; blockSize=128; break;
    //case 0xf5: maxSide=1; maxTrack=77; maxSector=26; blockSize=128; break;
    //case 0xf8: maxSide=1; maxTrack=77; maxSector=26; blockSize=128; break;
    case 0xf9: maxSide=2; maxTrack=80; maxSector=9; blockSize=128; break;
    case 0xfa: maxSide=1; maxTrack=80; maxSector=8; blockSize=128; break;
    case 0xfb: maxSide=2; maxTrack=80; maxSector=8; blockSize=128; break;
    case 0xfc: maxSide=1; maxTrack=40; maxSector=9; blockSize=128; break;
    case 0xfd: maxSide=2; maxTrack=40; maxSector=9; blockSize=128; break;
    case 0xfe: maxSide=1; maxTrack=40; maxSector=8; blockSize=128; break;
    case 0xff: maxSide=2; maxTrack=40; maxSector=8; blockSize=128; break;
    default:
      debug_msg("Didn't recognize fatMediaDescriptor="); debug_hexl(header->fatMediaDescriptor); debug_nl();
      return false;
      break;
  }
  maxSide=header->fatNumSides;
  maxSector=header->fatNumSectors;
  blockSize=512;
  //
  return ret;
}


bool DFatFS::readBootBlock()
{
  debug_msg("DFatFS::readBootBlock()...\n");
  bool ret=false;
  bool rret=readBlock(0);
  if(!rret) { debug_msg("readBlock failed!\n"); return false; }
  // TODO!
  return ret;
}


bool DFatFS::detectSubFormat()
{
  if(!basicSanityCheck()) return false;
  // By default
  return false;
}


bool DFatFS::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatFS::sanityCheck()...\n");
#endif
  //if(!readGeometry()) { debug_msg("sanityCheck failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("sanityCheck failed!\n"); return false; }
  if(header->fatHiddenSectors) { debug_msg("sanityCheck failed fatHiddenSectors!\n"); return false; }
  if(!header->fatNumSectors) { debug_msg("sanityCheck failed fatNumSectors!\n"); return false; }
  //if(header->fatNumSectors>20) { debug_msg("sanityCheck failed fatNumSectors!\n"); return false; }
  if(!header->fatNumSides) { debug_msg("sanityCheck failed fatNumSides!\n"); return false; }
  //if(header->fatNumSides>2) { debug_msg("readGeometry failed fatNumSides!\n"); return false; }
  if(header->fatMaxRootEntries>1024) { debug_msg("sanityCheck failed fatMaxRootEntries>1024!\n"); return false; }
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//  DFatFSHeader Class
////////////////////////////////////////////////////////////////////////////////


DFatFSHeader::DFatFSHeader(unsigned char *b,UINT offset)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatFSHeader::DFatFSHeader()...\n");
#endif
  fatBytesPerSector=(b[offset+1]*256)+b[offset];  offset+=2;
  fatSectorsPerCluster=b[offset]; offset++;
  fatReservedSectors=(b[offset+1]*256)+b[offset];  offset+=2;
  fatTotalFats=b[offset]; offset++;
  fatMaxRootEntries=(b[offset+1]*256)+b[offset];  offset+=2;
  fatTotalSectors=(b[offset]*256)+b[offset];  offset+=2;
  fatMediaDescriptor=b[offset]; offset++;
  fatSectorsPerFat=(b[offset+1]*256)+b[offset];  offset+=2;
  fatNumSectors=(b[offset+1]*256)+b[offset];  offset+=2;
  fatNumSides=(b[offset+1]*256)+b[offset];  offset+=2;
  fatHiddenSectors=(b[offset+3]<<24)+(b[offset+2]<<16)+(b[offset+1]<<8)+b[offset+0];  offset+=4;
  fatTotalSectorsLarge=(b[offset+3]<<24)+(b[offset+2]<<16)+(b[offset+1]<<8)+b[offset+0];  offset+=4;
  fatDriveNumber=b[offset]; offset++;
  fatFlags=b[offset]; offset++;
  fatSignature=b[offset]; offset++;
}


void DFatFSHeader::debugDump()
{
#ifdef DEBUG_VERBOSE
  debug_msg("fatBytesPerSector="); debug_int(fatBytesPerSector); debug_msg("\n");
  debug_msg("fatSectorsPerCluster="); debug_int(fatSectorsPerCluster); debug_msg("\n");
  debug_msg("fatReservedSectors="); debug_int(fatReservedSectors); debug_msg("\n");
  debug_msg("fatTotalFats="); debug_int(fatTotalFats); debug_msg("\n");
  debug_msg("fatMaxRootEntries="); debug_int(fatMaxRootEntries); debug_msg("\n");
  debug_msg("fatTotalSectors="); debug_int(fatTotalSectors); debug_msg("\n");
  debug_msg("fatMediaDescriptor="); debug_hexl(fatMediaDescriptor); debug_msg("\n");
  debug_msg("fatSectorsPerFat="); debug_int(fatSectorsPerFat); debug_msg("\n");
  debug_msg("fatNumSectors="); debug_int(fatNumSectors); debug_msg("\n");
  debug_msg("fatNumSides="); debug_int(fatNumSides); debug_msg("\n");
  debug_msg("fatHiddenSectors="); debug_int(fatHiddenSectors); debug_msg("\n");
  debug_msg("fatTotalSectorsLarge="); debug_int(fatTotalSectorsLarge); debug_msg("\n");
  debug_msg("fatDriveNumber="); debug_int(fatDriveNumber); debug_msg("\n");
  debug_msg("fatFlags="); debug_int(fatFlags); debug_msg("\n");
  debug_msg("fatSignature="); debug_int(fatSignature); debug_msg("\n");
#endif
}


////////////////////////////////////////////////////////////////////////////////
//  DFatDirEntry Class
////////////////////////////////////////////////////////////////////////////////

DFatDirEntry::DFatDirEntry(unsigned char *b,UINT offset)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DFatDirEntry::DFatDirEntry()...\n");
#endif
}


void DFatDirEntry::debugDump()
{
#ifdef DEBUG_VERBOSE
#endif
}


////////////////////////////////////////////////////////////////////////////////
//  Docs/notes
////////////////////////////////////////////////////////////////////////////////

/*
NOTES:

0xeb 0x34 0x90 first three bytes?
0xeb 0x3c 0x90 first three bytes?

0x03-0x0a creator dos name?
0x2b-0x35 disk name?
0x36-0x3d fat name?

8=label
32=normal
e5 first of name = deleted

lfn
0       Bits 0-4: sequence number; bit 6: final part of name
1-10    Unicode characters 1-5
11      Attribute: 0xf
12      Type: 0
13      Checksum of short name
14-25   Unicode characters 6-11
26-27   Starting cluster: 0
28-31   Unicode characters 12-13

attributes:
0 read only
1 hidden
2 system
3 label
4 directory
5 archive

*/
