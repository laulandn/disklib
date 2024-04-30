
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
#include <stdlib.h>

#include <stdint.h>


#include "DCpmFS.h"

#include "DTrs80FS.h"
#include "DCbmFS.h"
#include "DAppleDos3FS.h"
#include "DAppleProDosFS.h"



////////////////////////////////////////////////////////////////////////////////
//  DCpmFS Class
////////////////////////////////////////////////////////////////////////////////

DCpmFS::DCpmFS(const char *fname) : DDiskImageMem(fname)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::DCpmFS("); debug_msg(fname); debug_msg("')\n");
#endif
  init();
  Mount(fname);
}


const char *DCpmFS::guessSystem(const char *fname)
{
  return "cpm";
}


void DCpmFS::init()
{
  noFileSystem=false;
  allTracksSameSectors=true;
  noSectorZero=true;
  // Start with original cp/m geometry, most likely will change as we decode the image...
  //numTracks=80; numSectors=26; numSides=1; blockSize=128;
  unpackedSize=0;
  unpackedData=NULL;
  totalSectors=0;
  skipOffset=0;
  cpcTrackSize=0;
  blockSize=512;  // by default?
  dirAlreadyRead=false;
  hardDisk=false;
}


DCpmFS::~DCpmFS()
{
  freeDirCache();
  diskMounted=false;
}


/* STATIC */
bool DCpmFS::recognize(const char *fname)
{
  UINT theFormat=DCPM_FORMAT_UNKNOWN;
  if(!recognizeFileExtension(fname)) return false;
  bool ret=true;
  FILE *f=fopen(fname,"r");
  if(!f) {
#ifdef DEBUG_VERBOSE
    debug_msg("DCpmFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN);
#endif
    return false;
  }
  size_t size=getSize(fname);
  bool sizeMaybe=false;
  if(size==737280) sizeMaybe=true;  // msx disk?
  //if(size==143360) sizeMaybe=true;  // Apple 2
  if(size==163840) sizeMaybe=true;  // IBM PC
  if(size==194816) sizeMaybe=true;  // Amstrad disk?
  if(size==192563) sizeMaybe=true;  // ???
  //if(size==193024) sizeMaybe=true;  // TRS-80
  if(size==327680) sizeMaybe=true;  // IBM PC
  if(size==368640) sizeMaybe=true;  // IBM PC
  if(size==409600) sizeMaybe=true;  // rainbow?
  if(size==1474979) sizeMaybe=true;  // ???
  if(size==1474560) sizeMaybe=true;  // ???
  if(size==256016) sizeMaybe=true;  // ???
  if(size==256256) sizeMaybe=true;  // ???
  //if(size==819200) sizeMaybe=true;  // CBM 1581
  //if(size==174848) sizeMaybe=true;  // CBM 1541
  //if(size==349696) sizeMaybe=true;  // CBM 1571
  if(size==286720) sizeMaybe=true;  // CPM-8000?
  //
  char *tbuf=(char *)malloc(size);
  if(!tbuf) { /*::fclose(f);*/ debug_msg("DCpmFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret; }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  if(f) ::fclose(f);
  //
  if(size==143360) {
    sizeMaybe=true;  ret=true;
    debug_msg("DCpmFS: Looks like apple2 disk...will check specifically cpm...\n");
    if(DAppleDos3FS::recognize(fname)) {
      debug_msg("DAppleDos3FS::recognize said true...NOT cpm!\n");
      sizeMaybe=false; ret=false;
    }
    if(DAppleProDosFS::recognize(fname)) {
      debug_msg("DAppleProDosFS::recognize said true...NOT cpm!\n");
      sizeMaybe=false; ret=false;
    }
    if(DApplePascalFS::recognize(fname)) {
      debug_msg("DApplePascalFS::recognize said true...NOT cpm!\n");
      sizeMaybe=false; ret=false;
    }
  }
  if(size==193024) {
    debug_msg("Looks like trs80 disk...will check specifically cpm...\n");
    if(!DTrs80FS::recognize(fname)) {
      debug_msg("DTrs80FS::recognize said false!\n");
      sizeMaybe=true; ret=true;
    }
    else {
      debug_msg("DTrs80FS::recognize said true...NOT cpm!\n");
      sizeMaybe=false; ret=false;
    }
  }
  if(!sizeMaybe) {
#ifdef DEBUG_VERBOSE
    //debug_msg("disk image size is "); debug_int(size); debug_msg(" which doesn't look right for a cpm disk...\n");
#endif
    ret=false;
  }
  //
  theFormat=DCPM_FORMAT_RAW;
  //
  /*
    TODO: Check for cbm dos directory structure here?
  */
  //
  char *tstring=NULL;
  tstring=(char *)"EXTENDED CPC DSK File";
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) { ret=true; theFormat=DCPM_FORMAT_EXTDSK; }
  tstring=(char *)"MV - CPCEMU";
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) { ret=true; theFormat=DCPM_FORMAT_CPCEMU; }
  tstring=(char *)"IMD ";
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) { ret=true; theFormat=DCPM_FORMAT_IMD; }
  tstring=(char *)"63k CP/M 2.2b";  //hard disk image...I think...
  if(!strncmp((char *)tbuf,tstring,strlen(tstring))) { ret=true; theFormat=DCPM_FORMAT_RAW; }
  tstring=(char *)"multifmt";  // This is for the "RomWBW demodisk.dat" hard disk image
  if(!strncmp((char *)tbuf+0x584,tstring,strlen(tstring))) {
    tstring=(char *)"CDRIVE";
    if(!strncmp((char *)tbuf+0x5e7,tstring,strlen(tstring))) {
      ret=true; theFormat=DCPM_FORMAT_RAW;
    }
  }
  //
  free(tbuf);
  //ret=true;
  return ret;
}


