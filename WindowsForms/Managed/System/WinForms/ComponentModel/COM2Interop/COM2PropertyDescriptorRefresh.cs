//------------------------------------------------------------------------------
// <copyright file="COM2PropertyDescriptorRefresh.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
        
        [SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses")]
        internal class Com2PropertyDescriptorRefresh{
                public const int Attributes         = 0x0001;
                public const int DisplayName        = 0x0002;
                public const int ReadOnly           = 0x0004;
                public const int TypeConverter      = 0x0020;
                public const int TypeEditor         = 0x0040;
                
                public const int All                = 0x00FF;
                
                public const int TypeConverterAttr  = 0x2000;
                public const int TypeEditorAttr     = 0x4000;
                public const int BaseAttributes     = 0x8000;
                
        }
}
