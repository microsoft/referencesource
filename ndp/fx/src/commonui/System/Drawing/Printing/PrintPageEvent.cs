//------------------------------------------------------------------------------
// <copyright file="PrintPageEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System;
    using System.Drawing;

    /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='E:System.Drawing.Printing.PrintDocument.PrintPage'/>
    /// event.</para>
    /// </devdoc>
    // NOTE: Please keep this class consistent with PaintEventArgs.
    public class PrintPageEventArgs : EventArgs {
        private bool hasMorePages;
        private bool cancel;

        private Graphics graphics;
        private readonly Rectangle marginBounds;
        private readonly Rectangle pageBounds;
        private readonly PageSettings pageSettings;

        // Apply page settings to the printer.
        internal bool CopySettingsToDevMode = true;


        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.PrintPageEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Drawing.Printing.PrintPageEventArgs'/> class.</para>
        /// </devdoc>
        public PrintPageEventArgs(Graphics graphics, Rectangle marginBounds, Rectangle pageBounds, PageSettings pageSettings) {
            this.graphics = graphics; // may be null, see PrintController
            this.marginBounds = marginBounds;
            this.pageBounds = pageBounds;
            this.pageSettings = pageSettings;
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.Cancel"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the print job should be canceled.</para>
        /// </devdoc>
        public bool Cancel {
            get { return cancel;}
            set { cancel = value;}
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.Graphics"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the <see cref='System.Drawing.Graphics'/>
        ///       used to paint the
        ///       item.
        ///    </para>
        /// </devdoc>
        public Graphics Graphics {
            get {
                return graphics;
            }
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.HasMorePages"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets a value indicating whether an additional page should
        ///       be printed.</para>
        /// </devdoc>
        public bool HasMorePages {
            get { return hasMorePages;}
            set { hasMorePages = value;}
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.MarginBounds"]/*' />
        /// <devdoc>
        ///    <para>Gets the rectangular area that represents the portion of the page between the margins.</para>
        /// </devdoc>
        public Rectangle MarginBounds {
            get {
                return marginBounds;
            }
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.PageBounds"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the rectangular area that represents the total area of the page.
        ///    </para>
        /// </devdoc>
        public Rectangle PageBounds {
            get {
                return pageBounds;
            }
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.PageSettings"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the page settings for the current page.</para>
        /// </devdoc>
        public PageSettings PageSettings {
            get {
                return pageSettings;
            }
        }

        /// <include file='doc\PrintPageEvent.uex' path='docs/doc[@for="PrintPageEventArgs.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes
        ///       of the resources (other than memory) used by
        ///       the <see cref='System.Drawing.Printing.PrintPageEventArgs'/>.</para>
        /// </devdoc>
        // We want a way to dispose the GDI+ Graphics, but we don't want to create one
        // simply to dispose it
        internal void Dispose() {
            graphics.Dispose();
        }

        internal void SetGraphics(Graphics value) {
            this.graphics = value;
        }
    }
}