/* STATIC */
bool DCpmFS::recognizeFileExtension(const char *fname)
{
  //debug_msg("DCpmFS::recognizeFileExtension('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_NO_IMPL);
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DCpmFS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,"dsk")) return true;
  if(!strcmp(ext,"imd")) return true;
  if(!strcmp(ext,"IMD")) return true;
  // actually anything is ok for now...
  return true;
}


bool DCpmFS::readDirectory()
{
  if(dirAlreadyRead) return true;
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::readDirectory...\n");
#endif
  freeDirCache();
  //
  dirAlreadyRead=true;
  // defaults for now...
  dirTrack=3;
  dirStartSect=1;
  dirEndSect=16;
  dirEntrySize=32;
#ifdef DEBUG_VERBOSE
  debug_msg("dir is at "); debug_int(dirTrack); debug_msg(" from ");
  debug_int(dirStartSect); debug_msg(" to "); debug_int(dirEndSect);
  debug_msg(" entry size is "); debug_int(dirEntrySize); debug_msg("\n");
#endif
  //
  subFormat=detectSubFormat();
  UINT t=0;
  for(t=0;t<256;t++) diskName[t]=0;
  strcpy(diskName,"NO NAME");
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
  }
  */
  //
  // NOTE: We should do some sanity checking on the directory entries, etc...
  // Now read directory entries
  // TODO: Should we read less sectors if their size is larger than 128 bytes?!?
  bool result=false;
  for(t=dirStartSect;t<dirEndSect;t++) {
    readTrackSector(dirTrack,t);
#ifdef DEBUG_VERBOSE
    dumpBufHex((uint8_t *)buf,blockSize);
#endif
    for(UINT i=0;i<(blockSize/dirEntrySize);i++) {
      result=readDirEntry(i*dirEntrySize);
      //if(!result) return true;
    }
  }
  removeSpacesFromFilenames();
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::readDirectory done\n");
#endif
  return true;
}


bool DCpmFS::readCpcEmuHeader()
{
  debug_msg("DCpmFS::readCpcEmuHeader()...\n");
  blockSize=128;
  maxTrack=imageData[0x30];
  maxSide=imageData[0x31];
  numSectors=(UINT *)malloc(maxTrack*maxSide*(sizeof(UINT *)));
  debug_msg("maxTrack="); debug_int(maxTrack); debug_msg(" maxSides="); debug_int(maxSide); debug_msg("\n");
  cpcTrackSize=(imageData[0x33]*256)+imageData[0x32];
  if(!cpcTrackSize) {
    debug_msg("NOTE: Extended format with separate track sizes...\n");
    for(UINT i=0;i<maxTrack*maxSide;i++) {
      //numSectors[i]=(UINT)(cpcTrackSize/blockSize);
      numSectors[i]=imageData[0x34+i];
      debug_msg("Track #"); debug_int(i); debug_msg(" has ");
      debug_int(numSectors[i]); debug_msg(" sectors...\n");
      totalSectors+=numSectors[i];
    }
  }
  else {
    debug_msg("cpcTrackSize="); debug_long(cpcTrackSize); debug_msg("\n");
    if(blockSize) totalSectors=(UINT)(maxTrack*cpcTrackSize)/blockSize;
  }
  if(!totalSectors) { debug_msg("zero totalSectors!\n"); return false; }
  return true;
}


bool DCpmFS::readExtDskHeader()
{
  debug_msg("DCpmFS::readExtDskHeader()...\n");
  blockSize=128;
  maxTrack=imageData[0x30];
  maxSide=imageData[0x31];
  numSectors=(UINT *)malloc(maxTrack*(sizeof(UINT *)));
  debug_msg("maxTrack="); debug_int(maxTrack); debug_msg(" numSizes="); debug_int(maxSide); debug_msg("\n");
  trackDataSize=(size_t *)malloc(sizeof(UINT)*maxTrack);
  // TODO: This is the coded size with all the headers, right?  So totalSectors is WRONG.
  for(UINT t=0;t<maxTrack;t++) {
    trackDataSize[t]=imageData[0x34+t]*256;
    debug_msg("trackDataSize("); debug_int(t); debug_msg(")="); debug_long(trackDataSize[t]); debug_msg(" bytes\n");
    totalSectors+=trackDataSize[t]/blockSize;  // TODO: probably not right
    numSectors[t]=(UINT)trackDataSize[t];
  }
  debug_msg("totalSectors="); debug_int(totalSectors); debug_msg(" (probably completely wrong)\n");
  return false;
}


