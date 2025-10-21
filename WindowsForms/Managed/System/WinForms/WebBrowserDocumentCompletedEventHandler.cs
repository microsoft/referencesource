//------------------------------------------------------------------------------
// <copyright file="WebBrowserDocumentCompletedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Net;

namespace System.Windows.Forms {

    /// <include file='doc\WebBrowserDocumentCompletedEventHandler.uex' path='docs/doc[@for="WebBrowserDocumentCompletedEventHandler"]/*' />
    /// <devdoc>
    ///     <para>
    /// Delegate to the WebBrowser DocumentCompleted event.
    ///     </para>
    /// </devdoc>
    public delegate void WebBrowserDocumentCompletedEventHandler(object sender, WebBrowserDocumentCompletedEventArgs e);

    /// <include file='doc\WebBrowserDocumentCompletedEventHandler.uex' path='docs/doc[@for="WebBrowserDocumentCompletedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    /// Provides data for the <see cref='System.Windows.Forms.WebBrowser.OnDocumentCompleted'/> event.
    ///    </para>
    /// </devdoc>
    public class WebBrowserDocumentCompletedEventArgs : EventArgs {
        private Uri url;

        /// <include file='doc\WebBrowserDocumentCompletedEventHandler.uex' path='docs/doc[@for="WebBrowserDocumentCompletedEventArgs.WebBrowserDocumentCompletedEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        /// Creates an instance of the <see cref='System.Windows.Forms.WebBrowserDocumentCompletedEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public WebBrowserDocumentCompletedEventArgs(Uri url) {
            this.url = url;
        }
        
        /// <include file='doc\WebBrowserDocumentCompletedEventHandler.uex' path='docs/doc[@for="WebBrowserDocumentCompletedEventArgs.Url"]/*' />
        /// <devdoc>
        ///    <para>
        /// Url of the Document.
        ///    </para>
        /// </devdoc>
        public Uri Url {
            get {
                WebBrowser.EnsureUrlConnectPermission(url);
                return this.url;
            }
        }
    }
}

