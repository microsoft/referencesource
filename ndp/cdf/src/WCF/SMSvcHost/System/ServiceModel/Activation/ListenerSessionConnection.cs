//----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime.Diagnostics;
    using System.ServiceModel.Channels;

    class ListenerSessionConnection
    {
        IConnection connection;
        byte[] data;
        Action itemDequeuedCallback;
        Uri via;
        WorkerProcess worker;
        EventTraceActivity eventTraceActivity;

        internal ListenerSessionConnection(IConnection connection, byte[] data, Uri via,
                    Action itemDequeuedCallback)
        {
            this.connection = connection;
            this.data = data;
            this.via = via;
            this.itemDequeuedCallback = itemDequeuedCallback;            
        }
        internal IConnection Connection { get { return connection; } }
        internal byte[] Data { get { return data; } }
        internal Uri Via { get { return via; } }

        internal EventTraceActivity EventTraceActivity 
        { 
            get 
            {
                if (this.eventTraceActivity == null)
                {
                    this.eventTraceActivity = new EventTraceActivity();
                }
                return eventTraceActivity; 
            } 
        }

        internal WorkerProcess WorkerProcess
        {
            get
            {
                return this.worker;
            }

            set
            {
                this.worker = value;
            }
        }

        internal void TriggerDequeuedCallback()
        {
            if (this.itemDequeuedCallback != null)
            {
                this.itemDequeuedCallback();
            }
        }
    }
}
