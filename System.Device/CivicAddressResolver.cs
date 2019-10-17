// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: CivicAddress
**
** Purpose: Represents a CivicAddress object
**
=============================================================================*/

using System;
using System.ComponentModel;
using System.Globalization;
using System.Collections.Generic;
using System.Threading;
using System.Diagnostics.CodeAnalysis;

namespace System.Device.Location
{
    public class CivicAddress
    {
        public static readonly CivicAddress Unknown = new CivicAddress();

        //
        // private construcotr for creating single instance of CivicAddress.Unknown
        //
        public CivicAddress()
        {
            AddressLine1  = String.Empty;
            AddressLine2  = String.Empty;
            Building      = String.Empty;
            City          = String.Empty;
            CountryRegion = String.Empty;
            FloorLevel    = String.Empty;
            PostalCode    = String.Empty;
            StateProvince = String.Empty;
        }
            
        public CivicAddress(String addressLine1, String addressLine2, String building, String city, String countryRegion, String floorLevel, String postalCode, String stateProvince)
            : this()
        {
            bool hasField = false;

            if (addressLine1 != null && addressLine1 != String.Empty)
            {
                hasField = true;
                AddressLine1 = addressLine1;
            }
            if (addressLine2 != null && addressLine2 != String.Empty)
            {
                hasField = true;
                AddressLine2 = addressLine2;
            }
            if (building != null && building != String.Empty)
            {
                hasField = true;
                Building = building;
            }
            if (city != null && city != String.Empty)
            {
                hasField = true;
                City = city;
            }
            if (countryRegion != null && countryRegion != String.Empty)
            {
                hasField = true;
                CountryRegion = countryRegion;
            }
            if (floorLevel != null && floorLevel != String.Empty)
            {
                hasField = true;
                FloorLevel = floorLevel;
            }
            if (postalCode != null && postalCode != String.Empty)
            {
                hasField = true;
                PostalCode = postalCode;
            }

            if (stateProvince != null && stateProvince != String.Empty)
            {
                hasField = true;
                StateProvince = stateProvince;
            }

            if (!hasField)
            {
                throw new ArgumentException(SR.GetString(SR.Argument_RequiresAtLeastOneNonEmptyStringParameter));
            }
        }

        public String AddressLine1 { get; set; }
        public String AddressLine2 { get; set; }
        public String Building { get; set; }
        public String City { get; set; }
        public String CountryRegion { get; set; }
        public String FloorLevel { get; set; }
        public String PostalCode { get; set; }
        public String StateProvince { get; set; }

        public Boolean IsUnknown 
        {
            get
            {
                return (String.IsNullOrEmpty(AddressLine1) && String.IsNullOrEmpty(AddressLine2) &&
                        String.IsNullOrEmpty(Building) && String.IsNullOrEmpty(City) && String.IsNullOrEmpty(CountryRegion) && String.IsNullOrEmpty(FloorLevel) && String.IsNullOrEmpty(PostalCode) && String.IsNullOrEmpty(StateProvince));
            }
        }

    }

    public interface ICivicAddressResolver
    {
        CivicAddress ResolveAddress(GeoCoordinate coordinate);
        void ResolveAddressAsync(GeoCoordinate coordinate);
        event EventHandler<ResolveAddressCompletedEventArgs> ResolveAddressCompleted;
    }

    public class ResolveAddressCompletedEventArgs : AsyncCompletedEventArgs
    {
        public ResolveAddressCompletedEventArgs(CivicAddress address, Exception error, Boolean cancelled, Object userState)
                 : base(error, cancelled, userState) 
        {
            Address = address;
        }

        public CivicAddress Address { get; private set;}
    }

    public sealed class CivicAddressResolver : ICivicAddressResolver
    {
        private SynchronizationContext m_synchronizationContext;

        public CivicAddressResolver()
        {
            if (SynchronizationContext.Current == null)
            {
                //
                // Create a SynchronizationContext if there isn't one on calling thread
                //
                m_synchronizationContext = new SynchronizationContext();
            }
            else
            {
                m_synchronizationContext = SynchronizationContext.Current;
            }
        }

        public CivicAddress ResolveAddress(GeoCoordinate coordinate)
        {
            if (coordinate == null)
            {
                throw new ArgumentNullException("coordinate");
            }

            if (coordinate.IsUnknown)
            {
                throw new ArgumentException("coordinate");
            }

            return coordinate.m_address;
        }

        public void ResolveAddressAsync(GeoCoordinate coordinate)
        {
            if (coordinate == null)
            {
                throw new ArgumentNullException("coordinate");
            }

            if (Double.IsNaN(coordinate.Latitude) || Double.IsNaN(coordinate.Longitude))
            {
                throw new ArgumentException("coordinate");
            }

            ThreadPool.QueueUserWorkItem(new WaitCallback(this.ResolveAddress), coordinate);
        }

        public event EventHandler<ResolveAddressCompletedEventArgs> ResolveAddressCompleted;

        private void OnResolveAddressCompleted(ResolveAddressCompletedEventArgs e)
        {
            EventHandler<ResolveAddressCompletedEventArgs> t = ResolveAddressCompleted;
            if (t != null) t(this, e);
        }

        /// <summary>Represents a callback to a protected virtual method that raises an event.</summary>
        /// <typeparam name="T">The <see cref="T:System.EventArgs"/> type identifying the type of object that gets raised with the event"/></typeparam>
        /// <param name="e">The <see cref="T:System.EventArgs"/> object that should be passed to a protected virtual method that raises the event.</param>
        private delegate void EventRaiser<T>(T e) where T : EventArgs;

        /// <summary>A helper method used by derived types that asynchronously raises an event on the application's desired thread.</summary>
        /// <typeparam name="T">The <see cref="T:System.EventArgs"/> type identifying the type of object that gets raised with the event"/></typeparam>
        /// <param name="callback">The protected virtual method that will raise the event.</param>
        /// <param name="e">The <see cref="T:System.EventArgs"/> object that should be passed to the protected virtual method raising the event.</param>
        private void PostEvent<T>(EventRaiser<T> callback, T e) where T : EventArgs 
        {
            if (m_synchronizationContext != null)
            {
                m_synchronizationContext.Post(delegate(Object state) { callback((T)state); }, e);
            }
        }

        //
        // Thread pool thread used to resolve civic address
        //
        private void ResolveAddress(object state)
        {
            GeoCoordinate coordinate = state as GeoCoordinate;
            if (coordinate != null)
            {
                PostEvent(OnResolveAddressCompleted, new ResolveAddressCompletedEventArgs(coordinate.m_address, null, false, null));
            }
        }
    }
}
