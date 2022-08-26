//-----------------------------------------------------------------------------------------
// <copyright file="ListViewVirtualItemsSelectionRangeChangedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//-----------------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms
{
    using System.ComponentModel;

    /// <include file='doc\ListViewVirtualItemsSelectionRangeChangedEvent.uex' path='docs/doc[@for="ListViewVirtualItemsSelectionRangeChangedEventArgs"]/*' />
    /// <devdoc>
    /// The event class that is created when the selection state of a ListViewItem is changed.
    /// </devdoc>
    public class ListViewVirtualItemsSelectionRangeChangedEventArgs : EventArgs
    {
        private int startIndex;
        private int endIndex;
        private bool isSelected;
        
        /// <include file='doc\ListViewVirtualItemsSelectionRangeChangedEvent.uex' path='docs/doc[@for="ListViewVirtualItemsSelectionRangeChangedEventArgs.ListViewVirtualItemsSelectionRangeChangedEventArgs"]/*' />
        /// <devdoc>
        /// Constructs a ListViewVirtualItemsSelectionRangeChangedEventArgs object.
        /// </devdoc>
        public ListViewVirtualItemsSelectionRangeChangedEventArgs(int startIndex, int endIndex, bool isSelected)
        {
            if (startIndex > endIndex)
            {
                throw new ArgumentException(SR.GetString(SR.ListViewStartIndexCannotBeLargerThanEndIndex));
            }
            this.startIndex = startIndex;
            this.endIndex = endIndex;
            this.isSelected = isSelected;
        }

        /// <include file='doc\ListViewVirtualItemsSelectionRangeChangedEvent.uex' path='docs/doc[@for="ListViewVirtualItemsSelectionRangeChangedEventArgs.EndIndex"]/*' />
        /// <devdoc>
        /// Returns the end of the range where the selection changed
        /// </devdoc>
        public int EndIndex
        {
            get
            {
                return this.endIndex;
            }
        }

        /// <include file='doc\ListViewVirtualItemsSelectionRangeChangedEvent.uex' path='docs/doc[@for="ListViewVirtualItemsSelectionRangeChangedEventArgs.IsSelected"]/*' />
        /// <devdoc>
        /// Return true if the items are selected
        /// </devdoc>
        public bool IsSelected
        {
            get
            {
                return this.isSelected;
            }
        }

        /// <include file='doc\ListViewVirtualItemsSelectionRangeChangedEvent.uex' path='docs/doc[@for="ListViewVirtualItemsSelectionRangeChangedEventArgs.StartIndex"]/*' />
        /// <devdoc>
        /// Returns the begining of the range where the selection changed
        /// </devdoc>
        public int StartIndex
        {
            get
            {
                return this.startIndex;
            }
        }
    }
}
