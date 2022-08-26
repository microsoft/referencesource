//------------------------------------------------------------------------------
// <copyright file="Point.cs" company="Microsoft">
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
    using System.Drawing.Internal;
    using System.Diagnostics.CodeAnalysis;    
    using System.Globalization;

    /// <include file='doc\Point.uex' path='docs/doc[@for="Point"]/*' />
    /// <devdoc>
    ///    Represents an ordered pair of x and y coordinates that
    ///    define a point in a two-dimensional plane.
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(PointConverter)),
    ]
    [Serializable]
    [System.Runtime.InteropServices.ComVisible(true)]
    [SuppressMessage("Microsoft.Usage", "CA2225:OperatorOverloadsHaveNamedAlternates")]
    public struct Point {

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Empty"]/*' />
        /// <devdoc>
        ///    Creates a new instance of the <see cref='System.Drawing.Point'/> class
        ///    with member data left uninitialized.
        /// </devdoc>
        public static readonly Point Empty = new Point();

        private int x;
        private int y;

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Point"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.Point'/> class
        ///    with the specified coordinates.
        /// </devdoc>
        public Point(int x, int y) {
            this.x = x;
            this.y = y;
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Point1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Point'/> class
        ///       from a <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>
        public Point(Size sz) {
            this.x = sz.Width;
            this.y = sz.Height;
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Point2"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the Point class using
        ///    coordinates specified by an integer value.
        /// </devdoc>
        public Point(int dw) {
            unchecked {
                this.x = (short)LOWORD(dw);
                this.y = (short)HIWORD(dw);
            }
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether this <see cref='System.Drawing.Point'/> is empty.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public bool IsEmpty {
            get {
                return x == 0 && y == 0;
            }
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.X"]/*' />
        /// <devdoc>
        ///    Gets the x-coordinate of this <see cref='System.Drawing.Point'/>.
        /// </devdoc>
        public int X {
            get {
                return x;
            }
            set {
                x = value;
            }
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Y"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the y-coordinate of this <see cref='System.Drawing.Point'/>.
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

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.operatorPointF"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.PointF'/> with the coordinates of the specified
        ///    <see cref='System.Drawing.Point'/> 
        ///    .
        /// </para>
        /// </devdoc>
        public static implicit operator PointF(Point p) {
            return new PointF(p.X, p.Y);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.operatorSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.Size'/> with the coordinates of the specified <see cref='System.Drawing.Point'/> .
        ///    </para>
        /// </devdoc>
        public static explicit operator Size(Point p) {
            return new Size(p.X, p.Y);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.operator+"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates a <see cref='System.Drawing.Point'/> by a given <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>        
        public static Point operator +(Point pt, Size sz) {
            return Add(pt, sz);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.operator-"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Translates a <see cref='System.Drawing.Point'/> by the negative of a given <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>        
        public static Point operator -(Point pt, Size sz) {
            return Subtract(pt, sz);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.operator=="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two <see cref='System.Drawing.Point'/> objects. The result specifies
        ///       whether the values of the <see cref='System.Drawing.Point.X'/> and <see cref='System.Drawing.Point.Y'/> properties of the two <see cref='System.Drawing.Point'/>
        ///       objects are equal.
        ///    </para>
        /// </devdoc>
        public static bool operator ==(Point left, Point right) {
            return left.X == right.X && left.Y == right.Y;
        }
        
        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two <see cref='System.Drawing.Point'/> objects. The result specifies whether the values
        ///       of the <see cref='System.Drawing.Point.X'/> or <see cref='System.Drawing.Point.Y'/> properties of the two
        ///    <see cref='System.Drawing.Point'/> 
        ///    objects are unequal.
        /// </para>
        /// </devdoc>
        public static bool operator !=(Point left, Point right) {
            return !(left == right);
        }

        /// <devdoc>
        ///    <para>
        ///       Translates a <see cref='System.Drawing.Point'/> by a given <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>        
        public static Point Add(Point pt, Size sz) {
            return new Point(pt.X + sz.Width, pt.Y + sz.Height);
        }

        /// <devdoc>
        ///    <para>
        ///       Translates a <see cref='System.Drawing.Point'/> by the negative of a given <see cref='System.Drawing.Size'/> .
        ///    </para>
        /// </devdoc>        
        public static Point Subtract(Point pt, Size sz) {
            return new Point(pt.X - sz.Width, pt.Y - sz.Height);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Ceiling"]/*' />
        /// <devdoc>
        ///   Converts a PointF to a Point by performing a ceiling operation on
        ///   all the coordinates.
        /// </devdoc>
        public static Point Ceiling(PointF value) {
            return new Point((int)Math.Ceiling(value.X), (int)Math.Ceiling(value.Y));
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Truncate"]/*' />
        /// <devdoc>
        ///   Converts a PointF to a Point by performing a truncate operation on
        ///   all the coordinates.
        /// </devdoc>
        public static Point Truncate(PointF value) {
            return new Point((int)value.X, (int)value.Y);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Round"]/*' />
        /// <devdoc>
        ///   Converts a PointF to a Point by performing a round operation on
        ///   all the coordinates.
        /// </devdoc>
        public static Point Round(PointF value) {
            return new Point((int)Math.Round(value.X), (int)Math.Round(value.Y));
        }


        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies whether this <see cref='System.Drawing.Point'/> contains
        ///       the same coordinates as the specified <see cref='System.Object'/>.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (!(obj is Point)) return false;
            Point comp = (Point)obj;
            // Note value types can't have derived classes, so we don't need 
            // to check the types of the objects here.  -- Microsoft, 2/21/2001
            return comp.X == this.X && comp.Y == this.Y;
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a hash code.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return unchecked(x ^ y);
        }
        
        /**
         * Offset the current Point object by the given amount
         */
        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Offset"]/*' />
        /// <devdoc>
        ///    Translates this <see cref='System.Drawing.Point'/> by the specified amount.
        /// </devdoc>
        public void Offset(int dx, int dy) {
            X += dx;
            Y += dy;
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.Offset2"]/*' />
        /// <devdoc>
        ///    Translates this <see cref='System.Drawing.Point'/> by the specified amount.
        /// </devdoc>
        public void Offset(Point p) {
            Offset(p.X, p.Y);
        }

        /// <include file='doc\Point.uex' path='docs/doc[@for="Point.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts this <see cref='System.Drawing.Point'/>
        ///       to a human readable
        ///       string.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "{X=" + X.ToString(CultureInfo.CurrentCulture) + ",Y=" + Y.ToString(CultureInfo.CurrentCulture) + "}";
        }

        private static int HIWORD(int n) {
            return(n >> 16) & 0xffff;
        }

        private static int LOWORD(int n) {
            return n & 0xffff;
        }
    }
}
