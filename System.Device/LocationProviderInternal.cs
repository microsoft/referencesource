// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: GeoLocationProviderInternal
**
** Purpose: Represents a GeoLocationProviderInternal implementations
**  This is platform dependant wrapper for the provider.  It seperates
**  GeoLocationProvider from underly Location platforms.
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
using System.Diagnostics.CodeAnalysis;


namespace System.Device.Location
{
    internal sealed class GeoLocationProviderInternal : GeoLocationProviderBase, ILocationEvents, IDisposable
    {
        private object m_lock;
        private ILocation m_location;

        private bool m_disposed;

        private bool m_started;
        private bool m_eventPending;

        private bool m_latLongRegistered;
        private bool m_civicAddrRegistered;

        private ManualResetEvent m_eventCreateDone;
        private ManualResetEvent m_eventGetLocDone;

        private ReportStatus m_latLongStatus   = ReportStatus.NotSupported; 
        private ReportStatus m_civicAddrStatus = ReportStatus.NotSupported;

        private GeoLocationStatus m_curStatus  = GeoLocationStatus.Disabled;
        private GeoLocation m_curLocation      = GeoLocation.Unknown;

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
                    m_location = new COMLocation();
                }
                catch (COMException)
                {
                    Utility.Trace("Failed to CoCreate ILocation COM object.");
                }

                if (null != m_location)
                {
                    Utility.Trace("Done creating ILocation COM object");

                    //
                    // Mapping to platform accuracy
                    //
                    GeoLocationAccuracy desiredAccuracy = (GeoLocationAccuracy)state;
                    DesiredAccuracy accuracy = (desiredAccuracy == GeoLocationAccuracy.Low) ?
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
                }

                Utility.DebugAssert(m_eventCreateDone != null, "m_eventCreateDone is null");

