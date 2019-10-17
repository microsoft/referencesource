// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoCoordinateWatcher
**
** Purpose: Represents a GeoCoordinateWatcher object
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
using System.Diagnostics.CodeAnalysis;

namespace System.Device.Location
{
    /// <summary>
    /// Represents location provider accuracy
    /// </summary>
    public enum GeoPositionAccuracy
    {
        Default = 0,
        High
    }
    /// <summary>
    /// Represents location provider status
    /// </summary>
    public enum GeoPositionStatus
    {
        Ready,          // Enabled
        Initializing,   // Working to acquire data
        NoData,         // We have access to sensors, but we cannnot resolve
        Disabled        // Location service disabled or access denied
    }
    /// <summary>
    /// Represents Geo watcher permission state
    /// </summary>
    public enum GeoPositionPermission
    {
        Unknown,
        Granted,
        Denied
    }

    /// <summary>
    /// IGeoPositionWatcher interface
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public interface IGeoPositionWatcher<T>
    {
        void Start();
        void Start(Boolean suppressPermissionPrompt);
        Boolean TryStart(Boolean suppressPermissionPrompt, TimeSpan timeout);
        [SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords")]
        void Stop();

        GeoPosition<T> Position { get; }
        event EventHandler<GeoPositionChangedEventArgs<T>> PositionChanged;

        GeoPositionStatus Status { get; }
        event EventHandler<GeoPositionStatusChangedEventArgs> StatusChanged;
    }

    /// <summary>
    /// Internal abstract class, representing platform dependant provider implementations
    /// GeoCoordinate is the watching target in this implementation.
    /// </summary>
    internal abstract class GeoCoordinateWatcherBase
    {
        public abstract Boolean TryStart(Boolean suppressPermissionPrompt, TimeSpan timeout);
        public abstract void Stop();
        public virtual Boolean IsStarted { get; protected set; }
        public virtual GeoPositionPermission Permission { get; protected set; }
        public virtual GeoPositionStatus Status { get; protected set; }
        public virtual GeoPosition<GeoCoordinate> Position { get; protected set; }

        public virtual void OnPositionChanged(GeoPositionChangedEventArgs<GeoCoordinate> e)
        {
            EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>> t = PositionChanged;
            if (t != null) t(this, e);
        }

        public virtual void OnPositionStatusChanged(GeoPositionStatusChangedEventArgs e)
        {
            EventHandler<GeoPositionStatusChangedEventArgs> t = StatusChanged;
            if (t != null) t(this, e);
        }

        public virtual void OnPermissionChanged(GeoPermissionChangedEventArgs e)
        {
            EventHandler<GeoPermissionChangedEventArgs> t = PermissionChanged;
            if (t != null) t(this, e);
        }

        public event EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>> PositionChanged;
        public event EventHandler<GeoPositionStatusChangedEventArgs> StatusChanged;
        public event EventHandler<GeoPermissionChangedEventArgs> PermissionChanged;
    }

    /// <SecurityNote>
    /// Critical - the only class in Location API that invokes native platform
    /// </SecurityNote>
    [SecurityCritical]
    public class GeoCoordinateWatcher 
        : IDisposable, INotifyPropertyChanged, IGeoPositionWatcher<GeoCoordinate>
    {
        private GeoCoordinate m_lastCoordinate = GeoCoordinate.Unknown;
        private GeoPositionAccuracy m_desiredAccuracy = GeoPositionAccuracy.Default;
        private GeoCoordinateWatcherInternal m_watcher;
        private PropertyChangedEventHandler m_propertyChanged;
        private EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>> m_positionChanged;
        private EventHandler<GeoPositionStatusChangedEventArgs> m_statusChanged;
        private SynchronizationContext m_synchronizationContext;
        private bool m_disposed;
        private double m_threshold = 0.0;

        #region Constructors

        public GeoCoordinateWatcher()
            : this(GeoPositionAccuracy.Default)
        {
        }

        public GeoCoordinateWatcher(GeoPositionAccuracy desiredAccuracy)
        {
            m_desiredAccuracy = desiredAccuracy;

            m_watcher = new GeoCoordinateWatcherInternal(desiredAccuracy);

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

            m_watcher.StatusChanged += new EventHandler<GeoPositionStatusChangedEventArgs>(OnInternalStatusChanged);
            m_watcher.PermissionChanged +=new EventHandler<GeoPermissionChangedEventArgs>(OnInternalPermissionChanged);
            m_watcher.PositionChanged += new EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>>(OnInternalLocationChanged);
        }

        #endregion

        #region Properties

        public GeoPositionAccuracy DesiredAccuracy
        {
            get
            {
                DisposeCheck();
                return m_desiredAccuracy;
            }
            private set
            {
                DisposeCheck();
                m_desiredAccuracy = value;
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

        public GeoPositionPermission Permission
        {
            get
            {
                DisposeCheck();
                return m_watcher.Permission;
            }
        }

        #endregion

        #region IGeoCoordinateWatcher implementation

        public GeoPosition<GeoCoordinate> Position
        {
            [SecuritySafeCritical]
            get
            {
                DisposeCheck();
                return m_watcher.Position;
            }
        }

        public GeoPositionStatus Status
        {
            [SecuritySafeCritical]
            get
            {
                DisposeCheck();
                return m_watcher.Status;
            }
        }
        
        [SecuritySafeCritical]
        public void Start()
        {
            DisposeCheck();
            this.Start(false);
        }

        [SecuritySafeCritical]
        public void Start(Boolean suppressPermissionPrompt)
        {
            DisposeCheck();
            m_watcher.TryStart(suppressPermissionPrompt, TimeSpan.Zero);
        }

        [SecuritySafeCritical]
        public Boolean TryStart(Boolean suppressPermissionPrompt, TimeSpan timeout)
        {
            DisposeCheck();
            //
            // Timeout needs to be in the range of 0 ~ MaxValue
            //
            long tm = (long)timeout.TotalMilliseconds;
            if (tm <= 0 || (long)Int32.MaxValue < tm)
            {
                return m_watcher.IsStarted;
            }

            return m_watcher.TryStart(suppressPermissionPrompt, timeout);
        }

        [SecuritySafeCritical]
        public void Stop()
        {
            DisposeCheck();
            m_watcher.Stop();
        }

        #endregion

        private void OnInternalLocationChanged(object sender, GeoPositionChangedEventArgs<GeoCoordinate> e)
        {
            if (e.Position != null)
            {
                Utility.Trace("GeoCoordinateWatcher.OnInternalLocationChanged: " + e.Position.ToString());
                //
                // Only fire event when location change exceeds the movement threshold or the coordinate
                // is unknown, as in the case of a civic address only report.
                //
                if ((m_lastCoordinate == GeoCoordinate.Unknown) || (e.Position.Location == GeoCoordinate.Unknown)
                    || (e.Position.Location.GetDistanceTo(m_lastCoordinate) >= m_threshold))
                {
                    m_lastCoordinate = e.Position.Location;

                    PostEvent(OnPositionChanged, new GeoPositionChangedEventArgs<GeoCoordinate>(e.Position));

                    OnPropertyChanged("Position");
                }
            }
        }

        void OnInternalStatusChanged(object sender, GeoPositionStatusChangedEventArgs e)
        {
            PostEvent(OnPositionStatusChanged, new GeoPositionStatusChangedEventArgs(e.Status));

            OnPropertyChanged("Status");
        }

        void OnInternalPermissionChanged(object sender, GeoPermissionChangedEventArgs e)
        {
            OnPropertyChanged("Permission");
        }

        protected void OnPositionChanged(GeoPositionChangedEventArgs<GeoCoordinate> e)
        {
            Utility.Trace("GeoCoordinateWatcher.OnPositionChanged: " + e.Position.Location.ToString());
            EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>> t = PositionChanged;
            if (t != null) t(this, e);
        }

        protected void OnPositionStatusChanged(GeoPositionStatusChangedEventArgs e)
        {
            Utility.Trace("GeoCoordinateWatcher.OnPositionStatusChanged: " + e.Status.ToString());
            EventHandler<GeoPositionStatusChangedEventArgs> t = StatusChanged;
            if (t != null) t(this, e);
        }

        protected void OnPropertyChanged(String propertyName)
        {
            if (m_propertyChanged != null)
                m_propertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        #region Events

        public event EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>> PositionChanged;
        public event EventHandler<GeoPositionStatusChangedEventArgs> StatusChanged;

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

        event EventHandler<GeoPositionChangedEventArgs<GeoCoordinate>> IGeoPositionWatcher<GeoCoordinate>.PositionChanged
        {
            [SecuritySafeCritical]
            add
            {
                m_positionChanged += value;
            }
            [SecuritySafeCritical]
            remove
            {
                m_positionChanged -= value;
            }
        }

        event EventHandler<GeoPositionStatusChangedEventArgs> IGeoPositionWatcher<GeoCoordinate>.StatusChanged
        {
            [SecuritySafeCritical]
            add
            {
                m_statusChanged += value;
            }
            [SecuritySafeCritical]
            remove
            {
                m_statusChanged -= value;
            }
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
        ~GeoCoordinateWatcher()
        {
            Dispose(false);
        }

        protected virtual void Dispose(Boolean disposing)
        {
            if (!m_disposed)
            {
                if (disposing)
                {
                    if (m_watcher != null)
                    {
                        m_watcher.Dispose();
                        m_watcher = null;
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
                throw new ObjectDisposedException("GeoCoordinateWatcher");
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
    public class GeoPositionChangedEventArgs<T> : EventArgs
    {
        public GeoPositionChangedEventArgs(GeoPosition<T> position)
        {
            Position = position;
        }

        public GeoPosition<T> Position { get; private set; }
    }

    /// <summary>
    /// Provide Status corresponding to the most recent location change status
    /// </summary>
    public class GeoPositionStatusChangedEventArgs : EventArgs
    {
        public GeoPositionStatusChangedEventArgs(GeoPositionStatus status)
        {
            Status = status;
        }

        public GeoPositionStatus Status { get; private set; }
    }

    /// <summary>
    /// Provide Permission corresponding to the most recent location permission change status
    /// </summary>
    internal class GeoPermissionChangedEventArgs : EventArgs
    {
        public GeoPermissionChangedEventArgs(GeoPositionPermission permission)
        {
            Permission = permission;
        }

        public GeoPositionPermission Permission { get; private set; }
    }
}
