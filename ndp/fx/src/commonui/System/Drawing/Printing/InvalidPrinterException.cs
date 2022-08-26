//------------------------------------------------------------------------------
// <copyright file="InvalidPrinterException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;

    /// <include file='doc\InvalidPrinterException.uex' path='docs/doc[@for="InvalidPrinterException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents
    ///       the
    ///       exception that is thrown when trying to access a printer using invalid printer settings.
    ///    </para>
    /// </devdoc>
    [Serializable()]
    public class InvalidPrinterException : SystemException {
        private PrinterSettings settings;

        /// <include file='doc\InvalidPrinterException.uex' path='docs/doc[@for="InvalidPrinterException.InvalidPrinterException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.InvalidPrinterException'/> class.
        ///    </para>
        /// </devdoc>
        public InvalidPrinterException(PrinterSettings settings) 
        : base(GenerateMessage(settings)) 
        {
            this.settings = settings;
        }

        /// <include file='doc\InvalidPrinterException.uex' path='docs/doc[@for="InvalidPrinterException.InvalidPrinterException1"]/*' />
        protected InvalidPrinterException(SerializationInfo info, StreamingContext context) : base (info, context) {
            settings = (PrinterSettings)info.GetValue("settings", typeof(PrinterSettings));
        }

        /// <include file='doc\InvalidPrinterException.uex' path='docs/doc[@for="InvalidPrinterException.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand,SerializationFormatter=true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            IntSecurity.AllPrinting.Demand();
            info.AddValue("settings", settings);
            base.GetObjectData(info, context);
        }

        static string GenerateMessage(PrinterSettings settings) {
            if (settings.IsDefaultPrinter) {
                return SR.GetString(SR.InvalidPrinterException_NoDefaultPrinter);
            }
            else {
                try {
                    return SR.GetString(SR.InvalidPrinterException_InvalidPrinter, settings.PrinterName);
                }
                catch (SecurityException) {
                    return SR.GetString(SR.InvalidPrinterException_InvalidPrinter, SR.GetString(SR.CantTellPrinterName));
                }
            }
        }
    }
}

