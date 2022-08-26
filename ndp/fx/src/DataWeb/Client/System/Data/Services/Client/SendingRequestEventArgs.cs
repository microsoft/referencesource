//---------------------------------------------------------------------
// <copyright file="SendingRequestEventArgs.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Event args for the event fired before executing a web request. Gives a 
// chance to customize or replace the request object to be used.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Diagnostics;

    /// <summary>
    /// Event args for the event fired before executing a web request. Gives a 
    /// chance to customize or replace the request object to be used.
    /// </summary>
    public class SendingRequestEventArgs : EventArgs
    {
        /// <summary>The web request reported through this event</summary>
#if ASTORIA_LIGHT
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification = "Not used in Silverlight")]
#endif
        private System.Net.WebRequest request;

        /// <summary>The request header collection.</summary>
        private System.Net.WebHeaderCollection requestHeaders;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="request">The request reported through this event</param>
        /// <param name="requestHeaders">The request header collection.</param>
        internal SendingRequestEventArgs(System.Net.WebRequest request, System.Net.WebHeaderCollection requestHeaders)
        {
            // In Silverlight the request object is not accesible
#if ASTORIA_LIGHT
            Debug.Assert(null == request, "non-null request in SL.");
#else
            Debug.Assert(null != request, "null request");
#endif
            Debug.Assert(null != requestHeaders, "null requestHeaders");
            this.request = request;
            this.requestHeaders = requestHeaders;
        }

#if !ASTORIA_LIGHT // Data.Services http stack
        /// <summary>The web request reported through this event. The handler may modify or replace it.</summary>
        public System.Net.WebRequest Request
        {
            get
            {
                return this.request;
            }

            set
            {
                Util.CheckArgumentNull(value, "value");
                if (!(value is System.Net.HttpWebRequest))
                {
                    throw Error.Argument(Strings.Context_SendingRequestEventArgsNotHttp, "value");
                }

                this.request = value;
                this.requestHeaders = value.Headers;
            }
        }
#endif

        /// <summary>The request header collection.</summary>
        public System.Net.WebHeaderCollection RequestHeaders
        {
            get { return this.requestHeaders; }
        }
    }
}
