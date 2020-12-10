/******************************************************************************
*
*  version.cpp
*
*  (c) 1994-2019, Gatea Ltd.
******************************************************************************/
#include <MDDirect.h>

#if defined(__LP64__) || defined(_WIN64)
#define PTRSZ u_long
#define GL64 "(64-bit)"
#else
#define PTRSZ int
#define GL64 "(32-bit)"
#endif // __LP64__

char *MDDirectID()
{
   static string s;
   static char   *sccsid = (char *)0;

   // Once

   if ( !sccsid ) {
      char bp[K], *cp;

      cp     = bp;
      cp    += sprintf( cp, "@(#)MDDirect4py Build 3 " );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}


///////////////////////////////
// Utility Functions
///////////////////////////////

void breakpoint() { ; }

int strncpyz( char *dst, char *src, int bufSz )
{
   int sz;

   sz = gmin( bufSz-1, (int)strlen( src ) );
   ::memcpy( dst, src, sz );
   dst[sz] = '\0';
   return sz;
}

int atoin( char *str, int sz )
{
   char buf[K];

   strncpyz( buf, str, sz );
   return atoi( buf );
}
