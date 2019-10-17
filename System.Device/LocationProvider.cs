// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoLocationProvider
**
** Purpose: Represents a GeoLocationProvider object
**
=============================================================================*/

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Threading;
using System.Device.Location.Internal;
using System.Diagnostics;
using System.Globalization;
using System.Security;

namespace System.Device.Location
{
    /// <summary>
    /// Represents location provider accuracy
    /// </summary>
    public enum GeoLocationAccuracy
    {
        Low = 0,
        High
    }
    /// <summary>
    /// Represents location provider status
    /// </summary>
    public enum GeoLocationStatus
    {
        Ready,          // Enabled
        Initializing,   // Working to acquire data
        NoData,         // We have access to sensors, but we can’t resolve
        Disabled,       // Location service disabled or access denied
        NoPermissions   // User has not granted permissions
    }

    /// <summary>
    /// Internal abstract class, representing platform dependant provider implementations
    /// </summary>
    internal abstract class GeoLocationProviderBase
    {
        public abstract void Start();
        public abstract void Stop();

        public virtual GeoLocationStatus Status
        {
            get { return GeoLocationStatus.Disabled; }
        }

        public virtual GeoLocation Location
        {
            get { return GeoLocation.Unknown; }
        }

        public virtual void OnLocationChanged(GeoLocationChangedEventArgs e)
        {
            EventHandler<GeoLocationChangedEventArgs> t = LocationChanged;
            if (t != null) t(this, e);
        }
        public virtual void OnStatusChanged(GeoLocationStatusChangedEventArgs e)
        {
            EventHandler<GeoLocationStatusChangedEventArgs> t = StatusChanged;
            if (t != null) t(this, e);
        }

        public event EventHandler<GeoLocationChangedEventArgs> LocationChanged;
        public event EventHandler<GeoLocationStatusChangedEventArgs> StatusChanged;
    }

    /// <SecurityNote>
    /// Critical - the only class in Location API that invoke native platform
    /// </SecurityNote>
    [SecurityCritical]
    public class GeoLocationProvider : IDisposable, INotifyPropertyChanged
    {
        private GeoCoordinate m_lastCoordinate = GeoCoordinate.Unknown;
        private GeoLocationProviderInternal m_provider;
        private PropertyChangedEventHandler m_propertyChanged;
        private SynchronizationContext m_synchronizationContext;
        private bool m_disposed;
        private bool m_started;
        private double m_threshold = 0.0;
        private GeoLocationAccuracy m_desiredAccuracy;

        #region Constructors

        public GeoLocationProvider()
            : this(GeoLocationAccuracy.Low)
        {
        }

        public GeoLocationProvider(GeoLocationAccuracy desiredAccuracy)
        {
            m_desiredAccuracy = desiredAccuracy;

            m_provider = new GeoLocationProviderInternal(desiredAccuracy);

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

            m_provider.StatusChanged += new EventHandler<GeoLocationStatusChangedEventArgs>(OnInternalStatusChanged);
        }

        #endregion

        #region Properties

        public GeoLocationAccuracy DesiredAccuracy
        {
            get
            {
                DisposeCheck();
                return m_desiredAccuracy;
            }
        }

        public Double MovementThreshold
        {
            set
            {
                DisposeCheck();

                if (value < 0.0 || Double.IsNaN(value))
                {
                    throw new ArgumentOutOfRangeException("value", SR.GetString(SR.Argument_MustBeNonNegative));
                }
                m_threshold = value;
            }
            get
            {
                DisposeCheck();
                return m_threshold;
            }
        }

        public GeoLocationStatus Status
        {
            get
            {
                DisposeCheck();
                return (m_provider == null) ? GeoLocationStatus.Disabled : m_provider.Status;
            }
        }

        public GeoLocation Location
        {
            get
            {
                DisposeCheck();
                return (m_provider == null) ? GeoLocation.Unknown : m_provider.Location;
            }
        }
        #endregion

        #region Method

        public void Start()
        {
            DisposeCheck();

            if (!m_started)
            {
                if (m_provider != null)
                {
                    m_provider.LocationChanged += new EventHandler<GeoLocationChangedEventArgs>(OnInternalLocationChanged);

                    m_provider.Start();
                }

                m_started = true;
            }
        }

