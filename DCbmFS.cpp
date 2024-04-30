
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


#include <stdlib.h>
#include <string.h>


#include "DCbmFS.h"


#define HIGH_BYTDCbmFS(x)  ((x&0xff00)>>8)
#define LOW_BYTDCbmFS(x)  (x&0xff)


static UINT numSectors1541[]=
{
  21,21,21,21,21,21,21,21,21, // Tracks 1-9
  21,21,21,21,21,21,21,21,    // Tracks 10-17
  19,19,19,19,19,19,19,       // Tracks 18-24
  18,18,18,18,18,18,          // Tracks 25-30
  17,17,17,17,17              // Tracks 31-35
};


static UINT numSectors1571[]=
{
  21,21,21,21,21,21,21,21,21, // Tracks 1-9
  21,21,21,21,21,21,21,21,    // Tracks 10-17
  19,19,19,19,19,19,19,       // Tracks 18-24
  18,18,18,18,18,18,          // Tracks 25-30
  17,17,17,17,17,              // Tracks 31-35
  21,21,21,21,21,21,21,21,21, // Tracks 1-9 (other side)
  21,21,21,21,21,21,21,21,    // Tracks 10-17 (other side)
  19,19,19,19,19,19,19,       // Tracks 18-24 (other side)
  18,18,18,18,18,18,          // Tracks 25-30 (other side)
  17,17,17,17,17              // Tracks 31-35 (other side)
};


static UINT trackOffset1541[]=
{
  0,21,42,63,84,105,126,147,168,    // Tracks 1-9
  189,210,231,252,273,294,315,336,  // Tracks 10-17
  357,376,395,414,433,452,471,      // Tracks 18-24
  490,508,526,544,562,580,          // Tracks 25-30
  598,615,632,649,666               // Tracks 31-35
  // For a total of 683 blocks
};


static UINT trackOffset1571[]=
{
  0,21,42,63,84,105,126,147,168,    // Tracks 1-9
  189,210,231,252,273,294,315,336,  // Tracks 10-17
  357,376,395,414,433,452,471,      // Tracks 18-24
  490,508,526,544,562,580,          // Tracks 25-30
  598,615,632,649,666,              // Tracks 31-35 (other side)
  // Tracks 1-9 (other side)
  0+683,21+683,42+683,63+683,84+683,105+683,126+683,147+683,168+683,
  // Tracks 10-17 (other side)
  189+683,210+683,231+683,252+683,273+683,294+683,315+683,336+683,
  // Tracks 18-24 (other side)
  357+683,37+6836,395+683,414+683,433+683,452+683,471+683,
  // Tracks 25-30 (other side)
  490+683,508+683,526+683,544+683,562+683,580+683,
  // Tracks 31-35 (other side)
  598+683,615+683,632+683,649+683,666+683
  // For a total of 683*2 blocks
};


////////////////////////////////////////////////////////////////////////////////
//  DCbmFS Class
////////////////////////////////////////////////////////////////////////////////

DCbmFS::DCbmFS(const char *fname) : DDiskImageMem(fname)
{
  debug_msg("DCbmFS::DCbmFS("); debug_msg(fname); debug_msg("')\n");
  init();
  Mount(fname);
  detectSubFormat();
}


const char *DCbmFS::guessSystem(const char *fname)
{
  return "c64";
}


void DCbmFS::init()
{
  //if(w) debug_msg("DCbmFS disk image.\n");
  id1=0;  id2=0;  dos1=0;  dos2=0;
  // Set up for a default 1541 image...
  // This will be overridden if its a 1581, etc...
  /*
  numSectors=numSectors1541;
  trackOffset=trackOffset1541;
  cbmDirTrack=18;
  allTracksSameSectors=false;
  maxTrack=35;  maxSector=21;  maxSide=1;
  noTrackZero=true;
  blockSize=256;
  */
  numSectors=NULL;
  trackOffset=NULL;
  cbmDirTrack=0;
  allTracksSameSectors=false;
  maxTrack=0;  maxSector=0;  maxSide=0;
  noTrackZero=false;
  blockSize=0;
  geos=false;
  cpm=false;
  gcr=false;
  cpmDisk=NULL;
}


DCbmFS::~DCbmFS()
{
  freeDirCache();
  if(imageData) delete imageData;
  imageData=NULL;
  diskMounted=false;
}