bool DCpmFS::cacheFile(void *fp)
{
  debug_msg("DCpmFS::cacheFile "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  return false;
}


// TODO: What if there's no block 0?!?
// NOTE: We have our own version because we may have unpackedData a weird format
bool DCpmFS::readBlock(UINT blk)
{
  diskError=true;
  if(!unpackedData) { debug_msg("No unpackedData "); debug_msg(DDISK_ERR_BAD_BUFFER); return false; }
  curLoc=blk*blockSize+skipOffset;
  if(curLoc>unpackedSize) { debug_msg("Past end of unpackedData!\n"); return false; }
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::readBlock("); debug_int(blk); debug_msg(") at offset "); debug_long(curLoc); debug_msg("\n");
  //debug_msg((int)0); debug_msg(": ";
#endif
  for(UINT t=0;t<blockSize;t++) {
    buf[t]=unpackedData[curLoc+t];
    //debug_msg((int)buf[t]); debug_msg(",";
    //if(t&&(!(t&0xf))) debug_msg("\n"); debug_msg((int)t); debug_msg(": ";
  }
  //debug_msg("\n");
  diskError=false;
  return true;
}


bool DCpmFS::writeBlock(UINT blk)
{
  debug_msg("DCpmFS::writeBlock "); debug_msg(DDISK_ERR_NO_IMPL);
  diskError=true;
  diskStatus=DDISK_STATUS_WRITE_ERROR;
  return false;
}


bool DCpmFS::unpackCpcEmu()
{
  debug_msg("DCpmFS::unpackCpcEmu()...\n");
  unpackedSize=totalSectors*blockSize;
  unpackedData=(unsigned char *)calloc(totalSectors,blockSize);
  offset=0x100; // offset to first track
  dOffset=0;  // Start writing at 0
  skippedLast=0;  // reset skip counter
  for(UINT t=0;t<maxTrack;t++) {
    unpackCpcEmuTrack(t);
  }
  return false;
}


bool DCpmFS::unpackExtDsk()
{
  debug_msg("DCpmFS::unpackExtDsk()...\n");
  unpackedSize=totalSectors*blockSize;
  unpackedData=(unsigned char *)calloc(totalSectors,blockSize);
  offset=0x100; // offset to first track
  dOffset=0;  // Start writing at 0
  skippedLast=0;  // reset skip counter
  for(UINT t=0;t<maxTrack;t++) {
    if(trackDataSize[t]) {
      unpackCpcEmuTrack(t);
    }
    else {
      debug_msg("Skipping track "); debug_int(t); debug_msg("...\n");
      skippedLast++;
    }
  }
  return false;
}


// God only knows if this works at all...
bool DCpmFS::unpackCpcEmuTrack(UINT t)
{
  debug_msg("DCpmFS::unpackCpcEmuTrack("); debug_int(t); debug_msg(")...\n");
  UINT tNum,sNum,sSize,nSectors,gapLen,fillerByte;
  if(strncmp("Track-Info",(char *)&imageData[offset],10)) {
    debug_msg("Something went wrong with Track-Info, bailing!\n");
    diskStatus=DDISK_STATUS_BAD_FORMAT;
    diskError=true;
    //exit(EXIT_FAILURE);
    err.setError();
    return false;
  }
  else {
    tNum=imageData[offset+0x10];
    sNum=imageData[offset+0x11];
    if(sNum) {
      debug_msg("Only single sided disks "); debug_msg(DDISK_ERR_BAD_SIDE);
      diskError=true;
      //exit(EXIT_FAILURE);
      err.setError();
      return false;
    }
    sSize=imageData[offset+0x14];
    nSectors=imageData[offset+0x15];
    // TODO: Fill out track table from this like 1541 does
    gapLen=imageData[offset+0x16];
    fillerByte=imageData[offset+0x17];
    debug_msg("offset="); debug_long(offset); debug_msg(" "); debug_int(t); debug_msg(": tNum="); debug_int(tNum);
    debug_msg(" sNum="); debug_int(sNum); debug_msg(" sSize="); debug_int(sSize); debug_msg(" nSectors="); debug_int(nSectors);
    debug_msg(" gapLen="); debug_int(gapLen); debug_msg(" fillerByte="); debug_int(fillerByte); debug_msg("\n");
    if(tNum!=t) {
      debug_msg(DDISK_ERR_BAD_TRACK);
      //exit(EXIT_FAILURE);
      err.setError();
      return false;
    }
    ULONG sOffset=offset;
    UINT n765_c=imageData[sOffset+0x18];
    UINT n765_h=imageData[sOffset+0x19];
    UINT n765_r=imageData[sOffset+0x1a];
    UINT n765_n=imageData[sOffset+0x1b];
    UINT n765_1=imageData[sOffset+0x1c];
    UINT n765_2=imageData[sOffset+0x1d];
    UINT actualLen=(UINT)(cpcTrackSize-0x100)/nSectors;
    blockSize=actualLen;
    // TODO: We assume all sectors are the same!
    //numSectors=nSectors;
    // TODO: We assume all tracks have same number of sectors!
    if(skippedLast) {
      UINT sVal=(blockSize*numSectors[t])*skippedLast;
      debug_msg("(Skipped last "); debug_int(skippedLast); debug_msg(" tracks, so advancing dOffset by "); debug_int(sVal); debug_msg(" bytes)\n");
      skippedLast=0;
      dOffset+=sVal;
    }
    debug_msg("n765_c="); debug_int(n765_c); debug_msg(" n765_h="); debug_int(n765_h); debug_msg(" n765_r="); debug_int(n765_r);
    debug_msg(" n765_n="); debug_int(n765_n); debug_msg(" n765_1="); debug_int(n765_1); debug_msg(" n765_2="); debug_int(n765_2);
    debug_msg(" actualLen="); debug_int(actualLen); debug_msg("\n");
    for(UINT i=0;i<nSectors;i++) {
      debug_int(t); debug_msg(","); debug_int(i); debug_msg(": "); debug_long(dOffset); debug_msg(" "); debug_long(sOffset); debug_msg("\n");
      memcpy(unpackedData+dOffset,imageData+sOffset+0x20,actualLen);
      sOffset+=actualLen;
      dOffset+=actualLen;
    }
  }
  offset+=cpcTrackSize;
  return true;
}


// God only knows if this works at all...
bool DCpmFS::unpackExtDskTrack(UINT t)
{
  debug_msg("DCpmFS::unpackExtDskTrack("); debug_int(t); debug_msg(")...\n");
  UINT tNum,sNum,sSize,nSectors,gapLen,fillerByte;
  if(strncmp("Track-Info",(char *)&imageData[offset],10)) {
    debug_msg("Something went wrong with Track-Info, bailing!\n");
    diskError=true;
    //exit(EXIT_FAILURE);
    err.setError();
    diskStatus=DDISK_STATUS_BAD_FORMAT;
    return false;
  }
  else {
    tNum=imageData[offset+0x10];
    sNum=imageData[offset+0x11];
    if(sNum) {
      debug_msg("Only single sided disks "); debug_msg(DDISK_ERR_BAD_SIDE);
      diskError=true;
      //exit(EXIT_FAILURE);
      err.setError();
      return false;
    }
    sSize=imageData[offset+0x14];
    nSectors=imageData[offset+0x15];
    // TODO: Fill out track table from this like 1541 does
    gapLen=imageData[offset+0x16];
    fillerByte=imageData[offset+0x17];
    debug_msg("offset="); debug_long(offset); debug_msg(" "); debug_int(t); debug_msg(": tNum="); debug_int(tNum); debug_msg(" sNum="); debug_int(sNum);
    debug_msg(" sSize="); debug_int(sSize); debug_msg(" nSectors="); debug_int(nSectors); debug_msg(" gapLen="); debug_int(gapLen);
    debug_msg(" fillerByte="); debug_int(fillerByte); debug_msg("\n");
    if(tNum!=t) {
      debug_msg(DDISK_ERR_BAD_TRACK);
      //exit(EXIT_FAILURE);
    }
    ULONG sOffset=offset;
    UINT n765_c=imageData[sOffset+0x18];
    UINT n765_h=imageData[sOffset+0x19];
    UINT n765_r=imageData[sOffset+0x1a];
    UINT n765_n=imageData[sOffset+0x1b];
    UINT n765_1=imageData[sOffset+0x1c];
    UINT n765_2=imageData[sOffset+0x1d];
    UINT actualLen=(imageData[sOffset+0x1f]*256)+imageData[sOffset+0x1e];
    blockSize=actualLen;
    // TODO: We assume all sectors are the same!
    //numSectors=nSectors;
    // TODO: We assume all tracks have same number of sectors!
    if(skippedLast) {
      UINT sVal=(blockSize*numSectors[t])*skippedLast;
      debug_msg("(Skipped last "); debug_int(skippedLast); debug_msg(" tracks, so advancing dOffset by "); debug_int(sVal); debug_msg(" bytes)\n");
      skippedLast=0;
      dOffset+=sVal;
    }
    debug_msg("n765_c="); debug_int(n765_c); debug_msg(" n765_h="); debug_int(n765_h); debug_msg(" n765_r="); debug_int(n765_r);
    debug_msg(" n765_n="); debug_int(n765_n); debug_msg(" n765_1="); debug_int(n765_1); debug_msg(" n765_2="); debug_int(n765_2);
    debug_msg(" actualLen="); debug_int(actualLen); debug_msg("\n");
    for(UINT i=0;i<nSectors;i++) {
      debug_int(t); debug_msg(","); debug_int(i); debug_msg(": "); debug_long(dOffset); debug_msg(" "); debug_long(sOffset); debug_msg("\n");
      memcpy(unpackedData+dOffset,imageData+sOffset+0x20,actualLen);
      sOffset+=actualLen;
      dOffset+=actualLen;
    }
  }
  offset+=trackDataSize[t];
  return true;
}


bool DCpmFS::readDirEntry(ULONG off)
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::readDirEntry("); debug_long(off); debug_msg(")...\n");
#endif
  if(true) {
    UINT t=0;
    if(!buf[off+1]) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping zero'd out entry...\n");
#endif
      return false;
    }
    if(buf[off+1]==0xe5) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping 0xe5 entry...\n");
#endif
      return false;
    }
    if(buf[off+1]==0xff) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping 0xff entry...\n");
