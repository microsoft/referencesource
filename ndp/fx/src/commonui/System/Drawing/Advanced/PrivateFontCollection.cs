//------------------------------------------------------------------------------
// <copyright file="PrivateFontCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Text {

    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Drawing.Internal;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
    using System.Globalization;
    using System.Runtime.Versioning;
    using System.Collections.Generic;

    /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection"]/*' />
    /// <devdoc>
    ///    Encapsulates a collection of <see cref='System.Drawing.Font'/> objecs.
    /// </devdoc>
    public sealed class PrivateFontCollection : FontCollection {

        private List<string> gdiFonts = null;

        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.PrivateFontCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Text.PrivateFontCollection'/> class.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public PrivateFontCollection() {
            nativeFontCollection = IntPtr.Zero;

            int status = SafeNativeMethods.Gdip.GdipNewPrivateFontCollection(out nativeFontCollection);

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);

            if (!LocalAppContextSwitches.DoNotRemoveGdiFontsResourcesFromFontCollection)
                gdiFonts = new List<string>();
        }

        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Cleans up Windows resources for this
        ///    <see cref='System.Drawing.Text.PrivateFontCollection'/> .
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (nativeFontCollection != IntPtr.Zero) {
                try{
#if DEBUG
                    int status = 
#endif            
                    SafeNativeMethods.Gdip.GdipDeletePrivateFontCollection(out nativeFontCollection);
#if DEBUG
                    Debug.Assert(status == SafeNativeMethods.Gdip.Ok, "GDI+ returned an error status: " + status.ToString(CultureInfo.InvariantCulture));
#endif
                    if (gdiFonts != null) {
                        foreach (string fontFile in gdiFonts) {
                            SafeNativeMethods.RemoveFontFile(fontFile);
                        }
                        gdiFonts.Clear();
                        gdiFonts = null;
                    }
                }
                catch ( Exception ex ){
                    if( ClientUtils.IsSecurityOrCriticalException( ex ) ) {
                        throw;
                    }

                    Debug.Fail( "Exception thrown during Dispose: " + ex.ToString() );
                }
                finally{
                    nativeFontCollection = IntPtr.Zero;
                }
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\PrivateFontCollection.uex' path='docs/doc[@for="PrivateFontCollection.AddFontFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a font from the specified file to
        ///       this <see cref='System.Drawing.Text.PrivateFontCollection'/>.
        ///    </para>
        /// </devdoc>
        public void AddFontFile (string filename) {
            IntSecurity.DemandReadFileIO(filename);
            int status = SafeNativeMethods.Gdip.GdipPrivateAddFontFile(new HandleRef(this, nativeFontCollection), filename);

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);

            // Register private font with GDI as well so pure GDI-based controls (TextBox, Button for instance) can access it.
            if (SafeNativeMethods.AddFontFile(filename) != 0) {
                if (gdiFonts != null) {
                    gdiFonts.Add(filename);
                }
            }
        }

        // 









        public void AddMemoryFont (IntPtr memory, int length) {
            IntSecurity.ObjectFromWin32Handle.Demand();

            int status = SafeNativeMethods.Gdip.GdipPrivateAddMemoryFont(new HandleRef(this, nativeFontCollection), new HandleRef(null, memory), length);

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);
        }
    }
}