/* STATIC */
bool DCbmFS::recognize(const char *fname)
{
  if(!recognizeFileExtension(fname)) return false;
  bool ret=false;
  FILE *f=fopen(fname,"rb");
  if(!f) { debug_msg("DCbmFS::recognize('"); debug_msg(fname); debug_msg("') "); debug_msg(DDISK_ERR_COULDNT_OPEN); return ret; }
  //
  size_t size=getSize(fname);
  bool sizeIsGood=true;
  if((size!=174848)&&(size!=533248)&&(size!=819200)&&(size!=349696)&&(!333744)) {
    sizeIsGood=false;
    return false;
  }
  //
  char *tbuf=(char *)malloc(size);
  if(!tbuf) { /*::fclose(f);*/ debug_msg("DCbmFS::recognize "); debug_msg(DDISK_ERR_NO_BUFFER); return ret; }
  size_t nread=::fread(tbuf,1,size,f);
  // TODO: Check nread value!
  if(!strncmp(tbuf,"GCR-1541",8)) {
    debug_msg("NOTE: Raw GCR 1541 image...\n");
    //return false;
  }
  if(f) ::fclose(f);
  //
  if((tbuf[0]==0x43)&&(tbuf[1]==0x42)&&(tbuf[2]==0x4d)) {
    debug_msg("Found a C128 boot block...\n");
    //
#ifdef DEBUG_VERBOSE
    dumpBufHex((uint8_t *)tbuf,256);
#endif
    unsigned extraBlocks=tbuf[6];
    if(!extraBlocks) {
      debug_msg("(No extra blocks to load...)\n");
      if(!tbuf[7]) {
        debug_msg("(No msg to show...)\n");
        // No mesg...for now...
        if(!tbuf[8]) {
          debug_msg("(No filename to load...)\n");
          // No filename...for now...
          // assume ml starts at byte 9...
        }
        else debug_msg("(There is a filename to load...)\n");

      }
      else debug_msg("(There is a message to show...)\n");

    }
    else debug_msg("(There are extra blocks to load...)\n");
  }
  //
  debug_msg("DCbmFS::recognize was successful!\n");
  free(tbuf);
  ret=true;
  return ret;
}


/* STATIC */
bool DCbmFS::recognizeFileExtension(const char *fname)
{
  char ext[4];
  ext[0]=fname[strlen(fname)-3];
  ext[1]=fname[strlen(fname)-2];
  ext[2]=fname[strlen(fname)-1];
  ext[3]=0;
  //debug_msg("DCbmFS::recognizeFileExtension ext="); debug_msg(ext); debug_msg("\n");
  if(!strcmp(ext,"d64")) return true;
  if(!strcmp(ext,"g64")) return true;
  if(!strcmp(ext,"d71")) return true;
  if(!strcmp(ext,"d81")) return true;
  return false;
}


/*
void DCbmFS::ls(const char *dname)
{
  if(noFileSystem) { debug_msg("DCbmFS "); debug_msg(DDISK_ERR_NON_DOS); return; }
  //w->startMore();
  //w->dec();
  debug_msg("Disk: '"); debug_msg(diskName); debug_msg("'";
  debug_msg("  id='"); debug_msg(id1<<id2); debug_msg("' dos='"); debug_msg(dos1<<dos2); debug_msg("'\n");
  theDir.jumpToHead();
  DDirEntry *aFile=(DDirEntry *)theDir.info();
  while(aFile) {
    UINT afsize=aFile->sizeBlocks;
    debug_msg(afsize;
    if(afsize<100) debug_msg(" ";
    if(afsize<10) debug_msg(" ";
    debug_msg("  ";
    debug_msg("'"); debug_msg(aFile->name); debug_msg("'";
    UINT afnlen=strlen(aFile->name);
    for(UINT t=0;t<(24-afnlen);t++) debug_msg(" ";
    switch(aFile->nativeType) {
      case 0:  debug_msg("DEL";  break;
      case 1:  debug_msg("SEQ";  break;
      case 2:  debug_msg("PRG";  break;
      case 3:  debug_msg("USR";  break;
      case 4:  debug_msg("REL";  break;
      case 5:  debug_msg("CBM";  break;
      case 6:  debug_msg("DIR";  break;
      default: debug_msg("???";  break;
    }
    debug_msg("\n");
    theDir.advance();
    aFile=(DDirEntry *)theDir.info();
  }
  // debug_msg(blocksFree); debug_msg(" blocks free.\n");
  //w->hex();
  //w->endMore();
}
*/


