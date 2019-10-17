// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoLocationCoordinate
**
** Purpose: Represents a GeoCoordinate object
**
=============================================================================*/

using System;
using System.Globalization;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;

namespace System.Device.Location
{
    public class GeoCoordinate : IEquatable<GeoCoordinate>
    {
        private double m_latitude = double.NaN;
        private double m_longitude = double.NaN;
        private double m_altitude = double.NaN;
        private double m_verticalAccuracy = double.NaN;
        private double m_horizontalAccuracy = double.NaN;
        private double m_speed = double.NaN;
        private double m_course = double.NaN;

        public static readonly GeoCoordinate Unknown = new GeoCoordinate();

        internal CivicAddress m_address = CivicAddress.Unknown;

        #region Constructors
        //
        // private constructor for creating single instance of GeoCoordinate.Unknown
        //
        public GeoCoordinate() {}

        public GeoCoordinate(Double latitude, Double longitude) 
            : this(latitude, longitude, Double.NaN)
        {
        }

        public GeoCoordinate(Double latitude, Double longitude, Double altitude)
            : this(latitude, longitude, altitude, Double.NaN, Double.NaN, Double.NaN, Double.NaN)
        {
        }

        public GeoCoordinate(Double latitude, Double longitude, Double altitude,
            Double horizontalAccuracy, Double verticalAccuracy, Double speed, Double course)
        {
            Latitude = latitude;
            Longitude = longitude;

            Altitude = altitude;

            HorizontalAccuracy = horizontalAccuracy;
            VerticalAccuracy = verticalAccuracy;

            Speed = speed;
            Course = course;
        }
        #endregion

        #region Properties

        public Double Latitude 
        {
            get
            {
                return m_latitude;
            }
            set
            {
                if (value > 90.0 || value < -90.0)
                {
                    throw new ArgumentOutOfRangeException("Latitude", SR.GetString(SR.Argument_MustBeInRangeNegative90to90));
                }
                m_latitude = value;
            }
        }

        public Double Longitude
        {
            get
            {
                return m_longitude;
            }
            set
            {
                if (value > 180.0 || value < -180.0)
                {
                    throw new ArgumentOutOfRangeException("Longitude", SR.GetString(SR.Argument_MustBeInRangeNegative180To180));
                }
                m_longitude = value;
            }
        }

        public Double Altitude
        {
            get
            {
                return m_altitude;
            }

            set
            {
                m_altitude = value;
            }
        }

        public Double HorizontalAccuracy
        {
            get
            {
                return m_horizontalAccuracy;
            }
            set
            {
                if (value < 0.0)
                {
                    throw new ArgumentOutOfRangeException("HorizontalAccuracy", SR.GetString(SR.Argument_MustBeNonNegative));
                }
                m_horizontalAccuracy = (value == 0.0) ? Double.NaN : value;
            }
        }

        public Double VerticalAccuracy 
        {
            get
            {
                return m_verticalAccuracy;
            }
            set
            {
                if (value < 0.0)
                {
                    throw new ArgumentOutOfRangeException("VerticalAccuracy", SR.GetString(SR.Argument_MustBeNonNegative));
                }
                m_verticalAccuracy = (value == 0.0) ? Double.NaN : value;
            }
        }

        public Double Speed
        {
            get
            {
                return m_speed;
            }
            set
            {
                if (value < 0.0)
                {
                    throw new ArgumentOutOfRangeException("speed", SR.GetString(SR.Argument_MustBeNonNegative));
                }
                m_speed = value;
            }
        }

        public Double Course
        {
            get
            {
                return m_course;
            }
            set
            {
                if (value < 0.0 || value > 360.0)
                {
                    throw new ArgumentOutOfRangeException("course", SR.GetString(SR.Argument_MustBeInRangeZeroTo360));
                }
                m_course = value;
            }
        }
        
        public Boolean IsUnknown 
        {
            get
            {
                return this.Equals(GeoCoordinate.Unknown);
            }
        }

        #endregion

        #region Methods

        public Double GetDistanceTo(GeoCoordinate other)
        {
            //  The Haversine formula according to Dr. Math.
            //  http://mathforum.org/library/drmath/view/51879.html
                
            //  dlon = lon2 - lon1
            //  dlat = lat2 - lat1
            //  a = (sin(dlat/2))^2 + cos(lat1) * cos(lat2) * (sin(dlon/2))^2
            //  c = 2 * atan2(sqrt(a), sqrt(1-a)) 
            //  d = R * c
                
            //  Where
            //    * dlon is the change in longitude
            //    * dlat is the change in latitude
            //    * c is the great circle distance in Radians.
            //    * R is the radius of a spherical Earth.
            //    * The locations of the two points in 
            //        spherical coordinates (longitude and 
            //        latitude) are lon1,lat1 and lon2, lat2.

            if (Double.IsNaN(this.Latitude)  || Double.IsNaN(this.Longitude) ||
                Double.IsNaN(other.Latitude) || Double.IsNaN(other.Longitude))
            {
                throw new ArgumentException(SR.GetString(SR.Argument_LatitudeOrLongitudeIsNotANumber));
            }

            double dDistance = Double.NaN;

            double dLat1 = this.Latitude * (Math.PI / 180.0);
            double dLon1 = this.Longitude * (Math.PI / 180.0);
            double dLat2 = other.Latitude * (Math.PI / 180.0);
            double dLon2 = other.Longitude * (Math.PI / 180.0);

            double dLon = dLon2 - dLon1;
            double dLat = dLat2 - dLat1;

            // Intermediate result a.
            double a = Math.Pow(Math.Sin(dLat / 2.0), 2.0) + 
                       Math.Cos(dLat1) * Math.Cos(dLat2) * 
                       Math.Pow(Math.Sin(dLon / 2.0), 2.0);

            // Intermediate result c (great circle distance in Radians).
            double c = 2.0 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1.0 - a));

            // Distance.
            const Double kEarthRadiusMs = 6376500;
            dDistance = kEarthRadiusMs * c;

            return dDistance;
        }

        #endregion

        #region Object overrides
        public override int GetHashCode()
        {
            return Latitude.GetHashCode() ^ Longitude.GetHashCode();
        }

        /// <summary>
        /// Object.Equals. Calls into IEquatable.Equals
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            if (!(obj is GeoCoordinate)) return base.Equals(obj);
            return Equals(obj as GeoCoordinate);
        }
        /// <summary>
        /// This override is for debugging purpose only
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            if (this == GeoCoordinate.Unknown)
            {
                return "Unknown";
            }
            else
            {
                return Latitude.ToString("G", CultureInfo.InvariantCulture) + ", " +
                       Longitude.ToString("G", CultureInfo.InvariantCulture);
            }
        }

        #endregion

        #region IEquatable
        public bool Equals(GeoCoordinate other)
        {
            if (object.ReferenceEquals(other, null))
            {
                return false;
            }
            return Latitude.Equals(other.Latitude) && Longitude.Equals(other.Longitude);
        }
        #endregion

        #region Public static operators
        public static Boolean operator ==(GeoCoordinate left, GeoCoordinate right)
        {
            if (object.ReferenceEquals(left, null))
            {
                return object.ReferenceEquals(right, null);
            }
            return left.Equals(right);
        }

        public static Boolean operator !=(GeoCoordinate left, GeoCoordinate right)
        {
            return !(left == right);
        }
        #endregion
    }
}