                ManualResetEvent eventDone = m_eventCreateDone;
                if (eventDone != null)
                {
                    eventDone.Set();
                }
            }
        }

        #region Constractor
        // <summary>
        // Creates an instance of GeoLocationProviderInternal with the specified desired accuracy
        // </summary>
        public GeoLocationProviderInternal(GeoLocationAccuracy desiredAccuracy)
        {
            //
            // Create the native location object on a worker thread, so that it exists
            // in a multithreaded apartment.
            //
            m_eventCreateDone = new ManualResetEvent(false); 

            ThreadPool.QueueUserWorkItem(new WaitCallback(this.CreateHandler), desiredAccuracy);
#if _DEBUG
            m_eventCreateDone.WaitOne(5000);
#else
            m_eventCreateDone.WaitOne(500);
#endif
            Utility.Trace("GeoLocationProviderInternal.ctor: " +
                          "desiredAccuracy: " + desiredAccuracy.ToString() +
                          " m_latLongRegistered: " + m_latLongRegistered.ToString() +
                          " m_civicAddrRegistered: " + m_civicAddrRegistered.ToString() +
                          " m_latLongStatus: " + m_latLongStatus.ToString() +
                          " m_curStatus: " + m_curStatus.ToString());
        }
        #endregion

        #region GeoLocationProviderBase overrides
        public override GeoLocationStatus Status 
        { 
           get 
           {
                return ((m_curStatus == GeoLocationStatus.Ready) && (m_curLocation == GeoLocation.Unknown)) ? GeoLocationStatus.NoData : m_curStatus;
           }
        }

        public override GeoLocation Location
        {
            get
            {
                //
                // Don't return location data unless we are started and the current status allows it
                //
                if (m_started && !(GeoLocationStatus.NoData == m_curStatus || GeoLocationStatus.NoPermissions == m_curStatus))
                {
                    return m_curLocation;
                }
                else
                {
                    return GeoLocation.Unknown;
                }
            }
        }

        public override void Start()
        {
            if (!m_started)
            {
                m_started = true;

                //
                // If the reported status is ready and we have location data then generate an event 
                // to provide the initial location data. Otherwise, we spown worker thread to obtain
                // the initial status and location, and timeout on 100m seconds.
                //
                if (Status == GeoLocationStatus.Ready)
                {
                    OnLocationChanged(new GeoLocationChangedEventArgs(m_curLocation));
                }
                else
                {
                    m_eventGetLocDone = new ManualResetEvent(false);

                    ThreadPool.QueueUserWorkItem(new WaitCallback(this.GetInitialLocationData), null);
#if _DEBUG
                    m_eventGetLocDone.WaitOne(300);
#else
                    m_eventGetLocDone.WaitOne(100);
#endif
                }
            }

            Utility.Trace("GeoLocationProviderInternal.Start:" +
                          " m_started: " + m_started.ToString() +
                          " Status: " + Status.ToString() +
                          " m_curLocation: " + m_curLocation.Coordinate.ToString());
        }


        //
        // Get the initial location data and set completion event when done.
        // This is executed on a worker thread.
        //
        private void GetInitialLocationData(object state)
        {
            lock (this.InternalSyncObject)
            {
                //
                // Wait for the creation thread to finish before querying for status
                //
                if (m_location != null)
                {
                    if (m_curStatus == GeoLocationStatus.Ready)
                    {
                        GetLocationHelper();
                    }
                    else
                    {
                        Guid reportType = (m_latLongStatus != ReportStatus.NotSupported) ?
                                           LocationReportKey.LatLongReport : LocationReportKey.CivicAddressReport;
                        m_location.RequestPermissions(IntPtr.Zero, ref reportType, 1, 0);
                        Utility.Trace("RequestPermission is called");
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
            Thread.Sleep(10);
            GetLocationHelper();
        }

        //
        // Make multiple attempts to get location data. Quit if the status changes from Ready
        // or location data is present.
        //
        private void GetLocationHelper()
        {
            for (int i = 0; i < 5; i++)
            {
                if (m_location == null || m_curStatus != GeoLocationStatus.Ready || m_curLocation != GeoLocation.Unknown)
                {
                    break;
                }

                Guid reportType = (m_latLongStatus != ReportStatus.NotSupported) ?
                                    LocationReportKey.LatLongReport : LocationReportKey.CivicAddressReport;

                ILocationReport report;
                if (m_location.GetReport(ref reportType, out report) == 0)
                {
                    HandleLocationChangedEvent(report);
                    break;
                }

                Thread.Sleep(8);
            }
        }


        /// <summary>
        /// Disconnects from location report events.
        /// </summary>
        public override void Stop()
        {
            m_started = false;

            Utility.Trace("GeoLocationProviderInternal.Stop");
        }
        #endregion

        #region ILocationEvents
        /// <summary>
        /// ILocationEvents.OnLocationChanged 
        /// </summary>
        /// <param name="reportType"></param>
        /// <param name="locationReport"></param>
        void ILocationEvents.OnLocationChanged(ref Guid reportType, ILocationReport locationReport)
        {
            if (reportType.Equals(LocationReportKey.LatLongReport) ||
                reportType.Equals(LocationReportKey.CivicAddressReport))
            {
                HandleLocationChangedEvent(locationReport);
            }
        }

        /// <summary>
        /// ILocationEvents.OnStatusChanged
        /// </summary>
        /// <param name="reportType"></param>
        /// <param name="newStatus"></param>
        void ILocationEvents.OnStatusChanged(ref Guid reportType, ReportStatus newStatus)
        {
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
        }
        #endregion

        #region Helpers
        // Report status to GeoLocation status look-up table
        private static GeoLocationStatus[] m_geoStatusMap = new GeoLocationStatus[] 
        {
            GeoLocationStatus.NoData,           // ReportStatus.NotSupported = 0,
            GeoLocationStatus.NoData,           // ReportStatus.Error = 1,
            GeoLocationStatus.NoPermissions,    // ReportStatus.AccessDenied = 2,
            GeoLocationStatus.Initializing,     // ReportStatus.Initializing = 3,
            GeoLocationStatus.Ready             // ReportStatus.Running = 4
        };

        private void HandleLocationChangedEvent(ILocationReport locationReport)
        {
            const double KnotsToMetersPerSec = 463.0 / 900.0;
 
            GeoCoordinate coordinate = GeoCoordinate.Unknown;
            CivicAddress address = CivicAddress.Unknown;

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

                        coordinate = new GeoCoordinate(latitude, longitude, errorRadius, altitude, altitudeError);
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
            if (coordinate != GeoCoordinate.Unknown || address != CivicAddress.Unknown)
            {
                double heading = GetDoubleProperty(locationReport, LocationPropertyKey.Heading);
                double speed = GetDoubleProperty(locationReport, LocationPropertyKey.Speed) * KnotsToMetersPerSec;

                DateTimeOffset timestamp = DateTimeOffset.Now;
                SYSTEMTIME systime;
                if (0 == locationReport.GetTimestamp(out systime))
                {
                    timestamp = new DateTimeOffset(systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                                                   systime.wMinute, systime.wSecond, systime.wMilliseconds, TimeSpan.Zero);
                }

                GeoLocationStatus prevStatus = Status;

                m_curLocation = new GeoLocation(coordinate, heading, speed, address, timestamp);

                if (m_started)
                {
                    //
                    // Send a location change event if we are reporting a ready status. Otherwise, set
                    // an event pending flag which will cause the event to be sent when the status
                    // does switch to ready.
                    //
                    if (GeoLocationStatus.Ready == Status)
                    {
                        //
                        // The reported status may have changed because of the received data. If it
                        // has then generate a status change event.
                        //
                        if (Status != prevStatus)
                        {
                            OnStatusChanged(new GeoLocationStatusChangedEventArgs(m_curStatus));
                        }

                        OnLocationChanged(new GeoLocationChangedEventArgs(m_curLocation));
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
                    if (GeoLocationStatus.Ready == m_curStatus)
                    {
                        OnStatusChanged(new GeoLocationStatusChangedEventArgs(m_curStatus));
                    }
                }
            }
        }

        private void HandleLocationStatusChangedEvent(ReportStatus newStatus)
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

            GeoLocationStatus prevStatus = Status;
            //
            // Update the current status
            //
            ReportStatus status = (ReportStatus.NotSupported != m_latLongStatus) ? m_latLongStatus : m_civicAddrStatus;
            m_curStatus = m_geoStatusMap[(int)status];

            //
            // If the reported status has changed, send a status change event
            //
            if (prevStatus != Status)
            {
                OnStatusChanged(new GeoLocationStatusChangedEventArgs(Status));

                //
                // If we have location data and have held off sending it until the
                // status becomes ready, send the location event now
                //
                if (GeoLocationStatus.Ready == Status && m_eventPending)
                {
                    m_eventPending = false;
                    OnLocationChanged(new GeoLocationChangedEventArgs(m_curLocation));
                }

                // 
                // If switching to ready from a permission denied state, then the sensor may not
                // send a location changed event. So start a worker thread to get it via a
                // synchronous request for current data.
                // (Note: Check m_curStatus rather than Status because Status won't be Ready
                // if there is no location data.) 
                //
                if (prevStatus == GeoLocationStatus.NoPermissions && m_curStatus == GeoLocationStatus.Ready) 
                {
                    ThreadPool.QueueUserWorkItem(new WaitCallback(this.GetLocationData), null);
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
        #endregion

        #region IDisposable

        public void Dispose() 
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~GeoLocationProviderInternal()
        {
            Dispose(false);
        }

        private void Dispose(Boolean disposing)
        {
            Utility.Trace("GeoLocationProviderInternal.Dispose(disposing=)" + disposing.ToString() + ")");

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

                if (m_eventCreateDone != null)
                {
                    m_eventCreateDone.Close();
                    m_eventCreateDone = null;
                }
            }
        }

        #endregion
    }
}