bool DCbmFS::readDirectory()
{
  if(cpm) return readDirectoryCpm();
  //else debug_msg("not cpm\n");
#ifdef DEBUG_VERBOSE
  debug_msg("DCbmFS::readDirectory...\n");
#endif
  freeDirCache();
  //
  readGeometry();
  //
  readTrackSector(cbmDirTrack,0);
  if(diskError) {
    debug_msg("Can't read dir, "); debug_msg(DDISK_ERR_DISK);
    noFileSystem=true;
    diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
    return false;
  }
  //
  UINT tt,ss,t;
  bool ww=true;
  while(ww) {
    //debug_msg("Reading dir sector...\n");
    tt=buf[0];  ss=buf[1];
    if(tt==0) ww=false;
    else {
      readTrackSector(tt,ss);
      if(diskError) {
        debug_msg("Can't read dir, "); debug_msg(DDISK_ERR_DISK);
        noFileSystem=true;
        diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
        return false;
      }
      uint32_t off;
      for(t=0;t<7;t++) {
        off=t*32+2;
        readDirEntry(off);
      }
    }
  }
  return true;
}


bool DCbmFS::readDirEntry(long off)
{
  if(cpm) return readDirEntryCpm(off);
  //else debug_msg("not cpm\n");
  debug_msg("DCbmFS::readDirEntry("); debug_long(off); debug_msg(")...\n");
  UINT t=0;
  if(buf[off]&0x7f) {
    DDirEntry *theFile=new DDirEntry;
    if(theFile) {
      theFile->signature=0xface;
      for(t=0;t<256;t++) theFile->name[t]=0;
      for(t=3;t<19;t++) {
        if(buf[off+t]!=160) theFile->name[t-3]=buf[off+t];
      }
      theFile->sizeBlocks=buf[off+28]+(buf[off+29]*256);
      theFile->nativeType=buf[off]&0x7f;
      switch(theFile->nativeType) {
        case 0:  theFile->type=DDISK_TYPE_NONE;  break;
        case 1:  theFile->type=DDISK_TYPE_TEXT;  break;
        case 2:  theFile->type=DDISK_TYPE_BASIC;  break;
        case 3:  theFile->type=DDISK_TYPE_BINARY;  break;
        case 4:  theFile->type=DDISK_TYPE_BINARY;  break;
        case 5:  theFile->type=DDISK_TYPE_DIR;  break;
        case 6:  theFile->type=DDISK_TYPE_DIR;  break;
        default:  theFile->type=DDISK_TYPE_NONE;  break;
      }
      theFile->track=buf[off+1];
      theFile->sector=buf[off+2];
      theFile->data=NULL;
      theFile->size=theFile->sizeBlocks*256;
      theFile->block=0;
      theFile->curSeek=0;
      theFile->debugDump();
      theDir.insert((void *)theFile);
    }
    else { debug_msg("Couldn't alloc file entry!\n"); return false; }
    if(!theFile->checkDiskSanity(this)) return false;
  }
  else {
    if(!(buf[off]&0x7f)) {
      // empty dir entry...ignore...
      debug_msg("(dir entry was empty)\n");
      return false;
    }
    else {
      debug_msg("DCbmFS buf[off]&0x7f="); debug_int((buf[off]&0x7f)); debug_msg(" "); debug_msg(DDISK_ERR_NO_FILEENTRY);
      return false;
    }
  }
  return true;
}


