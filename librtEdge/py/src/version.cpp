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
      cp    += sprintf( cp, "@(#)MDDirect4py Build 5 " );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      cp    += sprintf( cp, "\n" );
      cp    += sprintf( cp, ::Py_GetVersion() );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}


///////////////////////////////
// Utility Functions
///////////////////////////////

void m_breakpoint() { ; }

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


///////////////////////////////
// Stupid, but effective
///////////////////////////////
PyObject *PyList_Pack2( PyObject *o0, PyObject *o1 )
{
   PyObject *rc = ::PyList_New( 2 );

   ::PyList_SetItem( rc, 0, o0 );
   ::PyList_SetItem( rc, 1, o1 );
   return rc;
}

PyObject *PyList_Pack3( PyObject *o0, PyObject *o1, PyObject *o2 )
{
   PyObject *rc = ::PyList_New( 3 );

   ::PyList_SetItem( rc, 0, o0 );
   ::PyList_SetItem( rc, 1, o1 );
   ::PyList_SetItem( rc, 2, o2 );
   return rc;
}

PyObject *PyList_Pack7( PyObject *o0, PyObject *o1, PyObject *o2, 
                        PyObject *o3, PyObject *o4, PyObject *o5, 
                        PyObject *o6 )
{
   PyObject *rc = ::PyList_New( 7 );

   ::PyList_SetItem( rc, 0, o0 );
   ::PyList_SetItem( rc, 1, o1 );
   ::PyList_SetItem( rc, 2, o2 );
   ::PyList_SetItem( rc, 3, o3 );
   ::PyList_SetItem( rc, 4, o4 );
   ::PyList_SetItem( rc, 5, o5 );
   ::PyList_SetItem( rc, 6, o6 );
   return rc;
}
