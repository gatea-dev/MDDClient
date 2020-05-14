/******************************************************************************
*
*  OS_disk.cpp
*
*  REVISION HISTORY:
*     23 OCT 2002 jcs  Created.
*     13 JUL 2017 jcs  librtEdge
*
*  (c) 1994-2017 Gatea Ltd.
*******************************************************************************/
#include <OS_disk.h>

using namespace RTEDGE_PRIVATE;

static char *_SEP = " ";

////////////////////////////////////////////////////////////////////////////
// 
//               c l a s s      D i s k S t a t
// 
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
DiskStat::DiskStat( char *def ) :
   string( "Undefined" ),
   _top( (DiskStat *)0 )
{
   string s( def );
   char  *sp, *pv, *rp;
   double dv;
   int    i;

   // Init

   _diskName = "undefined";
   _nRd      = 0.0;
   _nRdM     = 0.0;
   _nRdSec   = 0.0;
   _nRdMs    = 0.0;
   _nWr      = 0.0;
   _nWrM     = 0.0;
   _nWrSec   = 0.0;
   _nWrMs    = 0.0;
   _nIo      = 0.0;
   _nIoMs    = 0.0;
   _wIoMs    = 0.0;

   //    8    1 sda1 24 54 624 166 0 0 0 0 0 166 166

   sp = (char *)s.data();
   pv = ::strtok_r( sp, _SEP, &rp );
   for ( i=0; pv; pv=::strtok_r( NULL, _SEP, &rp ), i++ ) {
      dv = atof( pv );
//      dc = dv * _dClk;
      switch( i ) {
         case  2: assign( pv ); break;
         case  3: _nRd    = dv; break;
         case  4: _nRdM   = dv; break;
         case  5: _nRdSec = dv; break;
         case  6: _nRdMs  = dv; break;
         case  7: _nWr    = dv; break;
         case  8: _nWrM   = dv; break;
         case  9: _nWrSec = dv; break;
         case 10: _nWrMs  = dv; break;
         case 11: _nIo    = dv; break;
         case 12: _nIoMs  = dv; break;
         case 13: _wIoMs  = dv; break;
      }
   }
   _diskName = data();
}

DiskStat::DiskStat( DiskStat &c0, DiskStat &c1 )
{
   _nRd    = c0._nRd    - c1._nRd;
   _nRdM   = c0._nRdM   - c1._nRdM;
   _nRdSec = c0._nRdSec - c1._nRdSec;
   _nRdMs  = c0._nRdMs  - c1._nRdMs;
   _nWr    = c0._nWr    - c1._nWr;
   _nWrM   = c0._nWrM   - c1._nWrM;
   _nWrSec = c0._nWrSec - c1._nWrSec;
   _nWrMs  = c0._nWrMs  - c1._nWrMs;
   _nIo    = c0._nIo    - c1._nIo;
   _nIoMs  = c0._nIoMs  - c1._nIoMs;
   _wIoMs  = c0._wIoMs  - c1._wIoMs;
}


////////////////////////////////////////////
// Mutator
////////////////////////////////////////////
void DiskStat::SetTop( const DiskStat &c, double dd )
{
   assign( c.data() );
   _diskName = data();
   _nRd      = c._nRd / dd;
   _nRdM     = c._nRdM / dd;
   _nRdSec   = c._nRdSec / dd;
   _nRdMs    = ( 0.1 * c._nRdMs  ) / dd;
   _nWr      = c._nWr / dd;
   _nWrM     = c._nWrM / dd;
   _nWrSec   = c._nWrSec / dd;
   _nWrMs    = ( 0.1 * c._nWrMs  ) / dd;
   _nIo      = c._nIo / dd;
   _nIoMs    = ( 0.1 * c._nIoMs  ) / dd;
   _wIoMs    = ( 0.1 * c._wIoMs  ) / dd;
}

void DiskStat::Pair( DiskStat *top )
{  
   _top = top;
}