/*
// Doesn't work with current setup
// TODO: Redesign this...it only works if you read one and only one full sector at a time
uint32_t DCbmFS::Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp)
{
  if(!fp) return 0;
  if(!ptr) return 0;
  //EMapAbstract *myMap=dos->getMap();
  DDirEntry *aFile=(DDirEntry *)fp;
  char *tptr=(char *)ptr;
  UINT tt=aFile->track,ss=aFile->sector;
  UINT wantSize=sizelem*n;
  UINT bytesRead=0,ssize,t;
  bool WW=true;
  uint32_t offset=0,poff=0;
  while(WW) {
    if(bytesRead==wantSize) WW=false;
    else {
      readTrackSector(tt,ss);
      tt=buf[0];  ss=buf[1];
      ssize=254;
      offset=0;
      if(!tt) { ssize=ss;  WW=false; }  // At end of file...
      //if(firstSector) { firstSector=false; offset=2; ssize-=2; }
      if((bytesRead+ssize)>wantSize) ssize=(bytesRead+ssize)-wantSize;
      for(t=0;t<ssize;t++) {
        tptr[t+poff]=buf[t+offset+2];
        //poff++;
        bytesRead++;
      }
    }
  }
  //lastSeek+=bytesRead;
  return bytesRead;
}
*/


// NOTE: We read the whole file, start address included.
bool DCbmFS::cacheFile(void *fp)
{
  if(!fp) {
    debug_msg("DCbmFS couldn't cache file "); debug_msg(DDISK_ERR_NO_FP);
    return false;
  }
  DDirEntry *aFile=(DDirEntry *)fp;
  if(aFile->data) free(aFile->data);
  aFile->data=(char *)malloc(aFile->size);
  if(!aFile->data) {
     debug_msg("DCbmFS couldn't cache file "); debug_msg(DDISK_ERR_NO_BUFFER);
     return false;
  }
  UINT tt=aFile->track,ss=aFile->sector;
  UINT ssize,t;
  UINT poff=0;
  bool working=true;
  while(working) {
    readTrackSector(tt,ss);
    if(diskError) {
      debug_msg("Can't cacheFile, "); debug_msg(DDISK_ERR_DISK);
      return false;
    }
    tt=buf[0];  ss=buf[1];
    ssize=254;
    if(!tt) { ssize=ss;  working=false; }  // At end of file...
    for(t=0;t<ssize;t++) {
      aFile->data[poff]=buf[t+2];
      poff++;
    }
  }
  return true;
}


bool DCbmFS::readGeometry()
{
  debug_msg("DCbmFS::readGeometry()...\n");
  bool ret=true;
  //
  detectSubFormat();
  noFileSystem=false;
  // First, assume 1541...
  numSectors=numSectors1541;
  trackOffset=trackOffset1541;
  allTracksSameSectors=false;
  maxTrack=35;  maxSector=21;  maxSide=1;
  cbmDirTrack=18;
  noTrackZero=true;
  blockSize=256;
  geos=false;
  //cpm=false;
  switch(imageDataSize) {
    case 174848:
      // A 1541, we're already set up...
      break;
    case 533248:
      // A 8050...fix things somewhat...
      debug_msg("Detected CBM 8050 disk, not implemented!\n");
      return false;
      break;
    case 819200:
      // A 1581...fix things somewhat...
      allTracksSameSectors=true;
      maxTrack=80;
      maxSector=39;
      maxSide=2;
      cbmDirTrack=40;
      // TODO: this isn't finished...
      break;
    case 349696:
      // double sided 1571
      maxTrack=70;
      maxSector=21;
      maxSide=2;
      numSectors=numSectors1571;
      trackOffset=trackOffset1571;
      cbmDirTrack=18;
      // TODO: this isn't finished...
      break;
      /*
    case 333744:
      // raw 1541 gcr?
      break;
      */
    default:
     debug_msg("Can't read dir, disk size is weird!\n");
     noFileSystem=true;
     diskStatus=DDISK_STATUS_BAD_FORMAT;
     return false;
     break;
  }
  //
  if(!cpm) return readGeometryMore();
  //
  return ret;
}


bool DCbmFS::readGeometryMore()
{
  //debug_msg("DCbmFS::readGeometryMore()...\n");
  blocksFree=0;
  UINT t=0;
  readTrackSector(cbmDirTrack,0);
  if(diskError) {
    debug_msg("Can't read dir, "); debug_msg(DDISK_ERR_DISK);
    noFileSystem=true;
    diskStatus=DDISK_STATUS_BAD_BLOCKNUM;
    return false;
  }
  /*
  // read bam
  // TODO: This is wrong for anything but a plain 1541 disk image...
  for(t=4;t<143;t+=4) {
    blocksFree+=popCount(buf[t+1]);
    blocksFree+=popCount(buf[t+2]);
    blocksFree+=popCount(buf[t+3]);
  }
  */
  id1=buf[162];  id2=buf[163];
  dos1=buf[165];  dos2=buf[166];
  diskID[0]=id1;  diskID[1]=id2;  diskID[2]=32;
  diskID[3]=dos1;  diskID[4]=dos2; diskID[5]=0;
  // TODO: CHeck DOS version as part of sanity check (1541 is "2A")
  for(t=0;t<256;t++) diskName[t]=0;
  for(t=144;t<161;t++) {
    if(buf[t]!=160) diskName[t-144]=buf[t];
  }
  return true;
}