#endif
      return false;
    }
    if(buf[off]!=0) {
#ifdef DEBUG_VERBOSE
      debug_msg("Skipping non-zero file type entry...\n");
#endif
      return false;
    }
    UINT sizeBlocks=buf[off+15];
    UINT numRec=buf[off+18];
    UINT allocation=buf[off+19];
#ifdef DEBUG_VERBOSE
    debug_msg("sizeBlocks/numRec/allocation "); debug_int(sizeBlocks); debug_msg(" "); debug_int(numRec); debug_msg(" "); debug_int(allocation);  debug_msg("\n");
#endif
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<DDISK_MAX_NAMELEN;t++) theFile->name[t]=0;
      theFile->name[DDISK_MAX_NAMELEN-1]=0;
      for(t=1;t<9;t++) {
        theFile->name[t-1]=buf[off+t];
#ifdef DEBUG_VERBOSE
        debug_hex2(buf[off+t]); debug_msg(" ");
#endif
      }
#ifdef DEBUG_VERBOSE
      debug_nl();

#endif      // TODO: Should these be "8" and "t-1" sometimes?
      theFile->name[8]='.';
      for(t=9;t<12;t++) {
        theFile->name[t]=(buf[off+t]&0x7f);
#ifdef DEBUG_VERBOSE
        debug_hex2(buf[off+t]); debug_msg(" ");
#endif
      }
