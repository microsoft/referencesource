//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    /// Provides data for the DateSelected and DisplayDateChanged events.
    /// </summary>
    public class CalendarDateChangedEventArgs : System.Windows.RoutedEventArgs
    {
        internal CalendarDateChangedEventArgs(DateTime? removedDate, DateTime? addedDate)
        {
            this.RemovedDate = removedDate;
            this.AddedDate = addedDate;
        }

        /// <summary>
        /// Gets the date to be newly displayed.
        /// </summary>
        public DateTime? AddedDate
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the date that was previously displayed.
        /// </summary>
        public DateTime? RemovedDate
        {
            get;
            private set;
        }
    }
}
