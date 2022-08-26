//-----------------------------------------------------------------------------
//
// <copyright file="TrackingMemoryStreamFactory.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  This is a basic implementation of the ITrackingMemoryStreamFactory interface
//
// History:
//  07/4/2005: IgorBel: Initial creation.
//  11/08/2005: BruceMac: Change namespace
//
//-----------------------------------------------------------------------------

using System;
using System.IO;
using System.Diagnostics;

namespace MS.Internal.IO.Packaging
{   
    /// <summary>
    /// TrackingMemoryStreamFactory class is used in the Sparse Memory Stream to keep track of the memory Usage 
    /// </summary>
    internal class TrackingMemoryStreamFactory : ITrackingMemoryStreamFactory
    {

        public MemoryStream Create()
        {   
            return new TrackingMemoryStream((ITrackingMemoryStreamFactory)this);
        }

        public MemoryStream Create(int capacity)
        {   
            return new TrackingMemoryStream((ITrackingMemoryStreamFactory)this, capacity);
        }

        public void ReportMemoryUsageDelta(int delta)
        {   
            checked{_bufferedMemoryConsumption += delta;}
            Debug.Assert(_bufferedMemoryConsumption >=0, "we end up having buffers of negative size");
        }

        internal long CurrentMemoryConsumption
        {
            get
            {
                return _bufferedMemoryConsumption;
            }
        }

        private long _bufferedMemoryConsumption;
    }
} 
