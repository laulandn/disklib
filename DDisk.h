#ifndef DLIB_DDISK_H
#define DLIB_DDISK_H


// Half done hack for now...
#define diskLibOutputStream aDefaultOutputStream
//AWindowOutputStream *diskLibOutputStream=NULL;


#include "DDiskTypes.h"
#include "DDirEntry.h"

#include "DFinderDatEntry.h"


#define DDISK_MSG_SIZE 1024


#define DDISK_ERR_NO_IMPL "not implemented!\n"
#define DDISK_ERR_NO_DISK "no disk available.\n"
#define DDISK_ERR_COULDNT_OPEN "error, couldn't open file!\n"
#define DDISK_ERR_READ_ERROR "read error!\n"
#define DDISK_ERR_WRITE_ERROR "write error!\n"
#define DDISK_ERR_BAD_SIZE "error, bad size!\n"
#define DDISK_ERR_NO_BUFFER "error, no buffer!\n"
#define DDISK_ERR_BAD_BUFFER "error, bad buffer size!\n"
#define DDISK_ERR_NON_DOS "error, non-dos disk!\n"
#define DDISK_ERR_NO_IMAGE "error, no image buffer!\n"
#define DDISK_ERR_NO_FP "error, no file handle!\n"
#define DDISK_ERR_NO_FILEENTRY "error, couldn't get file entry!\n"
#define DDISK_ERR_DISK "disk error!\n"
#define DDISK_ERR_BAD_TRACK "error, bad track number!\n"
#define DDISK_ERR_BAD_SECTOR "error, bad sector number!\n"
#define DDISK_ERR_BAD_BLOCK "error, bad block number!\n"
#define DDISK_ERR_BAD_SIDE "error, bad size!\n"
#define DDISK_ERR_UNKNOWN_GEOM "error, geometry unknown!\n"
#define DDISK_ERR_UNKNOWN "unknown error!\n"


#define DDISK_STATUS_GOOD         0
#define DDISK_STATUS_READ_ERROR   1
#define DDISK_STATUS_WRITE_ERROR  2
#define DDISK_STATUS_BAD_FORMAT   3
#define DDISK_STATUS_BAD_BLOCKNUM 4
#define DDISK_STATUS_NO_FS        5


#define DDISK_FORMAT_DEFAULT 0


// The size was arbitrary...
#define EDISK_BUF_SIZE 1024

// Now, why did I do this again?
//#define awstat_struct stat


class DPartitionTable;


