//------------------------------------------------------------------------------
// <copyright file="_CTLBLDTYPE.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.ComponentModel;
    using System.Diagnostics.CodeAnalysis;
    
    [CLSCompliant(false)]
    [SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses")]
    internal class _CTLBLDTYPE {
        public const int CTLBLDTYPE_FSTDPROPBUILDER   = 0x00000001;
        public const int CTLBLDTYPE_FINTERNALBUILDER  = 0x00000002;
        public const int CTLBLDTYPE_FEDITSOBJDIRECTLY = 0x00000004;
    }
}