bool DCbmFS::readBootBlock()
{
  debug_msg("DCbmFS::readBootBlock()...\n");
  bool ret=false;
  readBlock(0);
  if((buf[0]==0x43)&&(buf[1]==0x42)&&(buf[2]==0x4d)) {
    // Found "CBM" signature...
    // TODO: Detect CP/M boot sector here!
    // 0: 3 bytes CBM
    // 3: 2 byte "starting address"
    // 5: byte "config index"
    // 6: byte "number of add'n blocks to read"
    // 7: C string to output in petsci up to zero
    // 8?: File name to load up to zero
    // 9?: If both the above were zero, what follows is code to exec.
    return true;
  }
  return ret;
}


bool DCbmFS::detectSubFormat()
{
  debug_msg("DCbmFS::detectSubFormat()...\n");
  //debug_int(imageDataSize); debug_msg(" was imageDataSize\n");
  subFormat=DCBM_FORMAT_UNKNOWN;
  if(imageDataSize==174848) subFormat=DCBM_FORMAT_1541;  // 1541
  if(imageDataSize==533248) subFormat=DCBM_FORMAT_PET1;  // PET?
  if(imageDataSize==819200) subFormat=DCBM_FORMAT_1581;  // 1581
  if(imageDataSize==349696) subFormat=DCBM_FORMAT_1571;  // 1571
  if(imageDataSize==333744) subFormat=DCBM_FORMAT_PET2;  // PET?
  //
  cpm=detectCpm();
  //
  if(subFormat==DCBM_FORMAT_UNKNOWN) {
    debug_msg("(Couldn't detectSubFormat)\n");
    return false;
  }
  return true;
}


bool DCbmFS::detectCpm()
{
  debug_msg("DCbmFS::detectCpm()...\n");
  bool ret=false;
  switch(subFormat) {
    case DCBM_FORMAT_1541:
      // Looking for start of copyright...not sure if this will always work
      if((imageData[0x810]=='C')&&(imageData[0x810+1]=='o')&&(imageData[0x810+2]=='p')&&(imageData[0x810+3]=='y')) ret=true;
      break;
    case DCBM_FORMAT_1571:
      // Looking for start of copyright...not sure if this will always work
      if((imageData[0x610]=='C')&&(imageData[0x610+1]=='o')&&(imageData[0x610+2]=='p')&&(imageData[0x610+3]=='y')) ret=true;
      break;
    case DCBM_FORMAT_1581:
      // Definitely for bootable, not sure if all disks have this...
      if((imageData[0x61e63]=='C')&&(imageData[0x61e63+1]=='P')&&(imageData[0x61e63+2]=='/')&&(imageData[0x61e63+3]=='M')) ret=true;
      break;
    default:
      debug_msg("bad subFormat for detectCpm\n");
     break;
  }
  if(ret) debug_msg("Disk is a commodore CP/M disk...\n"); //else debug_msg("was not cpm\n");
  return ret;
}


// NOTE: This is actually a pretty terrible hack using a 2nd disk!
bool DCbmFS::readDirectoryCpm()
{
  debug_msg("DCbmFS::readDirectoryCpm()...\n");
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
bool DCbmFS::readDirEntryCpm(long offset)
{
  debug_msg("DCbmFS::readDirEntryCpm()...\n");
  return cpmDisk->readDirEntry(offset);
}


bool DCbmFS::sanityCheck()
{
  debug_msg("DCbmFS::sanityCheck()...\n");
  if(!readGeometry()) { debug_msg("readGeometry failed!\n"); return false; }
  if(!basicSanityCheck()) { debug_msg("basicSanityCheck failed!\n"); return false; }
  //
  // TODO: Look at directory if it's sane or not
  //
  // By default
  return true;
}