class DDisk : public ABaseClass_disk {
public:
  // Public Member Functions
  DDisk(const char *fname=NULL);
  virtual ~DDisk();
  //
  static bool recognize(const char *fname);
  static bool recognizeFileExtension(const char *fname);
  virtual const char *guessSystem(const char *fname);
  virtual const char *getName() { return "unformatted disk"; }
  //
  virtual bool getDiskError();
  virtual UINT getDiskStatus(); // Anything but zero is a problem...
  virtual const char *getDiskErrorString();
  virtual const char *getStatusString();
  virtual const char *getMessageBuffer() { return messageBuffer; }
  //
  // NOTE: State of unmounted disk is undefined, don't try and do anything with it!
  virtual bool Mount(const char *fname);
  virtual bool Unmount();
  //
  // These should be (almost) exact copies of the real functions
  virtual void *Fopen(const char *filename,const char *mode);  // return value is opaque!
  virtual UINT Fclose(void *fp);
  virtual size_t Fread(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  virtual size_t Fwrite(uint8_t *ptr,size_t sizelem,size_t n,void *fp);
  virtual int Ferror(void *fp);
  virtual int Feof(void *fp);
  virtual int Fseek(void *fp,long offset,int origin);
  virtual long Ftell(void *fp);
  virtual int Fstat(int fd,struct awstat_struct *st);
  //
  virtual int Stat(const char *filename,struct awstat_struct *st);
  virtual void *Open(const char *filename,UINT mode);  // return value is opaque!
  virtual UINT Close(void *fp);
  virtual size_t Read(void *fp,uint8_t *ptr,size_t n);
  virtual size_t Write(void *fp,uint8_t *ptr,size_t n);
  virtual int Lseek(void *fp,long offset,int origin);
  //
  virtual bool Unlink(const char *filename);
  virtual bool Chdir(const char *dname);
  virtual const char *Getcwd(char *buf,size_t len);
  //
  // These are either my own, or have different names for convenience , or some other reason...
  virtual int Rename(const char *oldname,const char *newname);
  virtual bool myCopyFile(const char *oldname,const char *newname);
  // NOTE, addr in readMany is an address in a map, not an actual pointer
  virtual size_t readMany(void *fp,uint8_t *buffer,size_t n);
  //
  virtual bool isDir(const char *filename);  // is it a dir and not a normal file?
  virtual bool isSpecial(const char *filename);  // is it "special" and not a file?
  virtual UINT getFileVersionMajor(const char *filename);
  virtual UINT getFileVersionMinor(const char *filename);
  virtual size_t getFileSize(const char *filename);
  //
  virtual const char *searchForFirstCached(const char *fname);
  virtual const char *searchForNextCached();
  virtual DDirEntry *searchForFirstFileCached(const char *fname);
  virtual DDirEntry *searchForNextFileCached();
  //
  virtual const char *searchForFirst(const char *fname);
  virtual const char *searchForNext();
  virtual DDirEntry *searchForFirstFile(const char *fname);
  virtual DDirEntry *searchForNextFile();
  //
  virtual bool readBlock(UINT blk);
  //
  virtual bool readPhysTrackSector(UINT tt,UINT ss);
  virtual bool readPhysBlock(UINT blk);
  virtual bool writePhysTrackSector(UINT tt,UINT ss);
  virtual bool writePhysBlock(UINT blk);
  //
  virtual bool readLogicalTrackSector(UINT tt,UINT ss);
  virtual bool readLogicalBlock(UINT blk);
  virtual bool writeLogicalTrackSector(UINT tt,UINT ss);
  virtual bool writeLogicalBlock(UINT blk);
  //
  virtual bool markPhysBlockUsed(UINT blk);
  virtual bool markPhysBlockUnused(UINT blk);
  //
  virtual bool markLogicalBlockUsed(UINT blk);
  virtual bool markLogicalBlockUnused(UINT blk);
  //
  virtual uint8_t *getBuffer() { return buf; }
  virtual void flushBuffers();
  virtual bool refreshBuffers();
  //
  virtual const char *getShortName(const char *longName);
  virtual const char *getDiskName() { return (const char *)diskName; }
  virtual const char *getDiskID() { return (const char *)diskID; }
  virtual const char *getCurDir() { return (const char *)curDir; }
  virtual UINT getTrack() { return track; }
  virtual UINT getSector() { return sector; }
  virtual UINT getSide() { return side; }
  virtual UINT getPhysBlockSize() { return blockSize; }
  virtual UINT getLogicalBlockSize();
  virtual bool getWriteProtected() { return writeProtected; }
  virtual bool getHasPartitions() { return hasPartitions; }
  //
#ifdef USE_FINDER_DAT
  virtual bool setIgnoreFinderDat(bool t);
  virtual bool getIgnoreFinderDat() { return ignoreFinderDat; }
#endif // USE_FINDER_DAT
  //
  // Don't mess with the list when you get it!!!
  virtual AList_disk *getTheDir();
  virtual DDirEntry *findDirEntry(const char *fname);
  //
  virtual UINT getMaxTrack() { return maxTrack; }
  virtual UINT getMaxSector() { return maxSector; }
  virtual UINT getNoTrackZero() { return noTrackZero; }
  //
  virtual bool sanityCheck();
  virtual void debugDump();
  //
protected:
  // Private Member Functions
  virtual void init();
  //
  virtual bool detectSubFormat();
  virtual bool basicSanityCheck();
  //
  virtual bool readDirEntry(long offset);
  //
  virtual bool readDirectory();
  virtual bool readGeometry();
  virtual bool readBootBlock();
  virtual bool cacheFile(void *fp);
  virtual void freeDirCache();
  virtual void freeFileCache(void *fp);
  //
  UINT transPhysToLogTrack(UINT track);
  UINT transLogToPhysTrack(UINT track);
  UINT transPhysToLogSector(UINT sector);
  UINT transLogToPhysSector(UINT sector);
  UINT transPhysToLogBlock(UINT block);
  UINT transLogToPhysBlock(UINT block);
  UINT transPhysToLogSide(UINT side);
  UINT transLogToPhysSide(UINT side);
  //
  UINT transLogTrackSector2Block(UINT ttrack,UINT tsector);
  UINT transPhysTrackSector2Block(UINT ttrack,UINT tsector);
  //
  virtual bool removeAllFilesinFolder(const char *foldername);
  //
  static size_t getSize(const char *filename);
  //
  virtual UINT popCount(char q);
  //
  static void dumpBufHex(uint8_t *theBuffer,UINT theSize);
  //
  virtual void removeSpacesFromFilenames();
  //
  //
#ifdef USE_FAKE_CHDIR
  virtual const char *fakeFullPath(const char *name);
#endif // USE_FAKE_CHDIR
  //
  // Private Data Members
  uint8_t buf[EDISK_BUF_SIZE];
  long curLoc;
  bool diskError;
  UINT diskStatus;
  bool writeProtected;
  UINT track,sector,side;
  UINT blockSize;
  // If !allTracksSameSectors, maxSector is meaningless...
  UINT maxTrack,maxSector,maxSide;
  bool allTracksSameSectors;
  bool noTrackZero,noSectorZero;  // Instead start at one
  bool hasPartitions;
  DPartitionTable *partitionTable; // Non-NULL if we're a partition
  UINT partitionNumber; // Our number if we're a partition
  AList_disk theDir;
  char matchName[DDISK_MAX_NAMELEN]; // for finds
  char diskName[DDISK_MAX_NAMELEN]; // can be left blank
  char diskID[DDISK_MAX_NAMELEN];   // can be left blank
  char curDir[DDISK_MAX_NAMELEN];   // can be left blank
#ifdef USE_FAKE_CHDIR
  const char *prefixPath;  // can be left blank
  char fullPathName[DDISK_MAX_NAMELEN];   // can be left blank
#endif // USE_FAKE_CHDIR
  UINT subFormat;
  bool noFileSystem;
  bool cachesData;
  bool cachesFiles;
  bool cachesDir;
  bool doNotCacheDir;
  bool doNotCacheFiles;
#ifdef USE_FINDER_DAT
  bool ignoreFinderDat;
#endif // USE_FINDER_DAT
  char messageBuffer[DDISK_MSG_SIZE];
};


#endif // DLIB_DDISK_H

