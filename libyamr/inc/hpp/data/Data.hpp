/******************************************************************************
*
*  Data.hpp
*     libyamr Data Marshall / De-marshall base Classes
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_Data_H
#define __YAMR_Data_H
#include <hpp/Reader.hpp>
#include <hpp/Writer.hpp>

#ifndef DOXYGEN_OMIT

/*
 * Packing : Max sizes
 */
#define _MAX_PACK8  255
#define _MAX_PACK16 65535
#define _MAX_FIELDS _MAX_PACK16
/*
 * Protocols used by Library
 */
#define _PROTO_STRINGDICT 0x8000 // YAMR::StringDict
#define _PROTO_STRINGLIST 0x8001 // YAMR::Data::StringList
#define _PROTO_STRINGMAP  0x8002 // YAMR::Data::StringMap
#define _PROTO_STRINGTBL  0x8003 // YAMR::Data::StringTable

#define _PROTO_INTLIST8   0x8010 // YAMR::Data::IntList : Max < _MAX_PACK8
#define _PROTO_INTLIST16  0x8011 // YAMR::Data::IntList : Max < _MAX_PACK16
#define _PROTO_INTLIST32  0x8012 // YAMR::Data::IntList

#define _PROTO_FLOATLIST  0x8020 // YAMR::Data::FloatList : 4 sigFig
#define _PROTO_DOUBLELIST 0x8021 // YAMR::Data::DoubleList : 10 sigFig
#define _PROTO_FIELDLIST  0x8022 // YAMR::Data::FieldList

#endif // DOXYGEN_OMIT

namespace YAMR
{

namespace Data
{

/** \brief String Table (map) */
typedef std::map<std::string, std::string> StringHashMap;
/** \brief String List */
typedef std::vector<std::string>           Strings;
/** \brief Integer List */
typedef std::vector<u_int32_t>             Ints;
/** \brief Double List */
typedef std::vector<double>                Doubles;
/** \brief Float List */
typedef std::vector<float>                 Floats;

} // namespace Data

} // namespace YAMR



////////////////////////////////////////
//
// Structured data types
//
////////////////////////////////////////
#include <hpp/data/Double/DoubleList.hpp>
#include <hpp/data/Field/FieldList.hpp>
#include <hpp/data/Float/FloatList.hpp>
#include <hpp/data/Int/IntList.hpp>
#include <hpp/data/String/StringDict.hpp>
#include <hpp/data/String/StringList.hpp>
#include <hpp/data/String/StringMap.hpp>

#endif // __YAMR_Data_H 
