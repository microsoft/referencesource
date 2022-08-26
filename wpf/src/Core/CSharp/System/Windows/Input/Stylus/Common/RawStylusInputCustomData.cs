//-----------------------------------------------------------------------
// <copyright file="RawStylusInputCustomData.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------

using System;
using System.Windows;

using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;

namespace System.Windows.Input.StylusPlugIns
{
    /// <summary>
    /// RawStylusInputCustomData object
    /// </summary>
    internal class RawStylusInputCustomData
    {
        /// <summary>
        /// RawStylusInputCustomData constructor
        /// </summary>
        public RawStylusInputCustomData(StylusPlugIn owner, object data)
        {
            _data = data;
            _owner = owner;
        }

        /// <summary>
        /// Returns custom data
        /// </summary>
        public object Data
        {
            get
            {
                return _data;
            }
        }

        /// <summary>
        /// Returns owner of this object (which is who gets notification)
        /// </summary>
        public StylusPlugIn Owner
        {
            get
            {
                return _owner;
            }
        }

        StylusPlugIn    _owner;
        object          _data;
    }

}

