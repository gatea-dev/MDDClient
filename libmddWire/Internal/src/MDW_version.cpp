/******************************************************************************
*
*  MDW_version.cpp
*
*  (c) 1994-2012 Gatea Ltd.
*******************************************************************************/
#include <MDW_Internal.h>

#if defined(__LP64__) || defined(_WIN64)
#define PTRSZ u_long
#define GL64 "(64-bit)"
#else
#define PTRSZ int
#define GL64 "(32-bit)"
#endif // __LP64__

char *libmddWireID()
{
   static string s;
   static char   *sccsid = (char *)0;

   // Once

   if ( !sccsid ) {
      char bp[K], *cp;

      cp     = bp;
      cp    += sprintf( cp, "@(#)libmddWire %s Build %s ", GL64, _MDW_LIB_BLD );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}
