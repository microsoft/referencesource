//------------------------------------------------------------------------------
// <copyright file="Margins.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Serialization;

    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Globalization;

    /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the margins of a printed page.
    ///    </para>
    /// </devdoc>
    [
        Serializable,
        TypeConverterAttribute(typeof(MarginsConverter))
    ]
    public class Margins : ICloneable {
        private int left;
        private int right;
        private int top;
        private int bottom;

        [OptionalField]
        private double doubleLeft;
        [OptionalField]
        private double doubleRight;
        [OptionalField]
        private double doubleTop;
        [OptionalField]
        private double doubleBottom;

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Margins"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of a the <see cref='System.Drawing.Printing.Margins'/> class with one-inch margins.
        ///    </para>
        /// </devdoc>
        public Margins() : this(100, 100, 100, 100) {
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Margins1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of a the <see cref='System.Drawing.Printing.Margins'/> class with the specified left, right, top, and bottom
        ///       margins.
        ///    </para>
        /// </devdoc>
        public Margins(int left, int right, int top, int bottom) {
            CheckMargin(left, "left");
            CheckMargin(right, "right");
            CheckMargin(top, "top");
            CheckMargin(bottom, "bottom");

            this.left = left;
            this.right = right;
            this.top = top;
            this.bottom = bottom;

            this.doubleLeft = (double)left;
            this.doubleRight = (double)right;
            this.doubleTop = (double)top;
            this.doubleBottom = (double)bottom;
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Left"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the left margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Left {
            get { return left;}
            set { 
                CheckMargin(value, "Left");
                left = value;
                this.doubleLeft = (double)value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Right"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the right margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Right {
            get { return right;}
            set { 
                CheckMargin(value, "Right");
                right = value;
                this.doubleRight = (double)value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Top"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the top margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Top {
            get { return top;}
            set { 
                CheckMargin(value, "Top");
                top = value;
                this.doubleTop = (double)value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Bottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the bottom margin, in hundredths of an inch.
        ///    </para>
        /// </devdoc>
        public int Bottom {
            get { return bottom;}
            set { 
                CheckMargin(value, "Bottom");
                bottom = value;
                this.doubleBottom = (double)value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.DoubleLeft"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the left margin with double value, in hundredths of an inch.
        ///       When use the setter, the ranger of setting double value should between
        ///       0 to Int.MaxValue;
        ///    </para>
        /// </devdoc>
        internal double DoubleLeft {
            get { return doubleLeft; }
            set { 
                this.Left = (int)Math.Round(value);
                doubleLeft = value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.DoubleRight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the right margin with double value, in hundredths of an inch.
        ///       When use the setter, the ranger of setting double value should between
        ///       0 to Int.MaxValue;
        ///    </para>
        /// </devdoc>
        internal double DoubleRight {
            get { return doubleRight; }
            set { 
                this.Right = (int)Math.Round(value);
                doubleRight = value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.DoubleTop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the top margin with double value, in hundredths of an inch.
        ///       When use the setter, the ranger of setting double value should between
        ///       0 to Int.MaxValue;
        ///    </para>
        /// </devdoc>
        internal double DoubleTop {
            get { return doubleTop; }
            set { 
                this.Top = (int)Math.Round(value);
                doubleTop = value;
            }
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.DoubleBottom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the bottom margin with double value, in hundredths of an inch.
        ///       When use the setter, the ranger of setting double value should between
        ///       0 to Int.MaxValue;
        ///    </para>
        /// </devdoc>
        internal double DoubleBottom {
            get { return doubleBottom; }
            set { 
                this.Bottom = (int)Math.Round(value);
                doubleBottom = value;
            }
        }

        [OnDeserialized()]
        private void OnDeserializedMethod(StreamingContext context) {
            if (doubleLeft == 0 && left != 0) {
                doubleLeft = (double)left;
            }

            if (doubleRight == 0 && right != 0) {
                doubleRight = (double)right;
            }

            if (doubleTop == 0 && top != 0) {
                doubleTop = (double)top;
            }

            if (doubleBottom == 0 && bottom != 0) {
                doubleBottom = (double)bottom;
            }
        }

        private void CheckMargin(int margin, string name) {
            if (margin < 0)
                throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, name, margin, "0"));
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Clone"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a duplicate of this object, member by member.
        ///    </para>
        /// </devdoc>
        public object Clone() {
            return MemberwiseClone();
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares this <see cref='System.Drawing.Printing.Margins'/> to a specified <see cref='System.Drawing.Printing.Margins'/> to see whether they
        ///       are equal.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            Margins margins = obj as Margins;
            if (margins == this) return true;
            if (margins == null) return false;

            return margins.Left == this.Left
            && margins.Right == this.Right
            && margins.Top == this.Top
            && margins.Bottom == this.Bottom;
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Calculates and retrieves a hash code based on the left, right, top, and bottom
        ///       margins.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            // return HashCodes.Combine(left, right, top, bottom);
            uint left = (uint) this.Left;
            uint right = (uint) this.Right;
            uint top = (uint) this.Top;
            uint bottom = (uint) this.Bottom;

            uint result =  left ^
                           ((right << 13) | (right >> 19)) ^
                           ((top << 26) | (top >>  6)) ^
                           ((bottom <<  7) | (bottom >> 25));

            return unchecked((int) result);
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.operator=="]/*' />
        /// <devdoc>
        ///    Tests whether two <see cref='System.Drawing.Printing.Margins'/> objects
        ///    are identical.
        /// </devdoc>
        public static bool operator ==(Margins m1, Margins m2) {
            if (object.ReferenceEquals(m1, null) != object.ReferenceEquals(m2, null)) {
                return false;
            }
            if (!object.ReferenceEquals(m1, null)) {
                return m1.Left == m2.Left && m1.Top == m2.Top && m1.Right == m2.Right && m1.Bottom == m2.Bottom; 
            }
            return true;
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether two <see cref='System.Drawing.Printing.Margins'/> objects are different.
        ///    </para>
        /// </devdoc>
        public static bool operator !=(Margins m1, Margins m2) {
            return !(m1 == m2);
        }

        /// <include file='doc\Margins.uex' path='docs/doc[@for="Margins.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information for the Margins in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "[Margins"
            + " Left=" + Left.ToString(CultureInfo.InvariantCulture)
            + " Right=" + Right.ToString(CultureInfo.InvariantCulture)
            + " Top=" + Top.ToString(CultureInfo.InvariantCulture)
            + " Bottom=" + Bottom.ToString(CultureInfo.InvariantCulture)
            + "]";
        }
    }
}
