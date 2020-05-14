/******************************************************************************
*
*  EDG_GLmd5.h
*     Wrapper around MD5 RSA Data Security Algorithm
*
*  REVISION HISTORY:
*      2 JUL 2015 jcs  librtEdge
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
*******************************************************************************/
#ifndef __EDGLIB_GLMD5_H
#define __EDGLIB_GLMD5_H
#include <EDG_Internal.h>

/*
 ***********************************************************************
 ** md5.h -- header file for implementation of MD5                    **
 ** RSA Data Security, Inc. MD5 Message-Digest Algorithm              **
 ** Created: 2/17/90 RLR                                              **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version               **
 ** Revised (for MD5): RLR 4/27/91                                    **
 ***********************************************************************
 */

/*
 ***********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.  **
 **                                                                   **
 ** License to copy and use this software is granted provided that    **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message-     **
 ** Digest Algorithm" in all material mentioning or referencing this  **
 ** software or this function.                                        **
 **                                                                   **
 ** License is also granted to make and use derivative works          **
 ** provided that such works are identified as "derived from the RSA  **
 ** Data Security, Inc. MD5 Message-Digest Algorithm" in all          **
 ** material mentioning or referencing the derived work.              **
 **                                                                   **
 ** RSA Data Security, Inc. makes no representations concerning       **
 ** either the merchantability of this software or the suitability    **
 ** of this software for any particular purpose.  It is provided "as  **
 ** is" without express or implied warranty of any kind.              **
 **                                                                   **
 ** These notices must be retained in any copies of any part of this  **
 ** documentation and/or software.                                    **
 ***********************************************************************
 */
#ifndef _md5_h
#define _md5_h
extern "C" {
#include <sys/types.h>

/* typedef a 32-bit type */
typedef unsigned int UINT4;

/* Data structure for MD5 (Message-Digest) computation */
typedef struct
  {
    UINT4 i[2];			/* number of _bits_ handled mod 2^64 */
    UINT4 buf[4];		/* scratch buffer */
    unsigned char in[64];	/* input buffer */
    unsigned char digest[16];	/* actual digest after MD5Final call */
  }
MD5_CTX;

extern void MD5Init( MD5_CTX * );
extern void MD5Update( MD5_CTX *, u_char *, u_int );
extern void MD5Final( MD5_CTX * );

/*
 ***********************************************************************
 ** End of md5.h                                                      **
 ******************************** (cut) ********************************
 */
}

#endif				/*!_md5_h */
/////////////////////////////////////////
// MD5 Wrapper
/////////////////////////////////////////
namespace RTEDGE_PRIVATE
{
class GLmd5
{
private:
	MD5_CTX _ctx;

	// Constructor / Destructor
public:
	GLmd5();
	~GLmd5();

	// Access

	u_char *operator()();
	u_char *digest();

	// Mutator

	void update( char *, int );

	// Helpers (From md5.cpp) 
private:
	void MD5Init();
	void MD5Update( u_char *, u_int );
	void MD5Final();
	void Transform( UINT4 *, UINT4 * );
};

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_GLMD5_H
