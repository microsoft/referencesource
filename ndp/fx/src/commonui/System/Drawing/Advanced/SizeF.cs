//------------------------------------------------------------------------------
// <copyright file="SizeF.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {

    using System.Diagnostics;

    using System;
    using System.IO;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Diagnostics.CodeAnalysis;        
    using System.Globalization;

    /**
     * Represents a dimension in 2D coordinate space
     */
    /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the size of a rectangular region
    ///       with an ordered pair of width and height.
    ///    </para>
    /// </devdoc>
    [Serializable]
    [System.Runtime.InteropServices.ComVisible(true)]
    [TypeConverter(typeof(SizeFConverter))]
    [SuppressMessage("Microsoft.Usage", "CA2225:OperatorOverloadsHaveNamedAlternates")]
    public struct SizeF {

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.Empty"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.SizeF'/> class.
        /// </devdoc>
        public static readonly SizeF Empty = new SizeF();
        private float width;
        private float height;


        /**
         * Create a new SizeF object from another size object
         */
        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.SizeF"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.SizeF'/> class
        ///    from the specified existing <see cref='System.Drawing.SizeF'/>.
        /// </devdoc>
        public SizeF(SizeF size) {
            width = size.width;
            height = size.height;
        }

        /**
         * Create a new SizeF object from a point
         */
        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.SizeF1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.SizeF'/> class from
        ///       the specified <see cref='System.Drawing.PointF'/>.
        ///    </para>
        /// </devdoc>
        public SizeF(PointF pt) {
            width = pt.X;
            height = pt.Y;
        }

        /**
         * Create a new SizeF object of the specified dimension
         */
        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.SizeF2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.SizeF'/> class from
        ///       the specified dimensions.
        ///    </para>
        /// </devdoc>
        public SizeF(float width, float height) {
            this.width = width;
            this.height = height;
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.operator+"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs vector addition of two <see cref='System.Drawing.SizeF'/> objects.
        ///    </para>
        /// </devdoc>
        public static SizeF operator +(SizeF sz1, SizeF sz2) {
            return Add(sz1, sz2);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.operator-"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contracts a <see cref='System.Drawing.SizeF'/> by another <see cref='System.Drawing.SizeF'/>
        ///       .
        ///    </para>
        /// </devdoc>        
        public static SizeF operator -(SizeF sz1, SizeF sz2) {
            return Subtract(sz1, sz2);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.operator=="]/*' />
        /// <devdoc>
        ///    Tests whether two <see cref='System.Drawing.SizeF'/> objects
        ///    are identical.
        /// </devdoc>
        public static bool operator ==(SizeF sz1, SizeF sz2) {
            return sz1.Width == sz2.Width && sz1.Height == sz2.Height;
        }
        
        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two <see cref='System.Drawing.SizeF'/> objects are different.
        ///    </para>
        /// </devdoc>
        public static bool operator !=(SizeF sz1, SizeF sz2) {
            return !(sz1 == sz2);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.operatorPointF"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the specified <see cref='System.Drawing.SizeF'/> to a
        ///    <see cref='System.Drawing.PointF'/>.
        ///    </para>
        /// </devdoc>
        public static explicit operator PointF(SizeF size) {
            return new PointF(size.Width, size.Height);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether this <see cref='System.Drawing.SizeF'/> has zero
        ///       width and height.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public bool IsEmpty {
            get {
                return width == 0 && height == 0;
            }
        }

        /**
         * Horizontal dimension
         */
        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.Width"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the horizontal component of this
        ///    <see cref='System.Drawing.SizeF'/>.
        ///    </para>
        /// </devdoc>
        public float Width {
            get {
                return width;
            }
            set {
                width = value;
            }
        }

        /**
         * Vertical dimension
         */
        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.Height"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the vertical component of this
        ///    <see cref='System.Drawing.SizeF'/>.
        ///    </para>
        /// </devdoc>
        public float Height {
            get {
                return height;
            }
            set {
                height = value;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Performs vector addition of two <see cref='System.Drawing.SizeF'/> objects.
        ///    </para>
        /// </devdoc>
        public static SizeF Add(SizeF sz1, SizeF sz2) {
            return new SizeF(sz1.Width + sz2.Width, sz1.Height + sz2.Height);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.operator-"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contracts a <see cref='System.Drawing.SizeF'/> by another <see cref='System.Drawing.SizeF'/>
        ///       .
        ///    </para>
        /// </devdoc>        
        public static SizeF Subtract(SizeF sz1, SizeF sz2) {
            return new SizeF(sz1.Width - sz2.Width, sz1.Height - sz2.Height);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests to see whether the specified object is a
        ///    <see cref='System.Drawing.SizeF'/> 
        ///    with the same dimensions as this <see cref='System.Drawing.SizeF'/>.
        /// </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (!(obj is SizeF))
                return false;

            SizeF comp = (SizeF)obj;

            return(comp.Width == this.Width) && 
            (comp.Height == this.Height) &&
            (comp.GetType().Equals(GetType()));
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.ToPointF"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PointF ToPointF() {
            return (PointF) this;
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.ToSize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Size ToSize() {
            return Size.Truncate(this);
        }

        /// <include file='doc\SizeF.uex' path='docs/doc[@for="SizeF.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a human-readable string that represents this
        ///    <see cref='System.Drawing.SizeF'/>.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "{Width=" + width.ToString(CultureInfo.CurrentCulture) + ", Height=" + height.ToString(CultureInfo.CurrentCulture) + "}";
        }
    }

}

