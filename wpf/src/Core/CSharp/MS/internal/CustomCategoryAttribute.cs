//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.ComponentModel;
using MS.Internal.PresentationCore;

namespace MS.Internal
{
    /// <summary>
    ///     Allows for localizing custom categories.
    /// </summary>
    /// <remarks>
    ///     This class could be shared amongst any of the assemblies if desired.
    /// </remarks>
    internal sealed class CustomCategoryAttribute : CategoryAttribute
    {
        internal CustomCategoryAttribute() : base()
        {
        }

        internal CustomCategoryAttribute(string category) : base(category)
        {
        }

        protected override string GetLocalizedString(string value)
        {
            string s = SR.Get(value);
            if (s != null)
            {
                return s;
            }
            else
            {
                return value;
            }
        }
    }
}
