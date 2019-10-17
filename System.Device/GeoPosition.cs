// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoPosition
**
** Purpose: Represents a GeoPosition object
**
=============================================================================*/

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;

namespace System.Device.Location
{
    public class GeoPosition<T>
    {
        private DateTimeOffset m_timestamp = DateTimeOffset.MinValue;
        private T m_position;

        #region Constructors

        public GeoPosition() :
            this(DateTimeOffset.MinValue, default(T))
        {
        }

        public GeoPosition(DateTimeOffset timestamp, T position)
        {
            Timestamp = timestamp;
            Location = position;
        }

        #endregion

        #region Properties

        public T Location
        {
            get
            {
                return m_position;
            }

            set
            {
                m_position = value;
            }
        }

        public DateTimeOffset Timestamp
        {
            get
            {
                return m_timestamp;
            }
            set
            {
                m_timestamp = value;
            }
        }

        #endregion
    }
}
