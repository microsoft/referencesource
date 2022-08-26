//---------------------------------------------------------------------------
//
// <copyright file="AmbientLight.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//
// Description: 3D ambient light implementation.
//
//              See spec at http://avalon/medialayer/Specifications/Avalon3D%20API%20Spec.mht
//
// History:
//  06/18/2003 : t-gregr - Created
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Media;
using System.Windows.Media.Composition;
using MS.Internal;
using System.ComponentModel.Design.Serialization;
using System.Windows.Markup;

namespace System.Windows.Media.Media3D
{
    /// <summary>
    ///     Ambient lights light objects uniformly, regardless of their shape.
    /// </summary>
    public sealed partial class AmbientLight : Light
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        /// <summary>
        ///     Constructor that creates a new white ambient light.
        /// </summary>
        public AmbientLight() 
            : this( Colors.White )
        {
        }

        /// <summary>
        ///     Constructor that creates a new ambient light.
        /// </summary>
        /// <param name="ambientColor">Ambient color of the new light.</param>
        public AmbientLight(Color ambientColor)
        {
            Color = ambientColor;
        }

        #endregion Constructors

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

