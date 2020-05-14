/******************************************************************************
*
*  OS_disk.h
*
*  REVISION HISTORY:
*     23 OCT 2002 jcs  Created.
*     13 JUL 2017 jcs  librtEdge
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#ifndef __EDG_DISK_H
#define __EDG_DISK_H
#include <EDG_Internal.h>


namespace RTEDGE_PRIVATE
{

// Templatized GLvector collections

#define GLvecDiskStat     GLvector<DiskStat *>
#define GLvecFileSysStat  GLvector<FileSystem *>

/////////////////////////////////////////
// /proc/diskstat
/////////////////////////////////////////
class DiskStat : public ::OSDiskStat,
                 public string
{
public:
	DiskStat *_top;

	// Constructor / Destructor
public:
	DiskStat( char * );
	DiskStat( DiskStat &, DiskStat & );

	// Mutator

	void SetTop( const DiskStat &, double );
	void Pair( DiskStat * );

	// Assignment Operators

	DiskStat &operator=( DiskStat & );
};

typedef hash_map<string, DiskStat *> DiskStatMap;

class GLdiskStat : public string
{
public:
	static char **_KO;
private:
	DiskStatMap   _disksH;
	GLvecDiskStat _disks;
	GLvecDiskStat _tops;
	double        _d0;

	// Constructor / Destructor
public:
	GLdiskStat( const char *pFile="/proc/diskstats" );
	~GLdiskStat();

	// Access / Operations
public:
	GLvecDiskStat &disks();
	GLvecDiskStat &tops();
	DiskStat     *GetDisk( const char * );
	int           nDsk();
	GLdiskStat   &Snap();
	void          Dump();
};


/////////////////////////////////////////
// /etc/mtab -> ::statvfs()
/////////////////////////////////////////
#ifdef WIN32
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
 
struct statvfs {
   unsigned long  f_bsize;    /* file system block size */
   unsigned long  f_frsize;   /* fragment size */
   fsblkcnt_t     f_blocks;   /* size of fs in f_frsize units */
   fsblkcnt_t     f_bfree;    /* # free blocks */
   fsblkcnt_t     f_bavail;   /* # free blocks for non-root */
   fsfilcnt_t     f_files;    /* # inodes */
   fsfilcnt_t     f_ffree;    /* # free inodes */
   fsfilcnt_t     f_favail;   /* # free inodes for non-root */
   unsigned long  f_fsid;     /* file system ID */
   unsigned long  f_flag;     /* mount flags */
   unsigned long  f_namemax;  /* maximum filename length */
};
#else
#include <sys/statvfs.h>
#endif // WIN32

class FileSystem : public string
{
public:
	string         _mnt;
	struct statvfs _st;

	// Constructor / Destructor
public:
	FileSystem( char *, char * );
	~FileSystem();

	// Access / Mutator
public:
	char  *pFileSys();
	char  *pMount();
	double PctUse();
	double SizeGb();
	double UsedGb();
	double AvailGb();
	void   Snap();
};

class GLfileSysStat : public string
{
public:
	GLvecFileSysStat _fs;

	// Constructor / Destructor
public:
	GLfileSysStat( const char *mtab="/etc/mtab" );
	~GLfileSysStat();

	// Access / Operations

	GLfileSysStat &Snap();
	void           Dump();
};

}  // namespace RTEDGE_PRIVATE

#endif // __EDG_DISK_H
