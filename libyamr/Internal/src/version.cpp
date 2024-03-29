/******************************************************************************
*
*  version.cpp
*
*  (c) 1994-2017 Gatea Ltd.
*******************************************************************************/
#include <Internal.h>

char *libyamrID()
{
   static string s;
   static char   *sccsid = (char *)0;

   // Once

   if ( !sccsid ) {
      char bp[K], *cp;

      cp     = bp;
      cp    += sprintf( cp, "@(#)libyamr %s Build 3 ", GL64 );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}