////////////////////////////////////////////
// Assignment Operators
////////////////////////////////////////////
DiskStat &DiskStat::operator=( DiskStat &c )
{
   assign( c.data() );
   _diskName = data();
   _nRd      = c._nRd;
   _nRd      = c._nRd;
   _nRdM     = c._nRdM;
   _nRdSec   = c._nRdSec;
   _nRdMs    = c._nRdMs;
   _nWr      = c._nWr;
   _nWrM     = c._nWrM;
   _nWrSec   = c._nWrSec;
   _nWrMs    = c._nWrMs;
   _nIo      = c._nIo;
   _nIoMs    = c._nIoMs;
   _wIoMs    = c._wIoMs;
   return *this;
}



////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L d i s k S t a t
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLdiskStat::GLdiskStat( const char *pFile ) :
   string( pFile ),
   _disksH(),
   _disks(),
   _tops(),
   _d0( dNow() )
{
   Snap();
}

GLdiskStat::~GLdiskStat()
{
   _disks.clearAndDestroy();
   _tops.clearAndDestroy();
}


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
GLvecDiskStat &GLdiskStat::disks()
{
   return _disks;
}

GLvecDiskStat &GLdiskStat::tops()
{
   return _tops;
}

DiskStat *GLdiskStat::GetDisk( const char *pDsk )
{
   DiskStatMap          &db = _disksH;
   DiskStatMap::iterator it;
   string                s( pDsk );
   DiskStat             *rtn;

   rtn = ( (it=db.find( s )) != db.end() ) ? (*it).second : (DiskStat *)0;
   return rtn;
}

int GLdiskStat::nDsk()
{
   return _disks.size();
}

GLdiskStat &GLdiskStat::Snap()
{
   DiskStat *s1, *sd;
   string    s;
   FILE     *fp;
   char     *pn, c1, c2;
   char      buf[K];
   int       i, sz;
   double    dn, dd;

   // Snap 'em

   dn  = dNow();
   dd  = dn - _d0;
   dd  = dn - _d0;
   _d0 = dn;
   fp = ::fopen( data(), "r" );
   for ( i=0; fp && ::fgets( buf, K, fp ); ) {
      chop( buf );

      DiskStat s0( buf );

      /*
       * Knock out ram, loop
       * Knock out sd?[0-9]
       * Keep dm-[0-9]
       */

      pn = (char *)s0.data();
      sz = strlen( pn );
      if ( ::strstr( pn, "ram" ) == pn )
         continue; // for-i
      if ( ::strstr( pn, "loop" ) == pn )
         continue; // for-i
      if ( sz > 2 ) {
         c1 = pn[sz-1];
         c2 = pn[sz-2];
         if ( InRange( '0', c1, '9' ) && ( c2 != '-' ) )
            continue; // for-i
      }
      if ( !(s1=GetDisk( s0.data() )) ) {
         s1  = new DiskStat( "" );
         sd  = new DiskStat( "" );
         *s1 = s0;
         s   = s1->data();
         _disksH[s] = s1;
         _disks.Insert( s1 );
         _tops.Insert( sd );
         s1->Pair( sd );
      }
      s1 = _disks[i];
      sd = _tops[i];

      DiskStat d( s0, *s1 );

      *s1 = s0;
      sd->SetTop( d, dd );
      i++;
   }
   if ( fp )
      ::fclose( fp );
   return *this;
}

void GLdiskStat::Dump()
{
   DiskStat *cd;
   int       i;
   char     *cp;
   char      buf[K];

   // Dump 'em

   printf( "device  rrqm/s  wrqm/s  r/s     w/s     rsec/s  wsec/s  " );
   printf( "%%read   %%write  await   %%util \n" );
   printf( "------  ------  ------  ------  ------  ------  ------  " );
   printf( "------  ------  ------  ------\n" );
   for ( i=0; i<nDsk(); i++ ) {
      cd  = _tops[i];
      cp  = buf;
      cp += sprintf( cp, "%-6s  ", cd->data() );
      cp += sprintf( cp, "%6d  ", (int)cd->_nRdM );
      cp += sprintf( cp, "%6d  ", (int)cd->_nWrM );
      cp += sprintf( cp, "%6d  ", (int)cd->_nRd );
      cp += sprintf( cp, "%6d  ", (int)cd->_nWr );
      cp += sprintf( cp, "%6d  ", (int)cd->_nRdSec );
      cp += sprintf( cp, "%6d  ", (int)cd->_nWrSec );
      cp += sprintf( cp, "%6.2f  ", cd->_nRdMs );
      cp += sprintf( cp, "%6.2f  ", cd->_nWrMs );
      cp += sprintf( cp, "%6.2f  ", cd->_wIoMs );
      cp += sprintf( cp, "%6.2f  ", cd->_nIoMs );
      cp += sprintf( cp, "\n" );
      ::fwrite( buf, strlen( buf ), 1, stdout );
      ::fflush( stdout );
   }
}