        public void Stop()
        {
            DisposeCheck();

            if (m_started)
            {
                if (m_provider != null)
                {
                    m_provider.Stop();

                    m_provider.LocationChanged -= new EventHandler<GeoLocationChangedEventArgs>(OnInternalLocationChanged);
                }

                m_started = false;
            }
        }

        private void OnInternalLocationChanged(object sender, GeoLocationChangedEventArgs e)
        {
            if (e.Location != null)
            {
                Utility.Trace("GeoLocationProvider.OnInternalLocationChanged: " + e.Location.Coordinate.ToString());
                //
                // Only fire event when location change exceeds the movement threshold or the coordinate
                // is unknown, as in the case of a civic address only report.
                //
                if ((m_lastCoordinate == GeoCoordinate.Unknown) || (e.Location.Coordinate == GeoCoordinate.Unknown)
                    || (e.Location.Coordinate.GetDistanceTo(m_lastCoordinate) >= m_threshold))
                {
                    m_lastCoordinate = e.Location.Coordinate;

                    PostEvent(OnLocationChanged, new GeoLocationChangedEventArgs(e.Location));

                    OnPropertyChanged("Location");
                }
            }
        }

        void OnInternalStatusChanged(object sender, GeoLocationStatusChangedEventArgs e)
        {
            PostEvent(OnStatusChanged, new GeoLocationStatusChangedEventArgs(e.Status));

            OnPropertyChanged("Status");
        }

        protected void OnLocationChanged(GeoLocationChangedEventArgs e)
        {
            Utility.Trace("GeoLocationProvider.OnLocationChanged: " + e.Location.Coordinate.ToString());

            EventHandler<GeoLocationChangedEventArgs> t = LocationChanged;
            if (t != null) t(this, e);
        }

        protected void OnStatusChanged(GeoLocationStatusChangedEventArgs e)
        {
            Utility.Trace("GeoLocationProvider.OnStatusChanged: " + e.Status.ToString());
            EventHandler<GeoLocationStatusChangedEventArgs> t = StatusChanged;
            if (t != null) t(this, e);
        }

        #endregion

        #region Events

        event PropertyChangedEventHandler INotifyPropertyChanged.PropertyChanged
        {
            [SecuritySafeCritical]
            add
            {
                m_propertyChanged += value;
            }
            [SecuritySafeCritical]
            remove
            {
                m_propertyChanged -= value;
            }
        }

        public event EventHandler<GeoLocationChangedEventArgs> LocationChanged;
        public event EventHandler<GeoLocationStatusChangedEventArgs> StatusChanged;

        protected void OnPropertyChanged(String propertyName)
        {
            if (m_propertyChanged != null)
                m_propertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }


        #endregion

        #region IDisposable

        [SecuritySafeCritical]
        public void Dispose() 
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        [SecuritySafeCritical]
        ~GeoLocationProvider()
        {
            Dispose(false);
        }

        protected virtual void Dispose(Boolean disposing)
        {
            if (!m_disposed)
            {
                if (disposing)
                {
                    if (m_provider != null)
                    {
                        m_provider.Dispose();
                        m_provider = null;
                    }
                }

                m_disposed = true;
            }
        }

        #endregion

        private void DisposeCheck()
        {
            if (m_disposed)
            {
                throw new ObjectDisposedException("GeoLocationProvider");
            }
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
            Debug.Assert(m_synchronizationContext != null);
            m_synchronizationContext.Post(delegate(Object state) { callback((T)state); }, e);
        }
    }

    /// <summary>
    /// Provide Location data corresponding to the most recent location change data
    /// </summary>
    public class GeoLocationChangedEventArgs : EventArgs
    {
        public GeoLocationChangedEventArgs(GeoLocation location)
        {
            Location = location;
        }

        public GeoLocation Location { get; private set; }
    }
    /// <summary>
    /// 
    /// </summary>
    public class GeoLocationStatusChangedEventArgs : EventArgs
    {
        public GeoLocationStatusChangedEventArgs(GeoLocationStatus status)
        {
            Status = status;
        }

        public GeoLocationStatus Status { get; private set; }
    }

}
