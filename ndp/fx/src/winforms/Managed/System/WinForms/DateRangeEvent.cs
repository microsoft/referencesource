//------------------------------------------------------------------------------
// <copyright file="DateRangeEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\DateRangeEvent.uex' path='docs/doc[@for="DateRangeEventArgs"]/*' />
    /// <devdoc>
    ///     The SelectEvent is fired when the user makes an explicit date
    ///     selection within a month calendar control.
    /// </devdoc>
    public class DateRangeEventArgs : EventArgs {

        readonly DateTime start; // The date for the first day in the user's selection range.
        readonly DateTime end;   // The date for the last day in the user's selection range.

        /// <include file='doc\DateRangeEvent.uex' path='docs/doc[@for="DateRangeEventArgs.DateRangeEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateRangeEventArgs(DateTime start, DateTime end) {
            this.start = start;
            this.end = end;
        }

        /// <include file='doc\DateRangeEvent.uex' path='docs/doc[@for="DateRangeEventArgs.Start"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime Start {
            get { return start; }
        }
        /// <include file='doc\DateRangeEvent.uex' path='docs/doc[@for="DateRangeEventArgs.End"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime End {
            get { return end; }
        }
    }
}
