//------------------------------------------------------------------------------
// <copyright file="RectangleF.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {

    using System.Diagnostics;
    using System.Diagnostics.Contracts;
    using System;
    using System.IO;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Drawing.Internal;
    using System.Globalization;
    
    
    /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Stores the location and size of a rectangular region. For
    ///       more advanced region functions use a <see cref='System.Drawing.Region'/>
    ///       object.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public struct RectangleF {

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Empty"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Drawing.RectangleF'/>
        ///    class.
        /// </devdoc>
        public static readonly RectangleF Empty = new RectangleF();

        private float x;
        private float y;
        private float width;
        private float height;

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.RectangleF"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.RectangleF'/>
        ///       class with the specified location and size.
        ///    </para>
        /// </devdoc>
        public RectangleF(float x, float y, float width, float height) {
            this.x = x;
            this.y = y;
            this.width = width;
            this.height = height;
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.RectangleF1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.RectangleF'/>
        ///       class with the specified location
        ///       and size.
        ///    </para>
        /// </devdoc>
        public RectangleF(PointF location, SizeF size) {
            this.x = location.X;
            this.y = location.Y;
            this.width = size.Width;
            this.height = size.Height;
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.FromLTRB"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='System.Drawing.RectangleF'/> with
        ///       the specified location and size.
        ///    </para>
        /// </devdoc>
        // !! Not in C++ version
        public static RectangleF FromLTRB(float left, float top, float right, float bottom) {
            return new RectangleF(left,
                                 top,
                                 right - left,
                                 bottom - top);
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Location"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the coordinates of the upper-left corner of
        ///       the rectangular region represented by this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public PointF Location {
            get {
                return new PointF(X, Y);
            }
            set {
                X = value.X;
                Y = value.Y;
            }
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the size of this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public SizeF Size {
            get {
                return new SizeF(Width, Height);
            }
            set {
                this.Width = value.Width;
                this.Height = value.Height;
            }
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.X"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the x-coordinate of the
        ///       upper-left corner of the rectangular region defined by this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        public float X {
            get {
                return x;
            }
            set {
                x = value;
            }
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Y"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the y-coordinate of the
        ///       upper-left corner of the rectangular region defined by this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        public float Y {
            get {
                return y;
            }
            set {
                y = value;
            }
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Width"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the width of the rectangular
        ///       region defined by this <see cref='System.Drawing.RectangleF'/>.
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
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Height"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the height of the
        ///       rectangular region defined by this <see cref='System.Drawing.RectangleF'/>.
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
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Left"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the x-coordinate of the upper-left corner of the
        ///       rectangular region defined by this <see cref='System.Drawing.RectangleF'/> .
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public float Left {
            get {
                return X;
            }
        }
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Top"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the y-coordinate of the upper-left corner of the
        ///       rectangular region defined by this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public float Top {
            get {       
                return Y;
            }
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the x-coordinate of the lower-right corner of the
        ///       rectangular region defined by this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public float Right {
            get {
                return X + Width;
            }
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Bottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the y-coordinate of the lower-right corner of the
        ///       rectangular region defined by this <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public float Bottom {
            get {
                return Y + Height;
            }
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether this <see cref='System.Drawing.RectangleF'/> has a <see cref='System.Drawing.RectangleF.Width'/> or a <see cref='System.Drawing.RectangleF.Height'/> of 0.
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public bool IsEmpty {
            get {
                return (Width <= 0 )|| (Height <= 0);
            }
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether <paramref name="obj"/> is a <see cref='System.Drawing.RectangleF'/> with the same location and size of this
        ///    <see cref='System.Drawing.RectangleF'/>.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (!(obj is RectangleF)) 
                return false;
            
            RectangleF comp = (RectangleF)obj;
            
            return (comp.X == this.X) &&
                   (comp.Y == this.Y) &&
                   (comp.Width == this.Width) &&
                   (comp.Height == this.Height);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.operator=="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two <see cref='System.Drawing.RectangleF'/>
        ///       objects have equal location and size.
        ///    </para>
        /// </devdoc>
        public static bool operator ==(RectangleF left, RectangleF right) {
            return (left.X == right.X
                     && left.Y == right.Y
                     && left.Width == right.Width
                     && left.Height == right.Height);
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two <see cref='System.Drawing.RectangleF'/>
        ///       objects differ in location or size.
        ///    </para>
        /// </devdoc>
        public static bool operator !=(RectangleF left, RectangleF right) {
            return !(left == right);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the specfied point is contained within the
        ///       rectangular region defined by this <see cref='System.Drawing.Rectangle'/> .
        ///    </para>
        /// </devdoc>
        [Pure]
        public bool Contains(float x, float y) {
            return this.X <= x &&
            x < this.X + this.Width &&
            this.Y <= y &&
            y < this.Y + this.Height;
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Contains1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the specfied point is contained within the
        ///       rectangular region defined by this <see cref='System.Drawing.Rectangle'/> .
        ///    </para>
        /// </devdoc>
        [Pure]
        public bool Contains(PointF pt) {
            return Contains(pt.X, pt.Y);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Contains2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if the rectangular region represented by
        ///    <paramref name="rect"/> is entirely contained within the rectangular region represented by 
        ///       this <see cref='System.Drawing.Rectangle'/> .
        ///    </para>
        /// </devdoc>
        [Pure]
        public bool Contains(RectangleF rect) {
            return (this.X <= rect.X) &&
                   ((rect.X + rect.Width) <= (this.X + this.Width)) &&
                   (this.Y <= rect.Y) &&
                   ((rect.Y + rect.Height) <= (this.Y + this.Height));
        }

        // !! Not in C++ version
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.GetHashCode"]/*' />
        /// <devdoc>
        ///    Gets the hash code for this <see cref='System.Drawing.RectangleF'/>.
        /// </devdoc>
        public override int GetHashCode() {
            return unchecked((int)((UInt32)X ^
            (((UInt32)Y << 13) | ((UInt32)Y >> 19)) ^
            (((UInt32)Width << 26) | ((UInt32)Width >>  6)) ^
            (((UInt32)Height <<  7) | ((UInt32)Height >> 25))));
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Inflate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Inflates this <see cref='System.Drawing.Rectangle'/>
        ///       by the specified amount.
        ///    </para>
        /// </devdoc>
        public void Inflate(float x, float y) {
            this.X -= x;
            this.Y -= y;
            this.Width += 2*x;
            this.Height += 2*y;
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Inflate1"]/*' />
        /// <devdoc>
        ///    Inflates this <see cref='System.Drawing.Rectangle'/> by the specified amount.
        /// </devdoc>
        public void Inflate(SizeF size) { 
            Inflate(size.Width, size.Height);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Inflate2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a <see cref='System.Drawing.Rectangle'/>
        ///       that is inflated by the specified amount.
        ///    </para>
        /// </devdoc>
        // !! Not in C++
        public static RectangleF Inflate(RectangleF rect, float x, float y) {
            RectangleF r = rect;
            r.Inflate(x, y);
            return r;
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Intersect"]/*' />
        /// <devdoc> Creates a Rectangle that represents the intersection between this Rectangle and rect.
        /// </devdoc>
        public void Intersect(RectangleF rect)
        {
            RectangleF result = RectangleF.Intersect(rect, this);
            
            this.X = result.X;
            this.Y = result.Y;
            this.Width = result.Width;
            this.Height = result.Height;
        }
        
        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Intersect1"]/*' />
        /// <devdoc>
        ///    Creates a rectangle that represents the intersetion between a and
        ///    b. If there is no intersection, null is returned.
        /// </devdoc>
        [Pure]
        public static RectangleF Intersect(RectangleF a, RectangleF b) {
            float x1 = Math.Max(a.X, b.X);
            float x2 = Math.Min(a.X + a.Width, b.X + b.Width);
            float y1 = Math.Max(a.Y, b.Y);
            float y2 = Math.Min(a.Y + a.Height, b.Y + b.Height);

            if (x2 >= x1
                && y2 >= y1) {

                return new RectangleF(x1, y1, x2 - x1, y2 - y1);
            }
            return RectangleF.Empty;
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.IntersectsWith"]/*' />
        /// <devdoc>
        ///    Determines if this rectangle intersets with rect.
        /// </devdoc>
        [Pure]
        public bool IntersectsWith(RectangleF rect) {
            return (rect.X < this.X + this.Width) && 
                   (this.X < (rect.X + rect.Width)) &&
                   (rect.Y < this.Y + this.Height) &&
                   (this.Y < rect.Y + rect.Height);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Union"]/*' />
        /// <devdoc>
        ///    Creates a rectangle that represents the union between a and
        ///    b.
        /// </devdoc>
        [Pure]
        public static RectangleF Union(RectangleF a, RectangleF b) {
            float x1 = Math.Min(a.X, b.X);
            float x2 = Math.Max(a.X + a.Width, b.X + b.Width);
            float y1 = Math.Min(a.Y, b.Y);
            float y2 = Math.Max(a.Y + a.Height, b.Y + b.Height);

            return new RectangleF(x1, y1, x2 - x1, y2 - y1);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Offset"]/*' />
        /// <devdoc>
        ///    Adjusts the location of this rectangle by the specified amount.
        /// </devdoc>
        public void Offset(PointF pos) {
            Offset(pos.X, pos.Y);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.Offset1"]/*' />
        /// <devdoc>
        ///    Adjusts the location of this rectangle by the specified amount.
        /// </devdoc>
        public void Offset(float x, float y) {
            this.X += x;
            this.Y += y;
        }

        /**
         * Convert the current rectangle object into
         * a GDI+ GPRECTF structure.
         */
        internal GPRECTF ToGPRECTF() {
            return new GPRECTF(X, Y, Width, Height);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.operatorRectangleF"]/*' />
        /// <devdoc>
        ///    Converts the specified <see cref='System.Drawing.Rectangle'/> to a
        /// <see cref='System.Drawing.RectangleF'/>.
        /// </devdoc>
        public static implicit operator RectangleF(Rectangle r) {
            return new RectangleF(r.X, r.Y, r.Width, r.Height);
        }

        /// <include file='doc\RectangleF.uex' path='docs/doc[@for="RectangleF.ToString"]/*' />
        /// <devdoc>
        ///    Converts the <see cref='System.Drawing.RectangleF.Location'/> and <see cref='System.Drawing.RectangleF.Size'/> of this <see cref='System.Drawing.RectangleF'/> to a
        ///    human-readable string.
        /// </devdoc>
        public override string ToString() {
            return "{X=" + X.ToString(CultureInfo.CurrentCulture) + ",Y=" + Y.ToString(CultureInfo.CurrentCulture) +
            ",Width=" + Width.ToString(CultureInfo.CurrentCulture) +
            ",Height=" + Height.ToString(CultureInfo.CurrentCulture) + "}";
        }
   }
}
