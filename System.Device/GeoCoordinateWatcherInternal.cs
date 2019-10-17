// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoCoordinateWatcherInternal
**
** Purpose: Represents a GeoCoordinateWatcherInternal implementations
**  This is platform dependant wrapper for the provider.  It seperates
**  GeoPositionProvider from underly Location platforms.
**
=============================================================================*/

using System;
using System.Security;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Threading;
using System.Device.Location.Internal;
using System.Globalization;

namespace System.Device.Location
{
    internal sealed class GeoCoordinateWatcherInternal : 
        GeoCoordinateWatcherBase, ILocationEvents, IDisposable
    {
        private object m_lock;
        private int m_positionEventsProcessingCount = 0;
        private volatile bool m_dontProcessPositionEvents = false;

        private ILocation m_location;

        private bool m_disposed;

        private bool m_eventPending;

        private bool m_latLongRegistered;
        private bool m_civicAddrRegistered;

        private ManualResetEvent m_eventGetLocDone;

        private ReportStatus m_latLongStatus   = ReportStatus.NotSupported; 
        private ReportStatus m_civicAddrStatus = ReportStatus.NotSupported;

        private GeoPositionPermission m_permission = GeoPositionPermission.Unknown;
        private GeoPositionStatus m_curStatus  = GeoPositionStatus.NoData;
        private GeoPosition<GeoCoordinate> m_position = new GeoPosition<GeoCoordinate>(DateTimeOffset.MinValue, GeoCoordinate.Unknown);

        /// <summary>
        /// Helper to map internal COM ReportStatus into GeoPositionPermission
        /// </summary>
        /// <param name="statue"></param>
        private void ChangePermissionFromReportStatus(ReportStatus status)
        {
            GeoPositionPermission permission = GeoPositionPermission.Unknown;

            switch (status)
            {
                case ReportStatus.Error:
                case ReportStatus.Initializing:
                case ReportStatus.NotSupported:
                case ReportStatus.Running:
                    permission = GeoPositionPermission.Granted;
                    break;
                case ReportStatus.AccessDenied:
                    permission = GeoPositionPermission.Denied;
                    break;
                default: break;
            }

            this.Permission = permission;
        }
        /// <summary>
        /// Thread pool callback used to ensure location COM API is instantiated from
        /// multi-thread apartment, and register for location events.
        /// </summary>
        /// <param name="state"></param>
        private void CreateHandler(object state)
        {
            lock (this.InternalSyncObject)
            {
                try
                {
                    m_location = new COMLocation() as ILocation;
                }
                catch (COMException)
                {
                    Utility.Trace("Failed to CoCreate ILocation COM object.");
                }

                if (null != m_location)
                {
                    Utility.Trace("Successfully created ILocation COM object");

                    //
                    // Mapping to platform accuracy
                    //
                    GeoPositionAccuracy desiredAccuracy = (GeoPositionAccuracy)state;
                    DesiredAccuracy accuracy = (desiredAccuracy == GeoPositionAccuracy.Default) ?
                                                DesiredAccuracy.DefaultAccuracy : DesiredAccuracy.HighAccuracy;

                    Guid reportKey = LocationReportKey.LatLongReport;
                    m_location.SetDesiredAccuracy(ref reportKey, accuracy);

                    //
                    // Always listen for latlong reports
                    //
                    if (m_location.RegisterForReport(this, ref reportKey, 0) == 0)
                    {
                        m_latLongRegistered = true;
                    }
                    //
                    // Check the latlong status. If latlong reports are not supported, then register 
                    // for civic address reports.
                    //
                    m_location.GetReportStatus(ref reportKey, ref m_latLongStatus);

                    if (ReportStatus.NotSupported == m_latLongStatus)
                    {
                        reportKey = LocationReportKey.CivicAddressReport;
                        if (m_location.RegisterForReport(this, ref reportKey, 0) == 0)
                        {
                            m_civicAddrRegistered = true;
                            m_location.GetReportStatus(ref reportKey, ref m_civicAddrStatus);
                        }
                    }
                    //
                    // set the current status to the available report type status
                    // 
                    ReportStatus status = (ReportStatus.NotSupported != m_latLongStatus) ? m_latLongStatus : m_civicAddrStatus;
                    m_curStatus = m_geoStatusMap[(int)status];

                    ChangePermissionFromReportStatus(status);
                }
                else
                {
                    m_curStatus = GeoPositionStatus.Disabled;
                }
            }
        }

        #region Constructor
        // <summary>
        // Creates an instance of GeoCoordinateWatcherInternal with the specified desired accuracy
        // </summary>
        public GeoCoordinateWatcherInternal(GeoPositionAccuracy desiredAccuracy)
        {
            //
            // Create the native location object on a worker thread, so that it exists
            // in a multithreaded apartment.
            //
            ThreadPool.QueueUserWorkItem(new WaitCallback(this.CreateHandler), desiredAccuracy);

            Utility.Trace("GeoCoordinateWatcherInternal.ctor:" +
                          " desiredAccuracy: " + desiredAccuracy.ToString() +
                          " m_latLongRegistered: " + m_latLongRegistered.ToString() +
                          " m_civicAddrRegistered: " + m_civicAddrRegistered.ToString() +
                          " m_latLongStatus: " + m_latLongStatus.ToString() +
                          " m_curStatus: " + m_curStatus.ToString());
        }
        #endregion

        #region GeoPositionProviderBase overrides

        public override GeoPositionPermission Permission
        {
            get
            {
                return m_permission;
            }
            protected set
            {
                m_permission = value;
                OnPermissionChanged(new GeoPermissionChangedEventArgs(m_permission));
            }
        }

        public override GeoPositionStatus Status 
        { 
           get 
           {
               if (!IsStarted ||
                   ((m_curStatus == GeoPositionStatus.Ready) && m_position.Location.IsUnknown))
               {
                   return GeoPositionStatus.NoData;
               }

               return m_curStatus;
           }
        }

        public override GeoPosition<GeoCoordinate> Position
        {
            get
            {
                return (IsStarted && (this.Status == GeoPositionStatus.Ready)) ? m_position : new GeoPosition<GeoCoordinate>(DateTimeOffset.MinValue, GeoCoordinate.Unknown);
            }
        }

        public override Boolean TryStart(Boolean suppressPermissionPrompt, TimeSpan timeout)
        {
            if (!IsStarted)
            {
                IsStarted = true;

                //
                // If the reported status is ready and we have location data then generate an event 
                // to provide the initial location data. Otherwise, we spawn worker thread to obtain
                // the initial status and location, and timeout on 100m seconds.
                //
                if ((Status == GeoPositionStatus.Ready) && (!m_position.Location.IsUnknown))
                {
                    OnPositionChanged(new GeoPositionChangedEventArgs<GeoCoordinate>(m_position));
                }
                else
                {
                    m_eventGetLocDone = new ManualResetEvent(false);

                    ThreadPool.QueueUserWorkItem(new WaitCallback(this.GetInitialLocationData), suppressPermissionPrompt);
                    if (timeout != TimeSpan.Zero)
                    {
                        if (!m_eventGetLocDone.WaitOne(timeout))
                        {
                            this.Stop();
                        }
                    }
                }
            }

            Utility.Trace("GeoCoordinateWatcherInternal.TryStart " + timeout.ToString());

            return IsStarted;
        }

        //
        // Get the initial location data and set completion event when done.
        // This is executed on a worker thread.
        //
        private void GetInitialLocationData(object state)
        {
            lock (this.InternalSyncObject)
            {
                bool suppressPermissionPrompt = (bool)state;

                if (!(suppressPermissionPrompt && (this.Permission != GeoPositionPermission.Granted)))
                {
                    if (m_location != null)
                    {
                        if (m_curStatus == GeoPositionStatus.Ready)
                        {
                            GetLocationHelper();
                        }
                        else
                        {
                            //
                            // Check for permission only if asked
                            //
                            if (!suppressPermissionPrompt)
                            {
                                Guid reportType = (m_latLongStatus != ReportStatus.NotSupported) ?
                                                   LocationReportKey.LatLongReport : LocationReportKey.CivicAddressReport;
                                m_location.RequestPermissions(IntPtr.Zero, ref reportType, 1, 0);
                                Utility.Trace("RequestPermission is called");
                            }
                        }
                    }
                }

                ManualResetEvent eventDone = m_eventGetLocDone;
                if (eventDone != null)
                {
                    eventDone.Set();
                }
            }
        }

        //
        // Try to get location data after a short wait to see if a location
        // change event is fired. This is executed on a worker thread.
        //
        private void GetLocationData(object state)
        {
            GetLocationHelper();
        }

        //
        // Make an attempt to get location data
        //
        private void GetLocationHelper()
        {
            lock (this.InternalSyncObject)
            {
                if ((m_location != null) && (m_curStatus == GeoPositionStatus.Ready))
                {
                    Guid reportType = (m_latLongStatus != ReportStatus.NotSupported) ?
                                       LocationReportKey.LatLongReport : LocationReportKey.CivicAddressReport;

                    ILocationReport report;
                    if (m_location.GetReport(ref reportType, out report) == 0)
                    {
                        HandleLocationChangedEvent(report);
                    }
                }
            }
        }

        /// <summary>
        /// Stop the GeoCoordinateWatcher so that no internal state/location data/events can be exposed
        /// </summary>
        public override void Stop()
        {
            IsStarted = false;
            Utility.Trace("GeoCoordinateWatcherInternal.Stop");
        }
        #endregion

        #region ILocationEvents
        /// <summary>
        /// ILocationEvents.OnPositionChanged 
        /// </summary>
        /// <param name="reportType"></param>
        /// <param name="locationReport"></param>
        void ILocationEvents.OnPositionChanged(ref Guid reportType, ILocationReport locationReport)
        {
            if (!EnterProcessing())
                return;
                
            try {
                if (IsStarted)
                {
                    if (reportType.Equals(LocationReportKey.LatLongReport) ||
                        reportType.Equals(LocationReportKey.CivicAddressReport))
                    {
                        HandleLocationChangedEvent(locationReport);
                    }
                }                
            } finally {
                ExitProcessing();
            }
        }

        /// <summary>
        /// ILocationEvents.OnStatusChanged
        /// </summary>
        /// <param name="reportType"></param>
        /// <param name="newStatus"></param>
        void ILocationEvents.OnPositionStatusChanged(ref Guid reportType, ReportStatus newStatus)
        {
            if (!EnterProcessing())
                return;
                
            try {
                if (reportType.Equals(LocationReportKey.LatLongReport) ||
                    reportType.Equals(LocationReportKey.CivicAddressReport))
                {
                    if (reportType.Equals(LocationReportKey.LatLongReport))
                    {
                        m_latLongStatus = newStatus;
                    }
                    else if (reportType.Equals(LocationReportKey.CivicAddressReport))
                    {
                        m_civicAddrStatus = newStatus;
                    }

                    HandleLocationStatusChangedEvent(newStatus);
                }
            } finally {
                ExitProcessing();
            }
        }
        #endregion

        #region Helpers
        // Report status to GeoPosition status look-up table
        private static GeoPositionStatus[] m_geoStatusMap = new GeoPositionStatus[] 
        {
            GeoPositionStatus.NoData,       // ReportStatus.NotSupported = 0,
            GeoPositionStatus.NoData,       // ReportStatus.Error = 1,
            GeoPositionStatus.NoData,     // ReportStatus.AccessDenied = 2,
            GeoPositionStatus.Initializing, // ReportStatus.Initializing = 3,
            GeoPositionStatus.Ready         // ReportStatus.Running = 4
        };

        private void HandleLocationChangedEvent(ILocationReport locationReport)
        {
            const double KnotsToMetersPerSec = 463.0 / 900.0;
 
            GeoCoordinate coordinate = GeoCoordinate.Unknown;
            CivicAddress address = CivicAddress.Unknown;

            DateTimeOffset timestamp = DateTimeOffset.Now;
            SYSTEMTIME systime;
            if (0 == locationReport.GetTimestamp(out systime))
            {
                timestamp = new DateTimeOffset(systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                                               systime.wMinute, systime.wSecond, systime.wMilliseconds, TimeSpan.Zero);
            }

            //
            // If we are listening for latlong reports and this is one,
            // extract the coordinate properties
            //
            if (m_latLongStatus != ReportStatus.NotSupported)
            {
                ILatLongReport latLongReport = locationReport as ILatLongReport;
                if (latLongReport != null)
                {
                    double latitude, longitude, errorRadius, altitude, altitudeError;
                    if ((latLongReport.GetLatitude(out latitude) == 0) &&
                        (latLongReport.GetLongitude(out longitude) == 0))
                    {
                        latLongReport.GetErrorRadius(out errorRadius);
                        latLongReport.GetAltitude(out altitude);
                        latLongReport.GetAltitudeError(out altitudeError);

                        double speed = GetDoubleProperty(locationReport, LocationPropertyKey.Speed) * KnotsToMetersPerSec;
                        double course = GetDoubleProperty(locationReport, LocationPropertyKey.Heading);

                        coordinate = new GeoCoordinate(latitude, longitude, altitude, errorRadius, altitudeError, speed, course);
                    }
                }
            }
            
            //
            // Now see if there is civic address data in the report. We do this for both latlong and civic address
            // reports to handle the case of one sensor providing both types of data. Only generate a report if
            // countryRegion and at least one other string is present. For Win 7 a country string is always present 
            // because the native location provider API defaults to UserGeoID if no sensor provides one. 
            //
            string countryRegion = GetStringProperty(locationReport, LocationPropertyKey.CountryRegion);
            if (countryRegion != String.Empty)
            {
                string address1      = GetStringProperty(locationReport, LocationPropertyKey.AddressLine1);
                string address2      = GetStringProperty(locationReport, LocationPropertyKey.AddressLine2);
                string city          = GetStringProperty(locationReport, LocationPropertyKey.City);
                string postalCode    = GetStringProperty(locationReport, LocationPropertyKey.PostalCode);
                string stateProvince = GetStringProperty(locationReport, LocationPropertyKey.StateProvince);

                if (address1 != String.Empty || address2 != String.Empty || city != String.Empty || 
                    postalCode != String.Empty || stateProvince != String.Empty)
                {
                    address = new CivicAddress(address1, address2,  String.Empty, city, countryRegion, String.Empty, postalCode, stateProvince);
                }
            }

            //
            // if we have either coordinate or civic address report data, continue the processing 
            //
            if (!coordinate.IsUnknown || !address.IsUnknown)
            {
                GeoPositionStatus prevStatus = Status;

                //
                // Combine with civic address if there is one available
                //
                if (!coordinate.IsUnknown && (!address.IsUnknown))
                {
                    coordinate.m_address = address;
                }
                m_position = new GeoPosition<GeoCoordinate>(timestamp, coordinate);

                if (IsStarted)
                {
                    //
                    // Send a location change event if we are reporting a ready status. Otherwise, set
                    // an event pending flag which will cause the event to be sent when the status
                    // does switch to ready.
                    //
                    if (GeoPositionStatus.Ready == Status)
                    {
                        //
                        // The reported status may have changed because of the received data. If it
                        // has then generate a status change event.
                        //
                        if (Status != prevStatus)
                        {
                            OnPositionStatusChanged(new GeoPositionStatusChangedEventArgs(m_curStatus));
                        }

                        OnPositionChanged(new GeoPositionChangedEventArgs<GeoCoordinate>(m_position));
                    }
                    else
                    {
                        m_eventPending = true;
                    }
                }
                else
                {
                    //
                    // Fire Ready event when location is available in case Start() hasn't been called
                    //
                    if (GeoPositionStatus.Ready == m_curStatus)
                    {
                        OnPositionStatusChanged(new GeoPositionStatusChangedEventArgs(m_curStatus));
                    }
                }
            }
        }

        private void HandleLocationStatusChangedEvent(ReportStatus newStatus)
        {
            lock (this.InternalSyncObject)
            {
                //
                // If we are registered for civic address reports and a latlong provider
                // has become available, then unregister for civic address reports
                //
                if (m_civicAddrRegistered && m_latLongStatus != ReportStatus.NotSupported)
                {
                    if (m_civicAddrRegistered)
                    {
                        Guid reportType = LocationReportKey.CivicAddressReport;
                        if (m_location.UnregisterForReport(ref reportType) == 0)
                        {
                            m_civicAddrRegistered = false;
                        }
                    }
                }
            }

            GeoPositionStatus prevStatus = Status;
            
            //
            // Update current status
            //
            ReportStatus status = (ReportStatus.NotSupported != m_latLongStatus) ? m_latLongStatus : m_civicAddrStatus;
            m_curStatus = m_geoStatusMap[(int)status];

            ChangePermissionFromReportStatus(status);

            if (IsStarted)
            {
                //
                // If the reported status has changed, send a status change event
                //
                if (prevStatus != Status)
                {
                    OnPositionStatusChanged(new GeoPositionStatusChangedEventArgs(Status));

                    //
                    // If we have location data and have held off sending it until the
                    // status becomes ready, send the location event now
                    //
                    if (GeoPositionStatus.Ready == Status && m_eventPending)
                    {
                        m_eventPending = false;
                        OnPositionChanged(new GeoPositionChangedEventArgs<GeoCoordinate>(m_position));
                    }

                    // 
                    // If switching to ready from a permission denied state, then the sensor may not
                    // send a location changed event. So start a worker thread to get it via a
                    // synchronous request for current data.
                    // (Note: Check m_curStatus rather than Status because Status won't be Ready
                    // if there is no location data.) 
                    //
                    if (m_curStatus == GeoPositionStatus.Ready)
                    {
                        ThreadPool.QueueUserWorkItem(new WaitCallback(this.GetLocationData), null);
                    }
                }
            }
        }

        private double GetDoubleProperty(ILocationReport report, PROPERTYKEY propkey)
        {
            double val = double.NaN;

            PROPVARIANT pv = new PROPVARIANT();
            using (pv)
            {
                if (0 == report.GetValue(ref propkey, pv))
                {
                    if (pv.vt == VarEnum.VT_R8)
                    {
                        val = (double)pv.GetValue();
                    }
                }
            }

            return val;
        }

        private string GetStringProperty(ILocationReport report, PROPERTYKEY propkey)
        {
            string val = String.Empty;

            PROPVARIANT pv = new PROPVARIANT();
            using (pv)
            {
                int hr = report.GetValue(ref propkey, pv);
                if (0 == hr)
                {
                    if (pv.vt == VarEnum.VT_LPWSTR)
                    {
                        val = (string)pv.GetValue();
                    }
                }
            }

            return val;
        }

        private Object InternalSyncObject
        {
            get
            {
                if (m_lock == null)
                {
                    Object o = new Object();
                    Interlocked.CompareExchange<Object>(ref m_lock, o, null);
                }
                return m_lock;
            }
        }

        //
        // Enter processing of a callback.  If the return value is false then
        // immediately exit the callback.
        //
        private bool EnterProcessing()
        {
        
            // If clean-up has started, do not process any more events:
            if (m_dontProcessPositionEvents)
                return false;
            
            // Increment in-flight counter so we know how many events need to complete before we can start shut-down:
            if (Interlocked.Increment(ref m_positionEventsProcessingCount) == Int32.MaxValue - 1)
            {
                // Protect against counter overflow:
                Interlocked.Decrement(ref m_positionEventsProcessingCount);
                return false;                
            }

            return true;
        }

        private void ExitProcessing()
        {
            Interlocked.Decrement(ref m_positionEventsProcessingCount);
        }
        
#if !SILVERLIGHT
        // On Desktop, SpinWait.SpinUntil is available and we can use that to
        // spin until m_positionEventsProcessingCount is 0. This private method 
        // just checks for that.
        private bool NoPositionEventsCurrentlyProcessing()
        {
            return Volatile.Read(ref m_positionEventsProcessingCount) == 0;
        }
#else // if SILVERLIGHT is defined
        // On Silverlight, SpinWait.SpinUntil is NOT available and we cannot use that to
        // spin until m_positionEventsProcessingCount is 0. Instead we approximate the logic of
        // spin until with this private method.
        private void SpinUntil_NoPositionEventsCurrentlyProcessing()
        {        
            // Loop until we ae done processing all position events that we already started processing:
            while (Volatile.Read(ref m_positionEventsProcessingCount) > 0)
            {
            
                // Try a busy wait first:
                for (Int32 i = 0; i < 3; i++)
                {
                    if (Volatile.Read(ref m_positionEventsProcessingCount) == 0)
                        return;
                }
                
                // Try yielding a few times:
                for (Int32 i = 0; i < 5; i++)
                {
                    Thread.Sleep(1);
                    if (Volatile.Read(ref m_positionEventsProcessingCount) == 0)
                        return;
                }
            
                // This seems to take a while. Get off the CPU to allow other stuff to run:
                Thread.Sleep(100);
            }                        
        }        
#endif  // !SILVERLIGHT
        
        #endregion

        #region IDisposable

        public void Dispose() 
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~GeoCoordinateWatcherInternal()
        {
            Dispose(false);
        }

        private void Dispose(Boolean disposing)
        {
            Utility.Trace("GeoCoordinateWatcherInternal.Dispose(disposing=)" + disposing.ToString() + ")");
            
            // Do not notify user of any position events that may occur concurrently with the cleanup:
            m_dontProcessPositionEvents = true;

            if (!m_disposed)
            {
                if (disposing)
                {
                    Cleanup();
                }

                m_disposed = true;
            }
        }

        [SecuritySafeCritical]
        private void Cleanup()
        {
#if !SILVERLIGHT
            SpinWait.SpinUntil(NoPositionEventsCurrentlyProcessing);
#else
            SpinUntil_NoPositionEventsCurrentlyProcessing();
#endif  // !SILVERLIGHT
                        
            lock (this.InternalSyncObject)
            {
                if (m_location != null)
                {
                    //
                    // unregister for reports and status
                    //
                    if (m_latLongRegistered)
                    {
                        Guid reportType = LocationReportKey.LatLongReport;
                        if (m_location.UnregisterForReport(ref reportType) == 0)
                        {
                            m_latLongRegistered = false;
                        }
                    }

                    if (m_civicAddrRegistered)
                    {
                        Guid reportType = LocationReportKey.CivicAddressReport;
                        if (m_location.UnregisterForReport(ref reportType) == 0)
                        {
                            m_civicAddrRegistered = false;
                        }
                    }
#if !SILVERLIGHT
                    Marshal.ReleaseComObject(m_location);
#endif
                    m_location = null;
                }

                if (m_eventGetLocDone != null)
                {
                    m_eventGetLocDone.Close();
                    m_eventGetLocDone = null;
                }
            }
        }

        #endregion
    }
}
