//------------------------------------------------------------------------------
// <copyright file="Size.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {
    using System.Runtime.Serialization.Formatters;

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
    /// <include file='doc\Size.uex' path='docs/doc[@for="Size"]/*' />
    /// <devdoc>
    ///    Represents the size of a rectangular region
    ///    with an ordered pair of width and height.
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(SizeConverter)),
    ]
    [Serializable]
    [System.Runtime.InteropServices.ComVisible(true)]
    [SuppressMessage("Microsoft.Usage", "CA2225:OperatorOverloadsHaveNamedAlternates")]
    public struct Size {

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Empty"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Size'/> class.
        /// </devdoc>
        public static readonly Size Empty = new Size();

        private int width;
        private int height;

        /**
         * Create a new Size object from a point
         */
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Size'/> class from
        ///       the specified <see cref='System.Drawing.Point'/>.
        ///    </para>
        /// </devdoc>
        public Size(Point pt) {
            width = pt.X;
            height = pt.Y;
        }

        /**
         * Create a new Size object of the specified dimension
         */
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Size1"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Size'/> class from
        ///    the specified dimensions.
        /// </devdoc>
        public Size(int width, int height) {
            this.width = width;
            this.height = height;
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.operatorSizeF"]/*' />
        /// <devdoc>
        ///    Converts the specified <see cref='System.Drawing.Size'/> to a
        /// <see cref='System.Drawing.SizeF'/>.
        /// </devdoc>
        public static implicit operator SizeF(Size p) {
            return new SizeF(p.Width, p.Height);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.operator+"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs vector addition of two <see cref='System.Drawing.Size'/> objects.
        ///    </para>
        /// </devdoc>
        public static Size operator +(Size sz1, Size sz2) {
            return Add(sz1, sz2);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.operator-"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contracts a <see cref='System.Drawing.Size'/> by another <see cref='System.Drawing.Size'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public static Size operator -(Size sz1, Size sz2) {
            return Subtract(sz1, sz2);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.operator=="]/*' />
        /// <devdoc>
        ///    Tests whether two <see cref='System.Drawing.Size'/> objects
        ///    are identical.
        /// </devdoc>
        public static bool operator ==(Size sz1, Size sz2) {
            return sz1.Width == sz2.Width && sz1.Height == sz2.Height;
        }
        
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two <see cref='System.Drawing.Size'/> objects are different.
        ///    </para>
        /// </devdoc>
        public static bool operator !=(Size sz1, Size sz2) {
            return !(sz1 == sz2);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.operatorPoint"]/*' />
        /// <devdoc>
        ///    Converts the specified <see cref='System.Drawing.Size'/> to a
        /// <see cref='System.Drawing.Point'/>.
        /// </devdoc>
        public static explicit operator Point(Size size) {
            return new Point(size.Width, size.Height);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.IsEmpty"]/*' />
        /// <devdoc>
        ///    Tests whether this <see cref='System.Drawing.Size'/> has zero
        ///    width and height.
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
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Width"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the horizontal component of this
        ///    <see cref='System.Drawing.Size'/>.
        ///    </para>
        /// </devdoc>
        public int Width {
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
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Height"]/*' />
        /// <devdoc>
        ///    Represents the vertical component of this
        /// <see cref='System.Drawing.Size'/>.
        /// </devdoc>
        public int Height {
            get {
                return height;
            }
            set {
                height = value;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Performs vector addition of two <see cref='System.Drawing.Size'/> objects.
        ///    </para>
        /// </devdoc>
        public static Size Add(Size sz1, Size sz2) {
            return new Size(sz1.Width + sz2.Width, sz1.Height + sz2.Height);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Ceiling"]/*' />
        /// <devdoc>
        ///   Converts a SizeF to a Size by performing a ceiling operation on
        ///   all the coordinates.
        /// </devdoc>
        public static Size Ceiling(SizeF value) {
            return new Size((int)Math.Ceiling(value.Width), (int)Math.Ceiling(value.Height));
        }

        /// <devdoc>
        ///    <para>
        ///       Contracts a <see cref='System.Drawing.Size'/> by another <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>
        public static Size Subtract(Size sz1, Size sz2) {
            return new Size(sz1.Width - sz2.Width, sz1.Height - sz2.Height);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Truncate"]/*' />
        /// <devdoc>
        ///   Converts a SizeF to a Size by performing a truncate operation on
        ///   all the coordinates.
        /// </devdoc>
        public static Size Truncate(SizeF value) {
            return new Size((int)value.Width, (int)value.Height);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Round"]/*' />
        /// <devdoc>
        ///   Converts a SizeF to a Size by performing a round operation on
        ///   all the coordinates.
        /// </devdoc>
        public static Size Round(SizeF value) {
            return new Size((int)Math.Round(value.Width), (int)Math.Round(value.Height));
        }
        
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests to see whether the specified object is a
        ///    <see cref='System.Drawing.Size'/> 
        ///    with the same dimensions as this <see cref='System.Drawing.Size'/>.
        /// </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (!(obj is Size)) 
                return false;
            
            Size comp = (Size)obj;
            // Note value types can't have derived classes, so we don't need to 
            // check the types of the objects here.  -- Microsoft, 2/21/2001
            return (comp.width == this.width) && 
                   (comp.height == this.height);
        }

        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a hash code.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return width ^ height;
        }
        
        /// <include file='doc\Size.uex' path='docs/doc[@for="Size.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a human-readable string that represents this
        ///    <see cref='System.Drawing.Size'/>.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "{Width=" + width.ToString(CultureInfo.CurrentCulture) + ", Height=" + height.ToString(CultureInfo.CurrentCulture) + "}";
        }
    }

}
