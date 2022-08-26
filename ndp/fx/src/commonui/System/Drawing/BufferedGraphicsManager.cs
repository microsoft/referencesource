
//------------------------------------------------------------------------------
// <copyright file="BufferedGraphics.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Text;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.ConstrainedExecution;
    
    /// <include file='doc\BufferedGraphicsManager.uex' path='docs/doc[@for="BufferedGraphicsManager"]/*' />
    /// <devdoc>
    ///         The BufferedGraphicsManager is used for accessing a BufferedGraphicsContext.
    /// </devdoc>
    public sealed class BufferedGraphicsManager {
        private static BufferedGraphicsContext bufferedGraphicsContext;

        /// <include file='doc\BufferedGraphicsManager.uex' path='docs/doc[@for="BufferedGraphicsManager.BufferedGraphicsManager"]/*' />
        /// <devdoc>
        ///         Private constructor.
        /// </devdoc>
        private BufferedGraphicsManager() {
        }

        /// <include file='doc\BufferedGraphicsManager.uex' path='docs/doc[@for="BufferedGraphicsManager.BufferedGraphicsManager"]/*' />
        /// <devdoc>
        ///         Static constructor.  Here, we hook the exit & unload events so we can clean up our context buffer.
        /// </devdoc>
        static BufferedGraphicsManager() {
            AppDomain.CurrentDomain.ProcessExit += new EventHandler(BufferedGraphicsManager.OnShutdown);
            AppDomain.CurrentDomain.DomainUnload += new EventHandler(BufferedGraphicsManager.OnShutdown);
            bufferedGraphicsContext = new BufferedGraphicsContext();
        }

        /// <include file='doc\BufferedGraphicsManager.uex' path='docs/doc[@for="BufferedGraphicsManager.Current"]/*' />
        /// <devdoc>
        ///         Retrieves the context associated with the app domain.
        /// </devdoc>
        public static BufferedGraphicsContext Current {
            get {
                return bufferedGraphicsContext;
            }
        }

        /// <include file='doc\BufferedGraphicsManager.uex' path='docs/doc[@for="BufferedGraphicsManager.OnProcessExit"]/*' />
        /// <devdoc>
        ///         Called on process exit
        /// </devdoc>
        [PrePrepareMethod]
        private static void OnShutdown(object sender, EventArgs e) {
            BufferedGraphicsManager.Current.Invalidate();
        }
    }
}
