/******************************************************************************
*
*  config.c
*
*  REVISION HISTORY:
*      6 AUG 2009 jcs  Created.
*     17 AUG 2009 jcs  Build  2: strchr() bug
*      4 SEP 2010 jcs  Build  6: strchr(), not strtok()
*
*  (c) 1994-2010 Gatea Ltd.
******************************************************************************/
#include <string.h>
#include <stdio.h>

/*************************
 * Macros; Globals
 ************************/
#define K                    1024

/*************************
 * Config File Stuff
 ************************/
static char *_cfgKey[K];
static char *_cfgVal[K];

/*************************
 * Helper Functions
 ************************/
char *TrimChar( char *str, char trimch, char bLeading )
{
   char *cp, *np;

   cp = str;
   if ( bLeading )
      for ( ; *cp == trimch; cp++ );
   else {
      np = cp + strlen( cp ) - 1;
      for ( ; *np == trimch; *np='\0',np-- );
   }
   return cp;
}

void ParseConfigFile( char *pCfg )
{
   FILE *fp;
   char  sLine[2*K], *cp, *mp;
   char *p1, *p2;
   int   nc;

   /*
    * 1) Pull each line from file
    * 2) Chop CR / LF
    * 3) Scan characters AFTER the comment '#' character
    */
   if ( !(fp=fopen( pCfg, "r" )) )
      return;
   for ( nc=0; fp && fgets( (cp=sLine), 2*K, fp ); ) {
      cp += ( strlen( sLine ) - 1 );
      if ( ( *cp == 0x0d ) || ( *cp == 0x0a ) )
         *cp = '\0';
      if ( (mp=strchr( sLine, '#' )) )
         *mp = '\0';
      if ( !strlen( sLine ) )
         continue; /* for-loop */
/*
      p1 = strtok( sLine, "=" );
      p2 = strtok( NULL, "=" );
 */
      if ( !(p1=strchr( sLine, '=' )) )
         continue;
      *p1 = '\0';
      p2  = p1+1;
      p1  = sLine;
      p1          = TrimChar( TrimChar( p1, ' ', 1 ), ' ', 0 ); 
      p2          = TrimChar( TrimChar( p2, ' ', 1 ), ' ', 0 ); 
      _cfgKey[nc] = strdup( p1 );
      _cfgVal[nc] = strdup( p2 );
      nc         += 1;
   }
   fclose( fp ); 
}

char *GetCfgParam( char *pCfg )
{
   int i;

   for ( i=0; _cfgKey[i]; i++ ) {
      if ( !strcmp( _cfgKey[i], pCfg ) )
         return _cfgVal[i];
   }
   return (char *)0;
}

int GetCsvList( char *pCfg, char **rtn )
{
   char        *cp, sVal[8*K];
   int          r;
   static char *_sep = ",";

   /* 1) Find and copy the value */

   if ( !(cp=GetCfgParam( pCfg )) )
      return 0;
   strcpy( sVal, cp );

   /* 2) Tokenize and copy the results */

   for ( r=0,(cp=strtok( sVal, _sep )); cp; r++ ) {
      rtn[r] = strdup( cp );
      cp     = strtok( NULL, _sep );
   }
   return r;
}
