//-----------------------------------------------------------------------
// <copyright file="StylusPlugInCollection.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------

using System;
using System.Windows;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Threading;

using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;

namespace System.Windows.Input.StylusPlugIns
{
    /// <summary>
    /// Collection of RawStylusInputCustomData objects
    /// </summary>
    internal class RawStylusInputCustomDataList : Collection<RawStylusInputCustomData>
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
        
        /// <summary> Constructor </summary>
        internal RawStylusInputCustomDataList()
        {
        }
    }

}

