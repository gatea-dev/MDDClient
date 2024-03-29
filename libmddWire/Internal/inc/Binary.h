/******************************************************************************
*
*  Binary.h
*     MD-Direct binary data
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created
*      5 DEC 2013 jcs  Build  4: UNPACKED_BINFLD
*      4 JUN 2014 jcs  Build  7: mddFld_real / mddFld_bytestream
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*     29 MAR 2022 jcs  Build 13: Binary._bPackFlds
*      1 NOV 2022 jcs  Build 16: _GetVector() / _SetVector(); _wireMult()
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __MDD_BINARY_H
#define __MDD_BINARY_H
#include <MDW_Internal.h>
#include <BinProtocol.h>

////////////////////////
// Forward declarations
////////////////////////

/////////////////////////////////
// Endian-ness
/////////////////////////////////
namespace MDDWIRE_PRIVATE
{
class Endian
{
protected:
	bool _bBig;

	// Constructor / Destructor
protected:
	Endian();
	virtual ~Endian();

	// Operations

	float  _swapf( float );
	double _swapd( double );

	// Class-wide
protected:
	static bool _IsBig();

}; // class Endian


/////////////////////////////////////////
// Binary data
/////////////////////////////////////////
class Binary : public Endian
{
public:
	static double _dMidNt;
	static double _ymd_mul;

private:
	bool      _bPackFlds;
	mddBldBuf _vBuf;

	// Constructor / Destructor
public:
	Binary( bool );
	~Binary();

	// Access / Operations
public:
	double MsgTime( mddBinHdr & );

	// Wire Protocol - Get
public:
	int Get( u_char *, mddBinHdr & );
	int Get( u_char *, mddField & );
	int Get( u_char *, mddBuf & );
	int Get( u_char *, u_char & );
	int Get( u_char *, u_short & );
	int Get( u_char *, u_int &, bool bUnpacked=false );
	int Get( u_char *, u_int64_t &, bool bUnpacked=false );
	int Get( u_char *, float & );
	int Get( u_char *, double &, double );
	int Get( u_char *, mddReal & );

	// Wire Protocol - Set

	int Set( u_char *, mddBinHdr &, bool bLenOnly=false );
	int Set( u_char *, mddField );
	int Set( u_char *, mddBuf );
	int Set( u_char *, u_char );
	int Set( u_char *, u_short );
	int Set( u_char *, u_int, bool bUnpacked=false );
	int Set( u_char *, u_int64_t );
	int Set( u_char *, float );
	int Set( u_char *, double, double );
	int Set( u_char *, mddReal );

	// Wire Protocol - Unpacked
protected:
	int _Get_unpacked( u_char *, mddField &, bool ); 
	int _Set_unpacked( u_char *, mddField ); 

	// Helpers
private:
	u_int  _TimeNow();
	double _tMidNt( double );
	int    _u_unpack( u_char *, u_int & );
	int    _u_unpack( u_char *, u_int64_t & /* , bool */ );
	int    _u_pack( u_char *, u_int );
	int    _u_pack( u_char *, u_int64_t, bool & );
	mddBuf _GetVector( mddBuf & );
	mddBuf _SetVector( mddBuf &, char );
	double _wireMult( char, bool );

}; // class Binary

}  // namespace MDDWIRE_PRIVATE

#endif // __MDD_BINARY_H