#ifdef DEBUG_VERBOSE
      debug_nl();
      debug_msg("name is '"); debug_msg(theFile->name); debug_msg("'\n");
#endif
      //unsigned extentCount=buf[off+]
      theFile->sizeBlocks=sizeBlocks;
#ifdef DEBUG_VERBOSE
      debug_msg("size is "); debug_long(theFile->sizeBlocks); debug_msg(" blocks\n");
#endif
      //theFile->nativeType=buf[0];
      //debug_msg("nativeType="); debug_int(theFile->nativeType); debug_msg("\n");
      theFile->type=DDISK_TYPE_BINARY; // TODO
      theFile->track=0xff; // TODO:
      theFile->sector=0xff; // TODO:
      theFile->data=NULL;
      theFile->size=theFile->sizeBlocks*blockSize;
      theFile->block=0;
      theFile->curSeek=0;
      theDir.append((void *)theFile);
    }
    else { debug_msg("DCpmFS "); debug_msg(DDISK_ERR_NO_FILEENTRY); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  return true;
}


bool DCpmFS::detectSubFormat()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::detectSubFormat()...\n");
#endif
  subFormat=DCPM_FORMAT_UNKNOWN;
  if(!unpackedSize) unpackedSize=imageDataSize; // Lie for now...
  if(unpackedSize==737280) { /*debug_msg("(probably a raw 720k disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  //if(unpackedSize==143360) { /*debug_msg("(probably a apple 2 disk)\n");*/ subFormat=DCPM_FORMAT_APPLE2; }
  if(unpackedSize==163840) { /*debug_msg("(probably a raw 160k disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==194816) { /*debug_msg("(probably a cpc disk)\n");*/ subFormat=DCPM_FORMAT_CPCEMU; }
  if(unpackedSize==193024) { /*debug_msg("(probably a trs-80 disk)\n");*/ subFormat=DCPM_FORMAT_TRS80; }
  if(unpackedSize==192563) { /*debug_msg("(probably 180k cpm disk?)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==327680) { /*debug_msg("(probably a raw 320k disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==368640) { /*debug_msg("(probably a raw 360k disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==409600) { /*debug_msg("(probably a rainbow 400k disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==1474979) { /*debug_msg("(probably cpm disk?)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==1474560) { /*debug_msg("(probably cpm disk?)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==256256) { /*debug_msg("(probably a raw disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==256016) { /*debug_msg("(probably a raw disk)\n");*/ subFormat=DCPM_FORMAT_RAW; }
  if(unpackedSize==174848) { /*debug_msg("(probably a 1541 cpm disk)\n");*/ subFormat=DCPM_FORMAT_CBM; }
  if(unpackedSize==819200) { /*debug_msg("(probably a 1581 cpm disk)\n");*/ subFormat=DCPM_FORMAT_CBM; }
  if(unpackedSize==349696) { /*debug_msg("(probably a 1571 cpm disk)\n");*/ subFormat=DCPM_FORMAT_CBM; }
  if(unpackedSize==286720) { /*debug_msg("(probably a cpm-8000 disk)\n");*/ subFormat=DCPM_FORMAT_CPM8000; }
  //
  char *tstring=NULL;
  tstring=(char *)"IMD ";
  if(!strncmp((char *)imageData,tstring,strlen(tstring))) { subFormat=DCPM_FORMAT_IMD; }
  tstring=(char *)"EXTENDED CPC DSK File";
  if(!strncmp((char *)imageData,tstring,strlen(tstring))) { subFormat=DCPM_FORMAT_EXTDSK; }
  tstring=(char *)"MV - CPCEMU";
  if(!strncmp((char *)imageData,tstring,strlen(tstring))) { subFormat=DCPM_FORMAT_CPCEMU; }
  tstring=(char *)"63k CP/M 2.2b";  //hard disk image...I think...
  if(!strncmp((char *)imageData,tstring,strlen(tstring))) {
    subFormat=DCPM_FORMAT_RAW; hardDisk=true; skipOffset=256;
  }
  tstring=(char *)"multifmt";  // This is for the "RomWBW demodisk.dat" hard disk image
  if(!strncmp((char *)imageData+0x584,tstring,strlen(tstring))) {
    tstring=(char *)"CDRIVE";
    if(!strncmp((char *)imageData+0x5e7,tstring,strlen(tstring))) {
      subFormat=DCPM_FORMAT_RAW; hardDisk=true; skipOffset=1024;
    }
  }
  //
  if(subFormat==DCPM_FORMAT_UNKNOWN) {
    debug_msg("(Could't detect subformat for unpackedSize="); debug_long(unpackedSize); debug_msg(")!)\n");
    return false;
  }
  return true;
}


