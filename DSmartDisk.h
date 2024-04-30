#ifndef DLIB_DSMARTDISK_H
#define DLIB_DSMARTDISK_H


///////////////////////////////////////////////////////////////////////////////

// IN PROGRESS  IN PROGRESS  IN PROGRESS  IN PROGRESS  IN PROGRESS  IN PROGRESS

//////////////////////////////////////////////////////////////////////////////


#include "Dabasics.h"

#include "DDisk.h"


// NOTE: The idea is this would work like ASmartImageDecoder, but doesn't do anything yet...


class DSmartDisk : public ABaseClass_disk
{
public:
  DSmartDisk(const char *fname);
  virtual ~DSmartDisk();
  //
  static DDisk *pickBestDisk(const char *fname);
  //
  DDisk *getTheDisk();
  //
protected:
  //
  virtual bool Mount(const char *fname);
  virtual bool Unmount();
  //
  DDisk *theDisk;
  //
};


#endif // DLIB_DSMARTDISK_H

