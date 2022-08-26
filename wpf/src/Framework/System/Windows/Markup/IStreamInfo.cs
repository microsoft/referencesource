//---------------------------------------------------------------------------
//
// File: IStreamInfo.cs
//
// Description:
//   Provides an internal way to get the assembly from which the stream was created.
//   This is used by the various streams that the Navigation engine creates internally
//   and consumed by the parser\Baml Loader.
//
// Copyright (C) 2006 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Reflection;

namespace System.Windows.Markup
{
    internal interface IStreamInfo
    {
        Assembly Assembly { get; }
    }
}
