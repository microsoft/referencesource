//---------------------------------------------------------------------------
//
// <copyright file=InputProcessorProfilesLoader.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Creates ITfThreadMgr instances, the root object of the Text
//              Services Framework.
//
// History:  
//  07/30/2003 : yutakas - ported from dotnet tree.
//
//---------------------------------------------------------------------------

// PRESHARP: In order to avoid generating warnings about unkown message numbers and unknown pragmas.
#pragma warning disable 1634, 1691

using System;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Security;
using System.Threading;

using Microsoft.Win32;
using System.Diagnostics;
using MS.Win32;

namespace System.Windows.Input
{
    //------------------------------------------------------
    //
    //  InputProcessorProfilesLoader class
    //
    //------------------------------------------------------

    /// <summary>
    /// Loads an instance of the Text Services Framework.
    /// </summary>
    internal static class InputProcessorProfilesLoader
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------

        #region Internal Properties
        
        /// <summary>
        /// Loads an instance of the Text Services Framework.
        /// </summary>
        /// <returns>
        /// May return null if no text services are available.
        /// </returns>
        /// <SecurityNote>
        /// Critical - calls unmanaged code to load the input profiles, returns unmanaged object
        /// </SecurityNote>
        [SecurityCritical]
        internal static UnsafeNativeMethods.ITfInputProcessorProfiles Load()
        {
            UnsafeNativeMethods.ITfInputProcessorProfiles obj;

            Debug.Assert(Thread.CurrentThread.GetApartmentState() == ApartmentState.STA, "Load called on MTA thread!");

            //
            // Bug#1212202
            //
            // Presharp warn this though UnsafeNativeMethods.TF_CreateInputProcessorProfiles() does not have 
            // SetLastError attribute.
            // We think this is a false alarm of Presharp.
            //
#pragma warning suppress 6523
            if (UnsafeNativeMethods.TF_CreateInputProcessorProfiles(out obj) == NativeMethods.S_OK)
            {
                return obj;
            }
            return null;
        }

        #endregion Internal Properties
    }
}