bool DCpmFS::readGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::readGeometry()...\n");
#endif
  bool ret=false;
  blockSize=0;
  maxTrack=0;
  maxSector=0;
  maxSide=0;
  //
  detectSubFormat();
  //
  unpackedData=imageData;
  unpackedSize=imageDataSize;
  switch(subFormat) {
    case DCPM_FORMAT_RAW:
      ret=guessGeometry();
      break;
    case DCPM_FORMAT_IMD:
      debug_msg("readGeometry IMD format not implemented!!!\n");
      ret=false;
      break;
    case DCPM_FORMAT_EXTDSK:
      ret=readExtDskHeader();
      if(ret) unpackExtDsk();
      else debug_msg("readExtDskHeader failed!\n");
      break;
     case DCPM_FORMAT_CPCEMU:
      ret=readCpcEmuHeader();
      if(ret) unpackCpcEmu();
      else debug_msg("readCpcEmuHeader failed!\n");
      break;
    default:
      ret=guessGeometry();
      break;
  }
  if(!ret) {
    debug_msg("DCpmFS::readGeometry() guessGeometry failed for subFormat=="); debug_int(subFormat); debug_nl();
    diskError=true;
    diskStatus=DDISK_STATUS_BAD_FORMAT;
  }
  //
  // May not be needed?
  deinterleave();  // Doesn't belong here, but works for now...
  //
  return ret;
}


