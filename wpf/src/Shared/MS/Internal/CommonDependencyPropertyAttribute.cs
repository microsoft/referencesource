//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
//---------------------------------------------------------------------------

using System;
using System.Diagnostics;

#if WINDOWS_BASE
namespace MS.Internal.WindowsBase
#elif PRESENTATION_CORE
namespace MS.Internal.PresentationCore
#elif PRESENTATIONFRAMEWORK
namespace MS.Internal.PresentationFramework
#else
#error Class is being used from an unknown assembly.
#endif
{
    /// <summary>
    ///     An attribute that indicates that a DependencyProperty declaration is common
    ///     enough to be included in KnownTypes.cs.
    /// </summary>
    [Conditional("COMMONDPS")]
    internal sealed class CommonDependencyPropertyAttribute : Attribute
    {
    }
}
