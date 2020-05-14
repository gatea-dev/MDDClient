/******************************************************************************
*
*  OS_cpu.h
*
*  REVISION HISTORY:
*     23 OCT 2002 jcs  Created.
*     13 JUL 2017 jcs  librtEdge
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#ifndef __EDG_CPU_H
#define __EDG_CPU_H
#include <EDG_Internal.h>
#ifdef WIN32
#include <PDH.H>
#include <WCHAR.H>
#define PDH_HCOUNTER HCOUNTER
#define PDH_HQUERY   HQUERY
#else
#include <sys/times.h>
#include <limits.h>
#define PDH_HCOUNTER HANDLE
#define PDH_HQUERY   HANDLE
#define PDH_STATUS   long
#endif // WIN32


// Templatized Collections

#define GLvecPdhCntr  GLvector<PDH_HCOUNTER>
#define GLvecInt      GLvector<int>

////////////////////////
// CPU Time
////////////////////////
namespace RTEDGE_PRIVATE
{
class GLcpuTime
{
public:
	static bool _bEnable;
private:
	bool       _bOverride;
#ifdef WIN32
	HANDLE     _hProc;
	FILETIME   _tSys;
	FILETIME   _tUsr;
#else
	struct tms _cpu;
#endif // WIN32
	int        _nCpu;
	string    *_pCpu; 
public:
	double _dSys;
	double _dUsr;

	// Constructor / Destructor
public:
	GLcpuTime( bool bOverride=false );
	~GLcpuTime();

	// Access
public:
	int    nCpu();
	char  *pCpu();
	double dUsr();
	double dSys();
	double dCpu();

	// Operations

	double operator()();
	double dElapsed( GLcpuTime *c0=(GLcpuTime *)0 );

	// Helpers
private:
	void InitCPU();
	void CalcUsr();
	void CalcSys();

	// Class-wide
public:
	static int  MemSize( GLvecInt &, int pid=0 );
	static bool KillProcess( char * );
};

////////////////////////
// /proc/stat
////////////////////////
class CPUstat : public ::OSCpuStat
{
	// Constructor / Destructor
public:
	CPUstat( char * );
	CPUstat( CPUstat &, CPUstat & );

	// Mutator

	void SetTop( const CPUstat &, double );

	// Assignment Operators

	CPUstat &operator=( const CPUstat & );
};

#define GLvecCPUstat  GLvector<CPUstat *>
 
class GLprocStat : public string
{
private:
	GLvecCPUstat _cpus;
	GLvecCPUstat _tops;
	double       _d0;
	PDH_HQUERY   _qry;
	GLvecPdhCntr _UserTime;
	GLvecPdhCntr _PrivilegedTime;
	GLvecPdhCntr _ProcessorTime;
	GLvecPdhCntr _IdleTime;
	GLvecPdhCntr _InterruptTime;

	// Constructor / Destructor
public:
	GLprocStat( const char *pFile="/proc/stat" );
	~GLprocStat();

	// Access / Operations
public:
	GLvecCPUstat &cpus();
	GLvecCPUstat &tops();
	int           nCpu();
	int           snap( bool bDump=false );

	// Helpers
private:
	PDH_HCOUNTER _AddCounter( const char *, int );
	double       _GetCounter( PDH_HCOUNTER );
};

}  // namespace RTEDGE_PRIVATE

#endif // __EDG_CPU_H
