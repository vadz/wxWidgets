///////////////////////////////////////////////////////////////////////////////
// Name:        wx/stringfwd.h
// Purpose:     Forward declare wxString and related classes.
// Author:      Vadim Zeitlin
// Created:     2012-06-19
// RCS-ID:      $Id: wxhead.h,v 1.12 2010-04-22 12:44:51 zeitlin Exp $
// Copyright:   (c) 2012 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STRINGFWD_H_
#define _WX_STRINGFWD_H_

// Make sure to get wxHAS_COMPILER_ADL definition. Notice that we can't include
// wx/defs.h here because it includes wx/debug.h which in turn includes this
// header.
#include "wx/features.h"

#ifdef wxHAS_COMPILER_ADL
namespace wxInternal
{
#endif // wxHAS_COMPILER_ADL
class WXDLLIMPEXP_FWD_BASE wxString;
class WXDLLIMPEXP_FWD_BASE wxCStrData;
#ifdef wxHAS_COMPILER_ADL
} // namespace wxInternal

using wxInternal::wxCStrData;
using wxInternal::wxString;
#endif // wxHAS_COMPILER_ADL

#endif // _WX_STRINGFWD_H_
