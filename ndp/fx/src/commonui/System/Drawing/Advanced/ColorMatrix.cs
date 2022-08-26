//------------------------------------------------------------------------------
// <copyright file="ColorMatrix.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;
    using System.Runtime.InteropServices;

    // 





    [StructLayout(LayoutKind.Sequential)]
    public sealed class ColorMatrix {
        float matrix00;
        float matrix01;
        float matrix02;
        float matrix03;
        float matrix04;
        float matrix10;
        float matrix11;
        float matrix12;
        float matrix13;
        float matrix14;
        float matrix20;
        float matrix21;
        float matrix22;
        float matrix23;
        float matrix24;
        float matrix30;
        float matrix31;
        float matrix32;
        float matrix33;
        float matrix34;
        float matrix40;
        float matrix41;
        float matrix42;
        float matrix43;
        float matrix44;

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.ColorMatrix"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Imaging.ColorMatrix'/> class.
        ///    </para>
        /// </devdoc>
        public ColorMatrix() { 
            /*
             * Setup identity matrix by default
             */

            matrix00 = 1.0f;
            //matrix01 = 0.0f;
            //matrix02 = 0.0f;
            //matrix03 = 0.0f;
            //matrix04 = 0.0f;
            //matrix10 = 0.0f;
            matrix11 = 1.0f;
            //matrix12 = 0.0f;
            //matrix13 = 0.0f;
            //matrix14 = 0.0f;
            //matrix20 = 0.0f;
            //matrix21 = 0.0f;
            matrix22 = 1.0f;
           // matrix23 = 0.0f;
           // matrix24 = 0.0f;
           // matrix30 = 0.0f;
            //matrix31 = 0.0f;
           // matrix32 = 0.0f;
            matrix33 = 1.0f;
           // matrix34 = 0.0f;
           // matrix40 = 0.0f;
           // matrix41 = 0.0f;
           // matrix42 = 0.0f;
           // matrix43 = 0.0f;
            matrix44 = 1.0f;
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix00"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the
        ///       0th row and 0th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix00 {
            get { return matrix00; }
            set { matrix00 = value; }
        }
        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix01"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 0th row and 1st column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix01 {
            get { return matrix01; }
            set { matrix01 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix02"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 0th row and 2nd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix02 {
            get { return matrix02; }
            set { matrix02 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix03"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 0th row and 3rd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix03 {
            get { return matrix03; }
            set { matrix03 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix04"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 0th row and 4th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix04 {
            get { return matrix04; }
            set { matrix04 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix10"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 1st row and 0th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix10 {
            get { return matrix10; }
            set { matrix10 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix11"]/*' />
        /// <devdoc>
        ///    Represents the element at the 1st row and
        ///    1st column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        /// </devdoc>
        public float Matrix11 {
            get { return matrix11; }
            set { matrix11 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix12"]/*' />
        /// <devdoc>
        ///    Represents the element at the 1st row
        ///    and 2nd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        /// </devdoc>
        public float Matrix12 {
            get { return matrix12; }
            set { matrix12 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix13"]/*' />
        /// <devdoc>
        ///    Represents the element at the 1st row
        ///    and 3rd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        /// </devdoc>
        public float Matrix13 {
            get { return matrix13; }
            set { matrix13 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix14"]/*' />
        /// <devdoc>
        ///    Represents the element at the 1st row
        ///    and 4th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        /// </devdoc>
        public float Matrix14 {
            get { return matrix14; }
            set { matrix14 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix20"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 2nd row and
        ///       0th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix20 {
            get { return matrix20; }
            set { matrix20 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix21"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 2nd row and 1st column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix21 {
            get { return matrix21; }
            set { matrix21 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix22"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 2nd row and 2nd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix22 {
            get { return matrix22; }
            set { matrix22 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix23"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 2nd row and 3rd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix23 {
            get { return matrix23; }
            set { matrix23 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix24"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 2nd row and 4th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix24 {
            get { return matrix24; }
            set { matrix24 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix30"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 3rd row and 0th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix30 {
            get { return matrix30; }
            set { matrix30 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix31"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 3rd row and 1st column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix31 {
            get { return matrix31; }
            set { matrix31 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix32"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 3rd row and 2nd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix32 {
            get { return matrix32; }
            set { matrix32 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix33"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 3rd row and 3rd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix33 {
            get { return matrix33; }
            set { matrix33 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix34"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 3rd row and 4th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix34 {
            get { return matrix34; }
            set { matrix34 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix40"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 4th row and 0th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix40 {
            get { return matrix40; }
            set { matrix40 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix41"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 4th row and 1st column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix41 {
            get { return matrix41; }
            set { matrix41 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix42"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 4th row and 2nd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix42 {
            get { return matrix42; }
            set { matrix42 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix43"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 4th row and 3rd column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix43 {
            get { return matrix43; }
            set { matrix43 = value; }
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.Matrix44"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the element at the 4th row and 4th column of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float Matrix44 {
            get { return matrix44; }
            set { matrix44 = value; }
        }


        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.ColorMatrix1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Drawing.Imaging.ColorMatrix'/> class with the
        ///       elements in the specified matrix.
        ///    </para>
        /// </devdoc>
        [CLSCompliant(false)]
        public ColorMatrix(float[][] newColorMatrix) {
            SetMatrix(newColorMatrix);
        }

        internal void SetMatrix(float[][] newColorMatrix) {
            matrix00 = newColorMatrix[0][0];
            matrix01 = newColorMatrix[0][1];
            matrix02 = newColorMatrix[0][2];
            matrix03 = newColorMatrix[0][3];
            matrix04 = newColorMatrix[0][4];
            matrix10 = newColorMatrix[1][0];
            matrix11 = newColorMatrix[1][1];
            matrix12 = newColorMatrix[1][2];
            matrix13 = newColorMatrix[1][3];
            matrix14 = newColorMatrix[1][4];
            matrix20 = newColorMatrix[2][0];
            matrix21 = newColorMatrix[2][1];
            matrix22 = newColorMatrix[2][2];
            matrix23 = newColorMatrix[2][3];
            matrix24 = newColorMatrix[2][4];
            matrix30 = newColorMatrix[3][0];
            matrix31 = newColorMatrix[3][1];
            matrix32 = newColorMatrix[3][2];
            matrix33 = newColorMatrix[3][3];
            matrix34 = newColorMatrix[3][4];
            matrix40 = newColorMatrix[4][0];
            matrix41 = newColorMatrix[4][1];
            matrix42 = newColorMatrix[4][2];
            matrix43 = newColorMatrix[4][3];
            matrix44 = newColorMatrix[4][4];
        }

        internal float[][] GetMatrix() {
            float[][] returnMatrix = new float[5][];

            for (int i = 0; i < 5; i++)
                returnMatrix[i] = new float[5];

            returnMatrix[0][0] = matrix00;
            returnMatrix[0][1] = matrix01;
            returnMatrix[0][2] = matrix02;
            returnMatrix[0][3] = matrix03;
            returnMatrix[0][4] = matrix04;
            returnMatrix[1][0] = matrix10;
            returnMatrix[1][1] = matrix11;
            returnMatrix[1][2] = matrix12;
            returnMatrix[1][3] = matrix13;
            returnMatrix[1][4] = matrix14;
            returnMatrix[2][0] = matrix20;
            returnMatrix[2][1] = matrix21;
            returnMatrix[2][2] = matrix22;
            returnMatrix[2][3] = matrix23;
            returnMatrix[2][4] = matrix24;
            returnMatrix[3][0] = matrix30;
            returnMatrix[3][1] = matrix31;
            returnMatrix[3][2] = matrix32;
            returnMatrix[3][3] = matrix33;
            returnMatrix[3][4] = matrix34;
            returnMatrix[4][0] = matrix40;
            returnMatrix[4][1] = matrix41;
            returnMatrix[4][2] = matrix42;
            returnMatrix[4][3] = matrix43;
            returnMatrix[4][4] = matrix44;

            return returnMatrix;
        }

        /// <include file='doc\ColorMatrix.uex' path='docs/doc[@for="ColorMatrix.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the specified element of this <see cref='System.Drawing.Imaging.ColorMatrix'/>.
        ///    </para>
        /// </devdoc>
        public float this[int row, int column] {
            get {
                return GetMatrix()[row][column];
            }

            set {
                float[][] tempMatrix = GetMatrix();

                tempMatrix[row][column] = value;

                SetMatrix(tempMatrix);
            }
        }
    }
}
