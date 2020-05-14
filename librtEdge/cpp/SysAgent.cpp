/******************************************************************************
*
*  SysAgent.cpp
*     Connectionless publisher agent to monitor CPU and Disk stats
*
*  REVISION HISTORY:
*     13 JUL 2017 jcs  Created.
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

// Hard-coded

static int _nameFid =    3;
static int _cpuFid  =    6;
static int _seqFid  = 1021; // SEQNUM

using namespace RTEDGE;

// Locals

const char *_pSigLog = NULL;


/////////////////////////////////
// Support Functions
/////////////////////////////////
#if !defined(WIN32)
#include <assert.h>
#include <errno.h>
#include <signal.h>

void forkAndSig( bool bFork, void (*handler)( int ), int *sigs )
{
   int              i, pid;
   sigset_t         sigSet;
   struct sigaction s, lwc;

   // fork() if !StdOut

   if ( bFork ) {
      if ( (pid=::fork()) < 0 ) { // Error
         ::fprintf( stdout, "\nfork() ERROR:  %s\n", ::strerror( errno ) );
         exit( 1 );
      }
      else if ( pid > 0 )   // Parent
         exit( 0 );

      // Child:
      // 1) Ignore terminal stop signals (BSD).
      // 2) Process is session leader
      // 3) Disassociate from process group
      // 4) Change current dir to file system root so
      //    current filesystem may be unmounted.
      // 5) Clear file mode creation mask
      // 6) Ignore SIGCLD, allowing dead children to bypass
      //    zombie state and free up process table slot.

      int ttSigs[] = { SIGTTOU, SIGTTIN, SIGTSTP, 0 };

      ::sigemptyset( &sigSet );
      for ( i=0; ttSigs[i]; ::sigaddset( &sigSet, ttSigs[i++] ) );
      ::sigprocmask( SIG_UNBLOCK, &sigSet, (sigset_t *)0 );
      ::setpgrp();
/*
      ::chdir( "/" );
      ::umask( 0 );
 */
      ::sigemptyset( &sigSet );
      ::sigaddset( &sigSet, SIGHUP );
      ::sigaddset( &sigSet, SIGCLD );
      ::sigprocmask( SIG_UNBLOCK, &sigSet, (sigset_t *)0 );

      // Really get rid of controlling terminal

      if ( (pid=::fork()) < 0 ) { // Error
         ::fprintf( stdout, "\nfork() ERROR:  %s\n", ::strerror( errno ) );
         exit( 1 );
      }
      else if ( pid > 0 ) {       // Parent
         ::fprintf( stdout, "\nChild PID = %d\n", pid );
         exit( 0 );
      }
   }

   // Signal handlers

   for ( i=0; sigs[i]; i++ ) {
      ::sigemptyset( &(s.sa_mask) );
      s.sa_handler = handler;
      s.sa_flags   = 0;
      ::sigaction( sigs[i], &s, &lwc );
   }
}

void sigHandler( int num )
{
   switch( num ) {
      case SIGTERM: _pSigLog = "SIGTERM"; break;
      case SIGINT:  _pSigLog = "SIGINT";  break;
//      case SIGSEGV: _pSigLog = "SIGSEGV"; break;
      case SIGSEGV: assert( 0 ); break;
   }
}

#endif // !defined(WIN32)