// NOTE: cpm uses logical sector size of 128
// This will need to be converted at bios level and sector numbers read from disk!!!
bool DCpmFS::guessGeometry()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::guessGeometry()...\n");
#endif
  bool ret=false;
  //
  if(subFormat==DCPM_FORMAT_IMD) {
    debug_msg("guessGeometry IMD format not implemented!!!\n");
    return false;
  }
  //
  // NOTE: I may have some of these totally wrong...
  switch(imageDataSize) {
    case 143360:
#ifdef DEBUG_VERBOSE
      debug_msg("geo apple2 140k\n");
#endif
      // wrong?
      maxTrack=35; maxSector=16; maxSide=2; blockSize=256;
      ret=true;
      break;
    case 163840:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 160k\n");
#endif
      maxTrack=40; maxSector=8; maxSide=1; blockSize=256;
      dirTrack=2;  dirStartSect=1;  dirEndSect=7;
      ret=true;
      break;
    case 327680:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 320k\n");
#endif
      maxTrack=40; maxSector=16; maxSide=2; blockSize=256;
      dirTrack=2;  dirStartSect=0;  dirEndSect=15;
      ret=true;
      break;
    case 368640:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 360k\n");
#endif
      maxTrack=40; maxSector=18; maxSide=2; blockSize=256;
      dirTrack=2;  dirStartSect=0;  dirEndSect=17;
      ret=true;
      break;
    case 194816:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 190k?\n");
#endif
      //  wrong
      maxTrack=40; maxSector=9; maxSide=2; blockSize=256;
      ret=true;
      break;
    case 192563:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 188k?\n");
#endif
      //  wrong
      maxTrack=40; maxSector=9; maxSide=2; blockSize=256;
      ret=true;
      break;
    case 193024:
#ifdef DEBUG_VERBOSE
      debug_msg("geo trs-80 188k\n");
#endif
      //  wrong?
      // I think this has a 16 byte header...
      maxTrack=40; maxSector=9; maxSide=2; blockSize=256;
      dirTrack=8;  dirStartSect=1;  dirEndSect=8;
      ret=true;
      break;
     case 409600:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 400k\n");
#endif
      //  wrong?
      maxTrack=80; maxSector=10; maxSide=2; blockSize=256;
      dirTrack=4;  dirStartSect=1;  dirEndSect=9;
      ret=true;
     break;
   case 737280:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 720k\n");
#endif
      //  Directory shows up at 2*128*18, which is 4608?
      maxTrack=80; maxSector=18; maxSide=2; blockSize=256;
      //dirTrack=4;  dirStartSect=0;  dirEndSect=17;
      ret=true;
      break;
    case 1474979:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 1.44m?\n");
#endif
     //  wrong
      maxTrack=80; maxSector=36; maxSide=2; blockSize=256;
      dirTrack=4;  dirStartSect=0;  dirEndSect=35;
      ret=true;
      break;
    case 1474560:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 1.44m\n");
#endif
     maxTrack=80; maxSector=36; maxSide=2; blockSize=256;
      dirTrack=4;  dirStartSect=0;  dirEndSect=35;
      ret=true;
      break;
    case 174848:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 1541 170k\n");
#endif
      // wrong! tracks have different sector number!!!
      maxTrack=35; maxSector=20; maxSide=1; blockSize=256;
      dirTrack=0;  dirStartSect=10;  dirEndSect=19;
      ret=true;
      break;
    case 349696:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 1571 340k\n");
#endif
      // wrong! tracks have different sector number!!!
      maxTrack=35; maxSector=20; maxSide=2; blockSize=256;
      dirTrack=0;  dirStartSect=4;  dirEndSect=19;
      ret=true;
      break;
    case 819200:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 1581 800k\n");
#endif
      maxTrack=80; maxSector=20; maxSide=2; blockSize=256;
      dirTrack=0;  dirStartSect=0;  dirEndSect=19;
      ret=true;
      break;
    case 256256:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 256k?\n");
#endif
      // wrong
      maxTrack=80; maxSector=20; maxSide=1; blockSize=256;
      dirTrack=1;  dirStartSect=7;  dirEndSect=19;
      ret=true;
      break;
    case 256016:
#ifdef DEBUG_VERBOSE
      debug_msg("geo 256k?\n");
#endif
      // wrong
      maxTrack=80; maxSector=20; maxSide=1; blockSize=256;
      dirTrack=1;  dirStartSect=7;  dirEndSect=19;
      ret=true;
      break;
    case 286720:
#ifdef DEBUG_VERBOSE
      debug_msg("geo cpm-8000 280k?\n");
#endif
      //  wrong?
      maxTrack=40; maxSector=24; maxSide=1; blockSize=256;
      dirTrack=2;  dirStartSect=1;  dirEndSect=23;
      ret=true;
      break;
    default:
      debug_msg("DCpmFS::guessGeometry() unusual imageDataSize=="); debug_long(imageDataSize); debug_nl();
      break;
  }
  //
  if(!ret) {
    // If we get here, it is an odd size...so we just make something up...
    maxSector=20; maxSide=1; blockSize=256;
    maxTrack=imageDataSize/256/maxSector;
    dirTrack=2;  dirStartSect=7;  dirEndSect=20;
    if(hardDisk) {
      // We might be able to read some geometry from a header here?
    }
    ret=true;
  }
  //
  if(ret) {
#ifdef DEBUG_VERBOSE
    debug_msg("maxTrack=");  debug_int(maxTrack); debug_msg(" ");
    debug_msg("maxSector=");  debug_int(maxSector); debug_msg(" ");
    debug_msg("maxSide=");  debug_int(maxSide); debug_msg("\n");
    debug_msg("dirTrack=");  debug_int(dirTrack); debug_msg(" ");
    debug_msg("dirStartSect=");  debug_int(dirStartSect); debug_msg(" ");
    debug_msg("dirEndSect=");  debug_int(dirEndSect); debug_msg("\n");
#endif
  }
  return ret;
}


bool DCpmFS::readBootBlock()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::readBootBlock()...\n");
#endif
  bool ret=false;
  readBlock(0);
  return ret;
}


bool DCpmFS::deinterleave()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::deinterleave()...\n");
#endif
  bool ret=false;
  //debug_msg("deinterleave is not implemented yet!!!\n");
  return ret;
}


bool DCpmFS::verifyApple2()
{
  debug_msg("DCpmFS::verifyApple2()...\n");
  bool ret=false;
  debug_msg("verifyApple2 is not implemented!!!\n");
  return ret;
}


