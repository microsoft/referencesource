//---------------------------------------------------------------------------
//
// <copyright file="RowVisual.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Row visual is used to group cell visuals of cells 
//              belonging to the row. 
//              The only reason for RowVisual existence is keeping 
//              a reference to the associated row object. 
//
//
// History:  
//  10/20/2003 : olego - Created
//
//---------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows.Threading;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;

namespace MS.Internal.PtsHost
{
    /// <summary>
    /// RowVisual.
    /// </summary>
    internal sealed class RowVisual : ContainerVisual
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        /// <summary>
        /// Default constructor
        /// </summary>
        /// <param name="row">Row associated with this visual.</param>
        internal RowVisual(TableRow row)
        {
            _row = row;
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------

        #region Internal Properties

        /// <summary>
        /// Row.
        /// </summary>
        internal TableRow Row
        {
            get { return (_row); }
        }

        #endregion Internal Properties

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields 
        private readonly TableRow _row;
        #endregion Private Fields 
    }
}
