//------------------------------------------------------------------------------
// <copyright file="DateBoldEvent.cs" company="Microsoft">
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

    /// <include file='doc\DateBoldEvent.uex' path='docs/doc[@for="DateBoldEventArgs"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///     The month calendar control fires this event to request information
    ///     about how the days within the visible months should be displayed.
    /// </devdoc>
    //
    // 
    public class DateBoldEventArgs : EventArgs {
        readonly DateTime   startDate;  //the starting date
        readonly int        size; // requested length of array
        int[]            daysToBold = null;

        internal DateBoldEventArgs(DateTime start, int size) {
            startDate = start;
            this.size = size;
        }
        /// <include file='doc\DateBoldEvent.uex' path='docs/doc[@for="DateBoldEventArgs.StartDate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime StartDate {
            get { return startDate; }
        }
        /// <include file='doc\DateBoldEvent.uex' path='docs/doc[@for="DateBoldEventArgs.Size"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Size {
            get { return size; }
        }
        /// <include file='doc\DateBoldEvent.uex' path='docs/doc[@for="DateBoldEventArgs.DaysToBold"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int[] DaysToBold {
            get { return daysToBold; }
            set { daysToBold = value; }
        }
    }
}
