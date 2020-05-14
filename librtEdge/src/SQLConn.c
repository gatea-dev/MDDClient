/******************************************************************************
*
*  SQLConn.c
*     ODBC connectivity stuff
*
*  REVISION HISTORY:
*     27 SEP 2010 jcs  Created (from GLodbcDb.cpp)
*
*  (c) 1994-2010, Gatea Ltd.
******************************************************************************/
#ifdef WIN32
#include <windows.h>
#include <windowsx.h>
#endif /* WIN32 */
#include <sql.h>
#include <sqlext.h>
#include <librtEdge.h>

#ifdef WIN32
#define SQLLEN  SQLINTEGER
#define SQLULEN u_long
#else
#define SQL_API
#endif // WIN32

/* Locals */

static const char *_dsn   = (const char *)0;
static const char *_uid   = (const char *)0;
static const char *_pwd   = (const char *)0;
static SQLHDBC     _hdbc  = (SQLHDBC)0;
static SQLHSTMT    _hstmt = (SQLHSTMT)0;
static SQLHENV     _henv  = (SQLHENV)0;

/* Forwards */

static char  IsOK( SQLRETURN );
static char *pError( SQLHANDLE, SQLSMALLINT, char * );


/*******************************************
 * Initialization
 *******************************************/
void SQL_Init( const char *dsn, const char *uid, const char *pwd )
{
   SQLRETURN  rc;
   SQLINTEGER err;
   SQLPOINTER ver =(SQLPOINTER)SQL_OV_ODBC3;

   /* Store ODBC connection stuff */

   _dsn = dsn;
   _uid = uid;
   _pwd = pwd;

   /* Allocate ODBC parameters */

   rc = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_henv );
   if ( !IsOK( rc ) )
      return;
   rc = SQLSetEnvAttr( _henv, SQL_ATTR_ODBC_VERSION, ver, 0 );
   if ( !IsOK( rc ) )
      return;
   rc  = SQLAllocHandle( SQL_HANDLE_DBC, _henv, &_hdbc );
   if ( !IsOK( rc ) ) {
      _hdbc = (SQLHDBC)0;
      return;
   }
}

void SQL_Destroy()
{
   if ( _hstmt )
      SQLFreeHandle( SQL_HANDLE_STMT, _hstmt );
   if ( _hdbc )
      SQLFreeHandle( SQL_HANDLE_DBC, _hdbc );
   if ( _henv )
      SQLFreeHandle( SQL_HANDLE_ENV, _henv );
   _hdbc  = (SQLHDBC)0;
   _hstmt = (SQLHSTMT)0;
   _henv  = (SQLHENV)0;
}



/*******************************************
 * Connect / Insert
 *******************************************/
char SQL_Connect()
{
   char      *pErr;
   long       tTmout;
   SQLINTEGER sLen;
   SQLRETURN  rc;
   SQLINTEGER err;

   rc  = SQLConnect( _hdbc, 
                     (SQLCHAR *)_dsn, SQL_NTS,
                     (SQLCHAR *)_uid, SQL_NTS,
                     (SQLCHAR *)_pwd, SQL_NTS );
   if ( IsOK( rc ) ) {
      printf( "ODBC connection to %s UP\n", _dsn );
      rc  = SQLAllocHandle( SQL_HANDLE_STMT, _hdbc, &_hstmt );
      if ( !IsOK( rc ) ) {
          pErr = (char *)"TODO"; /* pError( _hstmt, SQL_HANDLE_STMT, rc ); */
          printf( "SQLAllocHandle( STMT ) error : %s\n", pErr );
         _hstmt = (SQLHSTMT)0;
      }
   }
   else {
      pErr = (char *)"TODO"; /* pError( _hdbc, SQL_HANDLE_DBC, rc ); */
      printf( "ODBC connection to %s DOWN : %s\n", _dsn, pErr );
   }
   return 1;
}

char SQL_Insert( char *tbl, char **cols, char **vals )
{
   int        i, nc, nv;
   SQLRETURN  rc;
   char       sIns[8*K];  /* Assume INSERT > 8K */
   char      *cp, *pErr, sErr[K];

   /* Pre-condition(s) */

   for ( nc=0; cols[nc]; nc++ );
   for ( nv=0; vals[nv]; nv++ );
   if ( !_hstmt )
      return 0;
   if ( !nc || ( nc != nv ) )
      return 0;

   /* Build INSERT statement */

   cp  = sIns;
   cp += sprintf( cp, "INSERT into %s ( ", tbl );
   for ( i=0; i<nc; i++ ) {
      cp += i ? sprintf( cp, ", " ) : 0;
      cp += sprintf( cp, cols[i] );
   }
   cp += sprintf( cp, " ) VALUES ( " );
   for ( i=0; i<nv; i++ ) {
      cp += i ? sprintf( cp, ", " ) : 0;
      cp += sprintf( cp, "\"%s\"", vals[i] );
   }
   cp += sprintf( cp, " );\n" );

   /* Shove it in */

   // Else us ...
printf( sIns );
   if ( _hstmt && nc && ( nc == nv ) ) {
      rc = SQLPrepare( _hstmt, (SQLCHAR *)sIns, SQL_NTS );
      if ( IsOK( rc ) ) {
         rc = SQLExecute( _hstmt );
         if ( !IsOK( rc ) ) {
            pErr = pError( _hstmt, SQL_HANDLE_STMT, sErr );
            printf( "%s\n", pErr );
         }
         return IsOK( rc );
      }
   }
   return 0;
}



/*******************************************
 * Helpers
 *******************************************/
static char IsOK( SQLRETURN rc )
{
   switch( rc ) {
      case SQL_SUCCESS:
      case SQL_SUCCESS_WITH_INFO:
         return 1;
      case SQL_INVALID_HANDLE:
      case SQL_ERROR:
         return 0;
   }
   return 0;
}

static char *pError( SQLHANDLE   hSQL, 
                     SQLSMALLINT ty, 
                     char       *rtn )
{
   SQLCHAR     szState[1024], szMsg[1024];
   SQLINTEGER  err;
   SQLSMALLINT mLen;

   SQLGetDiagRec( ty, hSQL, 1, szState, &err, szMsg, 1024, &mLen );
   strcpy( rtn, (char *)szMsg );
   return rtn;
}

