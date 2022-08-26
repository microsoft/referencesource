//------------------------------------------------------------------------------
// <copyright file="SolidBrush.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;    
    using System.ComponentModel;
    using System.Drawing.Internal;
    using System.Runtime.Versioning;

    /// <include file='doc\SolidBrush.uex' path='docs/doc[@for="SolidBrush"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines a brush made up of a single color. Brushes are
    ///       used to fill graphics shapes such as rectangles, ellipses, pies, polygons, and paths.
    ///    </para>
    /// </devdoc>
    public sealed class SolidBrush : Brush, ISystemColorTracker {
        // GDI+ doesn't understand system colors, so we need to cache the value here
        private Color color = Color.Empty;
        private bool immutable;

        /**
         * Create a new solid fill brush object
         */
        /// <include file='doc\SolidBrush.uex' path='docs/doc[@for="SolidBrush.SolidBrush"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.SolidBrush'/> class of the specified
        ///       color.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public SolidBrush(Color color)  
        {
            this.color = color;

            IntPtr brush = IntPtr.Zero;
            int status = SafeNativeMethods.Gdip.GdipCreateSolidFill(this.color.ToArgb(), out brush);

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);

            SetNativeBrushInternal(brush);

            if (color.IsSystemColor)
                SystemColorTracker.Add(this);
        }

        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        internal SolidBrush(Color color, bool immutable) : this(color) {
            this.immutable = immutable;
        }

        /// <devdoc>
        ///     Constructor to initialized this object from a GDI+ Brush native pointer.
        /// </devdoc>
        internal SolidBrush( IntPtr nativeBrush )
        {
            Debug.Assert( nativeBrush != IntPtr.Zero, "Initializing native brush with null." );
            SetNativeBrushInternal( nativeBrush );
        }

        /// <include file='doc\SolidBrush.uex' path='docs/doc[@for="SolidBrush.Clone"]/*' />
        /// <devdoc>
        ///    Creates an exact copy of this <see cref='System.Drawing.SolidBrush'/>.
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public override object Clone() 
        {
            IntPtr cloneBrush = IntPtr.Zero;

            int status = SafeNativeMethods.Gdip.GdipCloneBrush(new HandleRef(this, this.NativeBrush), out cloneBrush);

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);

            // We intentionally lose the "immutable" bit.

            return new SolidBrush(cloneBrush);
        }

        
        /// <include file='doc\SolidBrush.uex' path='docs/doc[@for="SolidBrush.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            if (!disposing) {
                immutable = false;
            }
            else if (immutable) {
                throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Brush"));
            }
            
            base.Dispose(disposing);
        }
        
        /// <include file='doc\SolidBrush.uex' path='docs/doc[@for="SolidBrush.Color"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The color of this <see cref='System.Drawing.SolidBrush'/>.
        ///    </para>
        /// </devdoc>
        public Color Color {
            get
            {
                if (this.color == Color.Empty)
                {
                    int colorARGB = 0;
                    int status = SafeNativeMethods.Gdip.GdipGetSolidFillColor(new HandleRef(this, this.NativeBrush), out colorARGB);

                    if (status != SafeNativeMethods.Gdip.Ok)
                        throw SafeNativeMethods.Gdip.StatusException(status);

                    this.color = Color.FromArgb(colorARGB);
                }

                // GDI+ doesn't understand system colors, so we can't use GdipGetSolidFillColor in the general case
                return this.color;
            }

            set 
            { 
                if (immutable)
                {
                    throw new ArgumentException(SR.GetString(SR.CantChangeImmutableObjects, "Brush"));
                }

                if( this.color != value )
                {
                    Color oldColor = this.color;
                    InternalSetColor(value);

                    // 

                    if (value.IsSystemColor && !oldColor.IsSystemColor)
                    {
                        SystemColorTracker.Add(this);
                    }
                }
            }
        }

        // Sets the color even if the brush is considered immutable
        private void InternalSetColor(Color value) {
            int status = SafeNativeMethods.Gdip.GdipSetSolidFillColor(new HandleRef(this, this.NativeBrush), value.ToArgb());

            if (status != SafeNativeMethods.Gdip.Ok)
                throw SafeNativeMethods.Gdip.StatusException(status);

            this.color = value;
        }

        /// <include file='doc\SolidBrush.uex' path='docs/doc[@for="SolidBrush.ISystemColorTracker.OnSystemColorChanged"]/*' />
        /// <internalonly/>
        void ISystemColorTracker.OnSystemColorChanged() {
            if( this.NativeBrush != IntPtr.Zero ){    
                InternalSetColor(color);
            }
        }
    }
}

