//---------------------------------------------------------------------------
//
// <copyright file="HashHelper.cs" company="Microsoft">
//    Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: Static class to help work around hashing-related bugs.
//
//---------------------------------------------------------------------------

using System;
using MS.Internal;                  // BaseHashHelper

#if WINDOWS_BASE
namespace MS.Internal.Hashing.WindowsBase
#elif PRESENTATION_CORE
namespace MS.Internal.Hashing.PresentationCore
#elif PRESENTATIONFRAMEWORK
using System.ComponentModel;        // ICustomTypeDescriptor
namespace MS.Internal.Hashing.PresentationFramework
#else
#error Attempt to define HashHelper in an unknown assembly.
namespace MS.Internal.YourAssemblyName
#endif
{
    internal static class HashHelper
    {
        // The class cctor registers this assembly's exceptional types with
        // the base helper.
        static HashHelper()
        {
            Initialize();       // this makes FxCop happy - otherwise Initialize is "unused code"

            Type[] types = new Type[] {
#if WINDOWS_BASE
#elif PRESENTATION_CORE
                typeof(System.Windows.Media.CharacterMetrics),      // bug 1612093
                typeof(System.Windows.Ink.ExtendedProperty),        // bug 1612101
                typeof(System.Windows.Media.FamilyTypeface),        // bug 1612103
                typeof(System.Windows.Media.NumberSubstitution),    // bug 1612105
#elif PRESENTATIONFRAMEWORK
                typeof(System.Windows.Markup.Localizer.BamlLocalizableResource),    // bug 1612118
                typeof(System.Windows.ComponentResourceKey),        // bug 1612119
#endif
            };

            BaseHashHelper.RegisterTypes(typeof(HashHelper).Assembly, types);

            // initialize lower-level assemblies
#if PRESENTATIONFRAMEWORK
            MS.Internal.Hashing.PresentationCore.HashHelper.Initialize();
#endif
        }


        // certain objects don't have reliable hashcodes, and cannot be used
        // within a Hashtable, Dictionary, etc.
        internal static bool HasReliableHashCode(object item)
        {
            return BaseHashHelper.HasReliableHashCode(item);
        }

        // this method doesn't do anything, but calling it makes sure the static
        // cctor gets called
        internal static void Initialize()
        {
        }
    }
}
