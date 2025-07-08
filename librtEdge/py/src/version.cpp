/******************************************************************************
*
*  version.cpp
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <MDDirect.h>
#include <bld.cpp>

// Platform-specific

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
      cp    += sprintf( cp, "@(#)MDDirect4py %s Build %s ", GL64, _MDD_LIB_BLD );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      cp    += sprintf( cp, "\n" );
      cp    += sprintf( cp, "%s", ::Py_GetVersion() );
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


PyObject *mdd_PyList_Pack2( PyObject *py1, PyObject *py2 )
{
   PyObject *rc;

   rc = ::PyList_New( 2 );
   ::PyList_SetItem( rc, 0, py1 );
   ::PyList_SetItem( rc, 1, py2 );
   return rc;
}

PyObject *mdd_PyList_Pack3( PyObject *py1, PyObject *py2, PyObject *py3 )
{
   PyObject *rc;

   rc = ::PyList_New( 3 );
   ::PyList_SetItem( rc, 0, py1 );
   ::PyList_SetItem( rc, 1, py2 );
   ::PyList_SetItem( rc, 2, py3 );
   return rc;
}

PyObject *mdd_PyList_Pack4( PyObject *py1, 
                            PyObject *py2, 
                            PyObject *py3,
                            PyObject *py4 )
{
   PyObject *rc;

   rc = ::PyList_New( 4 );
   ::PyList_SetItem( rc, 0, py1 );
   ::PyList_SetItem( rc, 1, py2 );
   ::PyList_SetItem( rc, 2, py3 );
   ::PyList_SetItem( rc, 3, py4 );
   return rc;
}

PyObject *_PyReturn( PyObject *obj )
{
   Py_INCREF( obj );
   return obj;
}

const char *_Py_GetString( PyObject *pyo, string &rc )
{
#if PY_MAJOR_VERSION >= 3
   Py_ssize_t sz;
   wchar_t   *pw = ::PyUnicode_AsWideCharString( pyo, &sz );
   wstring    wc( pw );
   string     ss( wc.begin(), wc.end() );

   rc = ss.data();
   ::PyMem_Free( pw );
#else
   rc = ::PyString_AsString( pyo );
#endif // PY_MAJOR_VERSION >= 3
   return rc.data();
}
