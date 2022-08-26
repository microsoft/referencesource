//------------------------------------------------------------------------------
// <copyright file="PrinterResolution.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;    
    using System;    
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Globalization;

    /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution"]/*' />
    /// <devdoc>
    ///    <para> Retrieves
    ///       the resolution supported by a printer.</para>
    /// </devdoc>
    [Serializable]
    public class PrinterResolution {
        private int x;
        private int y;
        private PrinterResolutionKind kind;

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.PrinterResolution"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Printing.PrinterResolution'/> class with default properties.
        ///       This constructor is required for the serialization of the <see cref='System.Drawing.Printing.PrinterResolution'/> class.
        ///    </para>
        /// </devdoc>
        public PrinterResolution()
        {
            this.kind = PrinterResolutionKind.Custom;          
        }

        internal PrinterResolution(PrinterResolutionKind kind, int x, int y) {
            this.kind = kind;
            this.x = x;
            this.y = y;
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.Kind"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating the kind of printer resolution.
        ///       Setter added to enable serialization of the PrinterSettings object.
        ///    </para>
        /// </devdoc>
        public PrinterResolutionKind Kind {
            get { return kind;}
            set {
                //valid values are 0xfffffffc to 0x0
                if (!ClientUtils.IsEnumValid(value, unchecked((int)value), unchecked((int)PrinterResolutionKind.High), unchecked((int)PrinterResolutionKind.Custom)))
                {
                    throw new InvalidEnumArgumentException("value", unchecked((int)value), typeof(PrinterResolutionKind));
                }

                kind = value; 
            }
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.X"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the printer resolution in the horizontal direction,
        ///       in dots per inch.
        ///       Setter added to enable serialization of the PrinterSettings object.
        ///    </para>
        /// </devdoc>
        public int X {
            get {
                return x;
            }
            set {
                x = value;
            }
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.Y"]/*' />
        /// <devdoc>
        ///    <para> Gets the printer resolution in the vertical direction,
        ///       in dots per inch.
        ///       Setter added to enable serialization of the PrinterSettings object.
        ///    </para>
        /// </devdoc>
        public int Y {
            get {
                return y;
            }
            set {
                y = value;
            }
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information about the PrinterResolution in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            if (kind != PrinterResolutionKind.Custom)
                return "[PrinterResolution " + TypeDescriptor.GetConverter(typeof(PrinterResolutionKind)).ConvertToString((int) Kind)
                + "]";
            else
                return "[PrinterResolution"
                + " X=" + X.ToString(CultureInfo.InvariantCulture)
                + " Y=" + Y.ToString(CultureInfo.InvariantCulture)
                + "]";
        }
    }
}