bool DCpmFS::sanityCheck()
{
#ifdef DEBUG_VERBOSE
  debug_msg("DCpmFS::sanityCheck()...\n");
#endif
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  // By default
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//  Docs
////////////////////////////////////////////////////////////////////////////////

/*
NOTES:

Originally only IBM 3740 8" floppy
single sided, single density
~250k capacity

"Technically, it was 77 tracks, 26 sectors per track, 128 byte sectors. Disks were single-sided, single-density (SSSD)."

80 tracks, 26 sectors, 1 side, 128 byte blocks
tracks start at 0
sectors start at 1

64 files per disk max
32 bytes per entry (FCB)

first 2.5 tracks "system area" (starting with 0)
OS is first 2 tracks (including boot block)
directory is 1/2 of track 2

boot block is track 0 sector 1

FCB Format:
0: BF busy/free (0=in use, 0xe5=free)
1-8: FN filename
9-11: FT file type
12: EX Extent (normally 0 except for really large files)
13-14: not used
15: RC record count # of recs for this extent
16-31: DM disk allocation map, shows clusters used by file

If file is under 16k in size, then only one FCB/extent is needed.

There are 241 clusters per disk (starting with 1)
First cluster is track 2 sector 17

cluster pattern is:
(starting after dir on track 2)
(skip 16),1-8, 2-2
2-6,3-8,4-8,5-4
5-4,6-8,7-8,8-6
8-2,9-8,10-8,11-8
12-8,13-8,(skip 10)
(repeat)

In memory "Allocation Vector" keeps track of clusters
(241 bytes, each bit is 1 if sector is used)

sector skew is:
1,14,10,23,6,19,2,15,11,24,7,20,3,16,12,25,8,21,4,17,13,26,9,22,5,18

"Drives seemed to come in 40 track versions, 35 track versions or 38 track versions."

"40 tracks, 18 sectors per track, 128 byte sectors, 3 system tracks for CP/M, 8 sectors (1 block) of directory entries, a skew factor of 5."

NOTE: Some really odd formats have track 0 complete different since the bios needs to read it

"CP/M 1.4 was designed to work with 8" 250k discs. Thus a CP/M 1.4 disc will be laid out in the following way:

 77 tracks in total;
 26 128-byte sectors per track, software skewed;
  2 reserved tracks;
  2 1k directory blocks, giving 64 directory entries;
240 1k data blocks, numbered 2-241."

"CPC system
This simple system is used by CPC computers:

If the first physical sector is 41h, the disc is in System format, ie:

single sided, single track, 40 tracks, 9 sectors/track, 512-byte sectors, 2 reserved tracks, 1k blocks, 2 directory blocks, gap lengths 2Ah and 52h, bootable.

If the first physical sector is C1h, the disc is in Data format, ie:

single sided, single track, 40 tracks, 9 sectors/track, 512-byte sectors, no reserved tracks, 1k blocks, 2 directory blocks, gap lengths 2Ah and 52h, not bootable."

"PCW/Spectrum system
In addition to the above system, the PCW and Spectrum +3 can determine the format of a disc from a 16-byte record on track 0, head 0, physical sector 1:

	DEFB	format number	;0 => SS SD, 3 => DS DD. Other values:
				;bad format.
				;1 and 2 are for the CPC formats, but those
				;formats don't have boot records anyway.
	DEFB	sidedness	;As in XDPB
	DEFB	tracks/side
	DEFB	sectors/track
	DEFB	physical sector shift	;psh in XDPB
	DEFB	no. reserved tracks	;off in XDPB
	DEFB	block shift		;bsh in XDPB
	DEFB	no. directory blocks
	DEFB	read/write gap length
	DEFB	format gap length
	DEFB	0,0,0,0,0		;Unused
	DEFB	checksum fiddle byte	;Used to indicate bootable
			;discs. Change this byte so that the 8-bit
			;checksum of the sector is:
                 	;  1 - sector contains a PCW9512 bootstrap
                 	;  3 - sector contains a Spectrum +3 bootstrap
               		;255 - sector contains a PCW8256 bootstrap
            		;(the bootstrap code is in the remainder of the sector)

If all bytes of the spec are 0E5h, it should be assumed that the disc is a 173k PCW/Spectrum +3 disc, ie:

single sided, single track, 40 tracks, 9 sectors/track, 512-byte sectors, 1 reserved track, 1k blocks, 2 directory blocks, gap lengths 2Ah and 52h, not bootable."

The DEC Rainbow releases are only for DEC Rainbow computers. To convert the 400k image files for writing with ImageDisk, use the command: BIN2IMD <image.img> <image.imd> DM=4 N=80 SS=512 SM=1-10 /1


Much CP/M software uses the Xerox 820's disk format, and other computers such as the Kaypro II are compatible with it.[10] [11] The CRT unit contains the processor, and a large port on the back connected via heavy cable to a disk drive, allowing a wide variety of configurations. Disk drives can be daisy-chained via a port on the back.

Component	Capacity	Tracks/disk	Sectors/track	Bytes/sector	Notes
Dual 5.25" single-sided floppy drives	81K usable single density, 155K double density	40	18 or 17	128 or 256	All floppy disks are soft-sectored
Dual 5.25" double-sided floppy drives	172K usable SD, 322K DD	80	18 or 17	128 or 256
Dual 8" single-sided floppy drives	241K usable SD, 482K DD	77	26	128 or 256
Dual 8" double-sided floppy drives	490K usable SD, 980K DD	154	26	128 or 256
8" rigid disk drive	8.19MB	1024	32	256	Provided with one 8" double-sided floppy drive, which was expandable to three such floppy drives

More info about the hard disk images in this directory:
The images are sector-by-sector representations of the data on the hard disks,
in the order in which Martin Eberhard's ADEXER tool's XD command would send them:
  cylinder 0 head 0 sector 0..24
  cylinder 0 head 1 sector 0..24
  cylinder 1 head 0 sector 0..24
  cylinder 1 head 1 sector 0..24
  ...
  cylinder 405 head 0 sector 0..24
  cylinder 405 head 1 sector 0..24


*/

