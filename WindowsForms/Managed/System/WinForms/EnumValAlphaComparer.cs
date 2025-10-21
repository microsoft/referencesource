//------------------------------------------------------------------------------
// <copyright file="EnumValAlphaComparer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.Collections;
    using System.Globalization;
 
    internal class EnumValAlphaComparer : IComparer {
        private CompareInfo m_compareInfo;
        internal static readonly EnumValAlphaComparer Default = new EnumValAlphaComparer();
        
        internal EnumValAlphaComparer() {
            m_compareInfo = CultureInfo.InvariantCulture.CompareInfo;
        }
  
        public int Compare(Object a, Object b) {
            return m_compareInfo.Compare(a.ToString(), b.ToString());
        }
    }
}

