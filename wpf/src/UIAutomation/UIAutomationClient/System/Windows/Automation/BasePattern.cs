//---------------------------------------------------------------------------
//
// <copyright file="BasePattern" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Client-side wrapper for Base Pattern
//
// History:  
//  06/23/2003 : BrendanM Ported to WCP
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Automation;
using System.Diagnostics;
using MS.Internal.Automation;
using System.Runtime.InteropServices;

namespace System.Windows.Automation
{
    /// <summary>
    /// Internal class
    /// </summary>
#if (INTERNAL_COMPILE)
    internal class BasePattern
#else
    public class BasePattern
#endif
    {
        internal AutomationElement _el;
        private SafePatternHandle _hPattern;

        internal BasePattern( AutomationElement el, SafePatternHandle hPattern )
        {
            Debug.Assert(el != null);

            _el = el;
            _hPattern = hPattern;
        }

        /// <summary>
        /// Overrides Object.Finalize
        /// </summary>
        ~BasePattern()
        {
            // 
        }
    }
}
