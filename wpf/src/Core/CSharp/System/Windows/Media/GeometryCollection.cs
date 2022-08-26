//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Description: Implementation of GeometryCollection
//
// History:
//      2004/11/11-Michka
//          Changed GeometryCollection from the result of a Boolean operation
//          to a simple collection class.
//
//---------------------------------------------------------------------------

using System;
using MS.Internal;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Diagnostics;
using System.Reflection;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Globalization;
using System.Windows.Media;
using System.Windows;
using System.Windows.Media.Composition;
using System.Text.RegularExpressions;
using System.Windows.Media.Animation;
using System.Windows.Markup;
using System.Runtime.InteropServices;

namespace System.Windows.Media
{
    /// <summary>
    /// The class definition for GeometryCollection
    /// </summary>    
    public sealed partial class GeometryCollection : Animatable, IList, IList<Geometry>
    {
    }
}
