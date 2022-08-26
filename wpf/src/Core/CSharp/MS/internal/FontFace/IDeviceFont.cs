//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2002
//
//  File:      IDeviceFont.cs
//
//  Contents:  Base definition of device font
//
//  Created:   6-20-2005 Niklas Borson (niklasb)
//
//------------------------------------------------------------------------

using System;
using System.Security;
using System.Security.Permissions;


namespace MS.Internal.FontFace
{
    internal interface IDeviceFont
    {
        /// <summary>
        /// Well known name of device font
        /// </summary>
        string Name
        { get; }

        /// <summary>
        /// Returns true if the device font maps the specified character.
        /// </summary>
        bool ContainsCharacter(int unicodeScalar);

        /// <summary>
        /// Return advance widths corresponding to characters in a given string.
        /// </summary>
        /// <SecurityNote>
        /// Critical - As it uses raw pointers.
        /// </SecurityNote>
        [SecurityCritical]
        unsafe void GetAdvanceWidths(
            char*   characterString,
            int     characterLength,
            double  emSize,
            int*    pAdvances
        );
    }
}
