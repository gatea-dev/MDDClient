/******************************************************************************
*
*  EDG_version.cpp
*
*  (c) 1994-2017 Gatea Ltd.
*******************************************************************************/
#include <EDG_Internal.h>

char *librtEdgeID()
{
   static string s;
   static char   *sccsid = (char *)0;

   // Once

   if ( !sccsid ) {
      char bp[K], *cp;

      cp     = bp;
      cp    += sprintf( cp, "@(#)librtEdge %s Build %s ", GL64, _BLD );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}

void breakpointE() { ; }

