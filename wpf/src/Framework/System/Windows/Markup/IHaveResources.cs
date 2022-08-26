//---------------------------------------------------------------------------
//
// File: IHaveResources.cs
//
// Description:
//   Provides an internal way to get the baml reader from a markup extension.
//   This was created for internal use, by the resource reference code, as a
//   quick fix.  It should be replaced by a public API to do the resource
//   lookup in the parser stack.
//
// Copyright (C) 2006 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Reflection;
using System.Globalization;

namespace System.Windows.Markup
{
    internal interface IHaveResources
    {
        ResourceDictionary Resources { get; set; }
    }
}
