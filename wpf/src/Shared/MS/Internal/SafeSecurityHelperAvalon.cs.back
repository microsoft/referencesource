/***************************************************************************\
*
* File: SafeSecurityHelper.cs
*
* Purpose:  Helper functions for avalon code that require elevation but are safe to use.
*                
* History:
*    12/09/04:    marka Created
*
* Copyright (C) 2004 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

using System;
using System.Globalization;
using System.Security;
using System.Security.Permissions;
using System.Reflection;
using System.Windows.Media ; 
using System.Windows; 

//****************
//
// this class is for helpers that require avalon.
// done this way so that Compiler doesn't build this file to minimize dependencies compiler has. 
//
//****************

#if WINDOWS_BASE
namespace MS.Internal.WindowsBase
#elif PRESENTATION_CORE
namespace MS.Internal.PresentationCore
#elif PRESENTATIONFRAMEWORK
namespace MS.Internal.PresentationFramework
#elif DRT
namespace MS.Internal.Drt
#else
#error Attempting to use this class from an unknown assembly.
#endif
{
    internal  static partial class SafeSecurityHelper
    {

        /// <summary> 
        /// is this visual connected to presentation source ? 
        /// i.e. is it "renderable" ?
        ///</summary> 
        /// <SecurityNote>
        /// Critical - extracts presentationsource
        /// TreatAsSafe - Knowing whether you're connected to presentation source is ok. 
        /// </SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe] 
        internal static bool IsConnectedToPresentationSource( Visual visual ) 
        {
            bool isConnected = false; 

            isConnected = PresentationSource.CriticalFromVisual(visual ) != null;

            return isConnected ; 
        }
    }
}
