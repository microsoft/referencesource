//------------------------------------------------------------------------------
// <copyright file="EmfPlusFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;

    /**
     * EMF+ Flags
     */
    internal enum EmfPlusFlags
    {
        Display		    = 0x00000001, 
        NonDualGdi          = 0x00000002
    }
}
