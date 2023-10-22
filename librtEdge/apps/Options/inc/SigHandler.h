/******************************************************************************
*
*  Signal.h
*
*  REVISION HISTORY:
*     24 APR 2017 jcs  Created.
*      3 JUN 2018 jcs  Build 13: ../common
*
*  (c) 1994-2018, Gatea Ltd.
*******************************************************************************/
#include <signal.h>

// Locals

const char *_pSigLog = NULL;

/////////////////////////////////
// Support Functions
/////////////////////////////////
void _sigHandler( int num )
{
   switch( num ) {
      case SIGPIPE: 
         // Do nothing ...
         break;
      case SIGTERM: _pSigLog = "SIGTERM"; break;
      case SIGINT:  _pSigLog = "SIGINT";  break;
//      case SIGSEGV: _pSigLog = "SIGSEGV"; break;
      case SIGSEGV: abort(); break;
   }
}
void forkAndSig( bool bFork )
{
   int              i, pid;
   sigset_t         sigSet;
   struct sigaction s, lwc;
   static int       _sigs[] = { SIGPIPE, SIGTERM, SIGINT, SIGSEGV, 0 };

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

   for ( i=0; _sigs[i]; i++ ) {
      ::sigemptyset( &(s.sa_mask) );
      s.sa_handler = _sigHandler;
      s.sa_flags   = 0;
      ::sigaction( _sigs[i], &s, &lwc );
   }
}