////////////////////////////////////////////////////////////////////////////
// 
//               c l a s s      F i l e S y s t e m
// 
////////////////////////////////////////////////////////////////////////////

static double _GB = 1.0 / 1073741824.0;

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
FileSystem::FileSystem( char *pFileSys, char *pMnt ) :
   string( pFileSys ),
   _mnt( pMnt )
{
   ::memset( &_st, 0, sizeof( _st ) );
}

FileSystem::~FileSystem()
{
}


////////////////////////////////////////////
// Access / Mutator
////////////////////////////////////////////
char *FileSystem::pFileSys()
{
   return (char *)data();
}

char *FileSystem::pMount() 
{
   return (char *)_mnt.data();
}

double FileSystem::PctUse()
{
   double num, den, rtn;

   den = _st.f_blocks * _st.f_frsize;
   num = den - ( _st.f_bfree * _st.f_bsize );
   rtn = ( den != 0.0 ) ? num / den : 0.0;
   return 100.0 * rtn;
}

double FileSystem::SizeGb()
{
   double rtn;

   rtn = _st.f_blocks * _st.f_frsize;
   return rtn *_GB;
}

double FileSystem::UsedGb()
{
   double rtn;

   rtn  = ( _st.f_blocks * _st.f_frsize );
   rtn -= ( _st.f_bfree * _st.f_bsize );
   return rtn * _GB;
}

double FileSystem::AvailGb()
{
   double rtn;

   rtn = _st.f_bfree * _st.f_bsize;
   return rtn * _GB; 
}

void FileSystem::Snap()
{
#if !defined(WIN32)
   struct statvfs st;

   if ( !::statvfs( pMount(), &st )  )
      _st = st;
#endif // !defined(WIN32)
}



////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L f i l e S y s S t a t
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLfileSysStat::GLfileSysStat( const char *mtab ) :
   string( mtab ),
   _fs()
{
   GLasciiFile a( (char *)mtab );
   char       *p1, *p2, *p3, *sp, *rp;
   bool        bOK;
   int         i, n;

   for ( i=0,n=a.GetAll(); i<n; i++ ) {
      string s( a[i]._data );

      // /dev/mapper/VolGroup00-LogVol05 /home ext3 rw 0 0

      sp = (char *)s.data();
      p1 = ::strtok_r( sp, _SEP, &rp );
      p2 = ::strtok_r( NULL, _SEP, &rp );
      p3 = ::strtok_r( NULL, _SEP, &rp );
      if ( !p2 || !p3 )
         continue; // for-i

      // Allowable file systems : xfs, ext, tmpfs

      bOK  = ( ::strstr( p3, "xfs" ) == p3 );
      bOK |= ( ::strstr( p3, "ext" ) == p3 );
      bOK |= ( ::strstr( p3, "tmpfs" ) == p3 );
      if ( bOK )
         _fs.Insert( new FileSystem( p1, p2 ) );
   }
}

GLfileSysStat::~GLfileSysStat()
{
   _fs.clearAndDestroy();
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
GLfileSysStat &GLfileSysStat::Snap()
{
   int i;

   for ( i=0; i<_fs.size(); _fs[i++]->Snap() );
   return *this;
}

void GLfileSysStat::Dump()
{
   FileSystem *fs;
   int         i;

   printf( "%-40s", "Filesystem" );
   printf( "Size  Used Avail Use%% Mounted on\n" );
   for ( i=0; i<_fs.size(); i++ ) {
      fs = _fs[i];
      printf( "%-40s%3.1fG %3.1fG %3.1fG %2d%% %s\n",
         fs->pFileSys(),
         fs->SizeGb(),
         fs->UsedGb(),
         fs->AvailGb(),
         (int)fs->PctUse(),
         fs->pMount() );
   }
}
