//-----------------------------------------------------------------------------
//-------------   *** WARNING ***
//-------------    This file is part of a legally monitored development project.  
//-------------    Do not check in changes to this project.  Do not raid bugs on this
//-------------    code in the main PS database.  Do not contact the owner of this
//-------------    code directly.  Contact the legal team at ‘ZSLegal’ for assistance.
//-------------   *** WARNING ***
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// <copyright file="CompressionMethodEnum.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  This is an internal class that enables interactions with Zip archives
//  for OPC scenarios 
//
// History:
//  11/19/2004: IgorBel: Initial creation.
//
//-----------------------------------------------------------------------------

using System;

namespace MS.Internal.IO.Zip
{
    /// <summary>
    /// CompressionMethodEnum is used to express a required compression on ZipArchive.AddPart calls. 
    /// Values in the enumeration correspond to the binary format of the ZipArchive's Compression Method field  
    /// </summary>
    internal enum CompressionMethodEnum : ushort // takes 2 bytes in data structure 
    {
        Stored = 0, 
        Deflated = 8
    }
}
