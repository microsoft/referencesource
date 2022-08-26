//---------------------------------------------------------------------------
//
// <copyright file="PathFigureCollection.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
//---------------------------------------------------------------------------

using MS.Internal;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Runtime.InteropServices;
using System.ComponentModel.Design.Serialization;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Composition;
using System.Windows.Markup;

namespace System.Windows.Media
{
    /// <summary>
    /// The class definition for PathFigureCollection
    /// </summary>
    public sealed partial class PathFigureCollection : Animatable, IList, IList<PathFigure>
    {
        /// <summary>
        /// Can serialze "this" to a string.  This returns true iff all of the PathFigures in the collection
        /// return true from their CanSerializeToString methods.
        /// </summary>
        internal bool CanSerializeToString()
        {
            bool canSerializeToString = true;

            for (int i=0; (i<_collection.Count) && canSerializeToString; i++)
            {
                canSerializeToString &= _collection[i].CanSerializeToString();
            }

            return canSerializeToString;
        }
    }
}
