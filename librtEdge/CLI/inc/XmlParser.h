/******************************************************************************
*
*  XmlParser.h
*
*  REVISION HISTORY:
*      3 NOV 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#endif // DOXYGEN_OMIT

namespace librtEdge
{


////////////////////////////////////////////////
//
//       c l a s s   X m l E l e m
//
////////////////////////////////////////////////
/**
 * \class XmlElem
 * \brief A parsed named XML element including all elements and attributes
 */
public ref class XmlElem : public librtEdge::rtEdge
{
private: 
	RTEDGE::XmlElem *_cpp;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/** \brief Constructor  */
	XmlElem( RTEDGE::XmlElem *cpp ) :
	   _cpp( cpp )
	{ ; }


	//////////////////////////////
	// Elements
	//////////////////////////////
	/**
	 * \brief Return element value as string, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as string, returning default if not found
	 */
	String ^getElemValue( String ^prop, String ^dflt )
	{
	   const char *rc = _cpp->getElemValue( _pStr( prop ), _pStr( dflt ) );

	   return gcnew String( rc );
	}

	/**
	 * \brief Return element value as bool, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as bool, returning default if not found
	 */
	bool getElemValue( String ^prop, bool dflt )
	{
	   return _cpp->getElemValue( _pStr( prop ), dflt );
	}

	/**
	 * \brief Return element value as int, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as int, returning default if not found
	 */
	int getElemValue( String ^prop, int dflt )
	{
	   return _cpp->getElemValue( _pStr( prop ), dflt );
	}

	/**
	 * \brief Return element value as 64-bit int, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as 64-bit int, returning default if not found
	 */
	long long getElemValue( String ^prop, long long dflt )
	{
	   return _cpp->getElemValue( _pStr( prop ), (u_int64_t)dflt );
	}

	/**
	 * \brief Return element value as double, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as double, returning default if not found
	 */
	double getElemValue( String ^prop, double dflt )
	{
	   return _cpp->getElemValue( _pStr( prop ), dflt );
	}

	//////////////////////////////
	// Elements
	//////////////////////////////
	/**
	 * \brief Return attribute value as string, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as string, returning default if not found
	 */
	String ^getAttrValue( String ^prop, String ^dflt )
	{
	   const char *rc = _cpp->getAttrValue( _pStr( prop ), _pStr( dflt ) );

	   return gcnew String( rc );
	}

	/**
	 * \brief Return attribute value as bool, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as bool, returning default if not found
	 */
	bool getAttrValue( String ^prop, bool dflt )
	{
	   return _cpp->getAttrValue( _pStr( prop ), dflt );
	}

	/**
	 * \brief Return attribute value as int, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as int, returning default if not found
	 */
	int getAttrValue( String ^prop, int dflt )
	{
	   return _cpp->getAttrValue( _pStr( prop ), dflt );
	}

	/**
	 * \brief Return attribute value as 64-bit int, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as 64-bit int, returning default if not found
	 */
	long long getAttrValue( String ^prop, long long dflt )
	{
	   return _cpp->getAttrValue( _pStr( prop ), (u_int64_t)dflt );
	}

	/**
	 * \brief Return attribute value as double, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as double, returning default if not found
	 */
	double getAttrValue( String ^prop, double dflt )
	{
	   return _cpp->getAttrValue( _pStr( prop ), dflt );
	}

}; // class XmlElem


////////////////////////////////////////////////
//
//       c l a s s   X m l P a r s e r
//
////////////////////////////////////////////////
/**
 * \class XmlParser
 * \brief A parsed XML document read from a file
 */
public ref class XmlParser : public librtEdge::rtEdge
{
private: 
	RTEDGE::XmlParser *_cpp;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/** \brief Constructor  */
	XmlParser() :
	   _cpp( new RTEDGE::XmlParser() )
	{ ; }

	/** \brief Destructor */
	~XmlParser()
	{
	   delete _cpp;
	   _cpp = (RTEDGE::XmlParser *)0;
	}

	//////////////////////////////
	// Operations
	//////////////////////////////
public:
	/**  
	 * \brief Load XML document from file 
	 *    
	 * \param pFile : Filename to load
	 * \return isValid()
	 */   
	bool Load( String ^pFile )
	{
	   return _cpp->Load( _pStr( pFile ) );    
	}

	/**
	 * \brief Search for child XML element by name in entire XML tree
	 *
	 * \param elemName - Element name to find
	 * \return Element reference if found; null if not.
	 */
	XmlElem ^getElem( String ^elemName, bool bRecurse )
	{
	   return getElem( elemName, true );
	}

	/**
	 * \brief Search for child XML element by name
	 *
	 * \param elemName - Element name to find
	 * \param bRecurse - Search only 1st-level children if false
	 * \return Element reference if found; null if not.
	 */
	XmlElem ^getElem( String ^elemName, bool bRecurse )
	{
	   RTEDGE::XmlElem *cpp;

	   cpp = _cpp->find( _pStr( elemName ), bRecurse );
	   return cpp ? gcnew XmlElem( cpp ) : nullptr;
	}

}; // class XmlParser

} // namespace librtEdge
