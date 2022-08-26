//---------------------------------------------------------------------------
//
// <copyright file="Affine3DTransform.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Affine 3D transformations. 
//
//              See spec at http://avalon/medialayer/Specifications/Avalon3D%20API%20Spec.mht 
//
// History:  
//  06/04/2003 : t-gregr - Created
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Media.Media3D
{
    /// <summary>
    ///     Base class that all concrete affine 3D transforms derive from
    ///     (translate, rotate, scale, etc.)
    /// </summary>
    public abstract partial class AffineTransform3D : Transform3D
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        // Prevent 3rd parties from extending this abstract base class.
        internal AffineTransform3D() {}

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------
        
        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        /// <summary>
        ///     Determines if this is an affine transformation.
        /// </summary>
        public override bool IsAffine 
        {
            get
            {
                ReadPreamble();

                // All subclasses should be affine by definition.
                return true;
            }
        }

        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
    }
}

