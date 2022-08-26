//------------------------------------------------------------------------------
// <copyright file="QueryPageSettingsEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    
    /// <include file='doc\QueryPageSettingsEventArgs.uex' path='docs/doc[@for="QueryPageSettingsEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='E:System.Drawing.Printing.PrintDocument.QueryPageSettings'/> event.
    ///    </para>
    /// </devdoc>
    public class QueryPageSettingsEventArgs : PrintEventArgs {
        private PageSettings pageSettings;
        
        /// <summary>
        /// It's too expensive to compare 2 instances of PageSettings class, as the getters
        /// are accessing the printer spooler, thus we track any explicit invocations of the setters or getters on this class,
        /// and this field tracks if PageSettings property was accessed. It will return a false 
        /// positive when the user is reading property values, but we'll take a perf hit in this case assuming this event is not 
        /// used often.
        internal bool PageSettingsChanged;

        /// <include file='doc\QueryPageSettingsEventArgs.uex' path='docs/doc[@for="QueryPageSettingsEventArgs.QueryPageSettingsEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.QueryPageSettingsEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public QueryPageSettingsEventArgs(PageSettings pageSettings) : base() {
            this.pageSettings = pageSettings;
        }

        /// <include file='doc\QueryPageSettingsEventArgs.uex' path='docs/doc[@for="QueryPageSettingsEventArgs.PageSettings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the page settings for the page to be printed.
        ///    </para>
        /// </devdoc>
        public PageSettings PageSettings {
            get {
                PageSettingsChanged = true;
                return pageSettings;
            }
            set {
                if (value == null) {
                    value = new PageSettings();
                }
                pageSettings = value;
                PageSettingsChanged = true;
            }
        }
    }
}

