//---------------------------------------------------------------------------
//
// <copyright file="ValidateEnums.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// ValidateEnums is defined across several generated files in 
//  Shared\MS\Internal.  The class is built into Core but also used
//  by Framework.  This file's sole purpose is to mark the class
//  with the FriendAccessAllowed attribute denoting that it's OK for
//  Framework to be reaching into this class in Core.
//
//---------------------------------------------------------------------------

namespace System.Windows.Media
{
#if WINDOWS_BASE
    using MS.Internal.WindowsBase;
#elif PRESENTATION_CORE
    using MS.Internal.PresentationCore;
#elif PRESENTATIONFRAMEWORK
    using MS.Internal.PresentationFramework;
#elif DRT
    using MS.Internal.Drt;
#else
#error Attempt to use FriendAccessAllowedAttribute from an unknown assembly.
using MS.Internal.YourAssemblyName;
#endif

    [FriendAccessAllowed] // Built into Core, also used by Framework.
    internal static partial class ValidateEnums
    {
    }
}
