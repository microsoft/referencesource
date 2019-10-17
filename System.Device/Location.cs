// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoLocation
**
** Purpose: Represents a GeoLocation object
**
=============================================================================*/

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;

namespace System.Device.Location
{
    public class GeoLocation
    {
        public static readonly GeoLocation Unknown = new GeoLocation(GeoCoordinate.Unknown);

        #region Constructors

        public GeoLocation(GeoCoordinate coordinate)
            : this(coordinate, Double.NaN, Double.NaN)
        {
        }

        public GeoLocation(GeoCoordinate coordinate, Double heading, Double speed)
            : this(coordinate, heading, speed, CivicAddress.Unknown, DateTimeOffset.Now)
        {
        }

        public GeoLocation(CivicAddress address)
            : this(GeoCoordinate.Unknown, Double.NaN, Double.NaN, address, DateTimeOffset.Now)
        {
        }

        public GeoLocation(GeoCoordinate coordinate, Double heading, Double speed, CivicAddress address, DateTimeOffset timestamp)
        {
            if (coordinate == null)
            {
                throw new ArgumentNullException("coordinate");
            }

            if (address == null)
            {
                throw new ArgumentNullException("address");
            }

            if (heading < 0.0 || heading > 360.0)
            {
                throw new ArgumentOutOfRangeException("heading", SR.GetString(SR.Argument_MustBeInRangeZeroTo360));
            }

            if (speed < 0.0)
            {
                throw new ArgumentOutOfRangeException("speed", SR.GetString(SR.Argument_MustBeNonNegative));
            }

            Coordinate = coordinate;
            Address = address;
            Heading = heading;
            Speed = speed;
            Timestamp = timestamp;
        }

        #endregion

        #region Properties

        public GeoCoordinate Coordinate { get; private set;}
        public Double Heading {get; private set; }
        public Double Speed { get; private set; }
        public CivicAddress Address { get; private set; }
        public DateTimeOffset Timestamp {get; private set; }

        #endregion
    }
}
