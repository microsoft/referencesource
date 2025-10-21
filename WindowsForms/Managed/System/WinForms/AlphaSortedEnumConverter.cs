//------------------------------------------------------------------------------
// <copyright file="AlphaSortedEnumConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Collections;
    
    internal class AlphaSortedEnumConverter : EnumConverter {
        public AlphaSortedEnumConverter(Type type) : base(type) {
        }

        protected override IComparer Comparer {
            get {
                return EnumValAlphaComparer.Default;
            }
        }
    }
}
