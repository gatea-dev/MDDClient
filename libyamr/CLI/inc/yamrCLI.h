/******************************************************************************
*
*  yamrCLI.h
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <libyamr.h>
#endif // DOXYGEN_OMIT
#include <vcclr.h>  // gcroot

using namespace System;
using namespace System::Runtime::InteropServices;

/**
 * \mainpage libyamr API Reference Manual
 *
 * YAMR : Yet Another Messaging Recorder.  libyamr allows you to send
 * unstructured messages to the yamRecorder server farm for persistence.
 *
 * Multiple protocols are supported, allowing you to record any type of
 * data:
 * + Real-Time Market Data
 * + Order Flow : Trades and Fills
 * + Web Traffic
 * + Usage Traffic
 * + Text Messages
 * + Encrypted Data
 * + etc.
 *
 * You are free to define your own structured data atop the unstructured
 * message recording platform.  To do this, you define an extension of
 * the YAMR::Data::Codec class
 *
 * Several native protocol types are already defined for you:
 * Description | Class
 * --- | ---
 * Integer List | YAMR::Data::IntList
 * Single Precision List | YAMR::Data::FloatList
 * Double Precision List | YAMR::Data::DoubleList
 * Field List | YAMR::Data::FieldList
 * String List | YAMR::Data::StringList
 * String ( Key, Value ) Map | YAMR::Data::StringMap
 */