class MyChannel : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   Mutex      _mtx;
   CPUStats  *_cs;
   DiskStats *_ds;
   int        _RTL;
   char       _host[K];

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyChannel( const char *pPub, bool bCPU ) :
      PubChannel( pPub ),
      _mtx(),
      _cs( (CPUStats *)0 ),
      _ds( (DiskStats *)0 ),
      _RTL( 1 )
   {
      strcpy( _host, "Undefined" );
      if ( bCPU )
         _cs = new CPUStats();
      else
         _ds = new DiskStats();
   }

   ~MyChannel()
   {
      if ( _cs )
         delete _cs;
      if ( _ds )
         delete _ds;
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
public:
   char *SetHost()
   {
      /*
       * WIN32 requires WinSock to load
       * Call this AFTER Start() or StartConnectionless()
       */
      ::gethostname( _host, sizeof( _host ) );
      return _host;
   }

   int PublishAll()
   {
      int nr;

      if ( _cs ) nr = _Publish( *_cs );
      else       nr = _Publish( *_ds );
      _RTL++;
      return nr;
   }

   int _Publish( CPUStats &cs )
   {
      Locker      lck( _mtx );
      Update     &u = upd();
      ::OSCpuStat c;
      char        tkr[K];
      bool        bImg;
      int         i, id, n, fid;

      n    = cs.Snap();
      bImg = ( _RTL == 1 );
      for ( i=0; i<n; i++ ) {
         id = i+1;
         sprintf( tkr, "%s.%d", _host, i );
         u.Init( tkr, (void *)id, bImg );
         c   = cs.Get( i );
         fid = _cpuFid;
         u.AddField( fid++, c._us );
         u.AddField( fid++, c._sy );
         u.AddField( fid++, c._ni );
         u.AddField( fid++, c._id );
         u.AddField( fid++, c._wa );
         u.AddField( fid++, c._si );
         u.AddField( fid++, c._st );
         u.AddField( _seqFid, _RTL );
         u.Publish();
      }
      return n;
   }

   int _Publish( DiskStats &ds )
   {
      Locker       lck( _mtx );
      Update      &u = upd();
      ::OSDiskStat d;
      char         tkr[K];
      bool         bImg;
      int          i, id, n, fid;

      n    = ds.Snap();
      bImg = ( _RTL == 1 );
      for ( i=0; i<n; i++ ) {
         id = i+1;
         sprintf( tkr, "%s.%d", _host, i );
         u.Init( tkr, (void *)id, bImg );
         d   = ds.Get( i );
         fid = _cpuFid;
         u.AddField( _nameFid, d._diskName );
         u.AddField( fid++, d._nRdSec );
         u.AddField( fid++, d._nWrSec );
         u.AddField( fid++, d._nIoMs );
         u.AddField( _seqFid, _RTL );
         u.Publish();
      }
      return n;
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      ::fprintf( stdout, "CONN %s : %s\n", pUp, msg );
      ::fflush( stdout );
   }

   virtual void OnSymListQuery( int nSym )
   {
      ::fprintf( stdout, "OnSymListQuery( %d )\n", nSym );
      PublishAll();
   }

   virtual void OnRefreshImage( const char *tkr, void *arg )
   {
      ::fprintf( stdout, "OnRefreshImage( %s )\n", tkr );
      ::fflush( stdout );
#ifdef TODO_REFRESH
      if ( (it=_wl.find( s )) != _wl.end() ) {
         w = (*it).second;
         PubTkr( *w );
      }
#endif // TODO_REFRESH
   }


   ////////////////////////////////////
   // PubChannel Interface
   ////////////////////////////////////
protected:
   virtual Update *CreateUpdate()
   {
      return new Update( *this );
   }

}; // class MyChannel


/////////////////
// Helpers
/////////////////
void ProgressBar( int i )
{
   if ( i && !(i%100) )
      printf( "%d", i/100 );
   else if ( i && !(i%50) )
      printf( "+" );
   else if ( i && !(i%10) )
      printf( "."  );
   fflush( stdout );
}


////////////////////////////////////////////
// Do it baby!!
////////////////////////////////////////////
int main( int argc, char **argv )
{
   const char *pSvr, *pPub, *pc;
   double      tSlp;
   int         i;
   bool        bCPU;
#if !defined(WIN32) 
   static int  sigs[] = { SIGTERM, SIGINT, SIGSEGV, 0 };
#endif // !defined(WIN32) 

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }

   // cmd-line args

   if ( argc < 4 ) {
      pc = "Usage: %s <hosts> <Svc> <pubTmr> [<bDisk>]";
      printf( pc, argv[0] );
      printf( "; Exitting ...\n" );
      return 0;
   }
   pSvr = argv[1];
   pPub = argv[2];
   tSlp = atof( argv[3] );
   bCPU = ( argc >= 4 );

   MyChannel   pub( pPub, bCPU );

   ::fprintf( stdout, "%s\n", pub.Version() );
   ::fprintf( stdout, "%s\n", pub.StartConnectionless( pSvr ) );
   ::fprintf( stdout, "Publish every %.1fs for %s\n", tSlp, pub.SetHost() );
   ::fflush( stdout );
#if !defined(WIN32) 
   forkAndSig( false, sigHandler, sigs );
#endif // !defined(WIN32) 
   for ( i=0; !_pSigLog; i++ ) {
      pub.Sleep( tSlp );
      pub.PublishAll();
      ProgressBar( i );
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub.Stop();
   printf( "Done!!\n " );
   return 1;
}
