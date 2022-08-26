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
// <copyright file="DeflateOptionEnum.cs" company="Microsoft">
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
    /// DeflateOptionEnum is used to express a required compression on ZipArchive.AddPart calls 
    /// Values in the enumeration (except None 0xFF) correspond to the binary format of the 
    /// ZipArchive's General Purpose Bit flags (bits 1 and 2). In order to match this value with the 
    /// General Purpose Bit flags, DeflateOptionEnum must be masked by the value 0x06
    /// 0xFF is a special value that is used to indicate "not applicable" for cases when data isn't deflated.  
    /// </summary>
    internal enum DeflateOptionEnum : byte // takes 2 bits in the data structure 
    {
        Normal = 0,         //values are selected based on their position in the General purpose bit flag 
        Maximum = 2,    // bits 1 and 2 
        Fast = 4,
        SuperFast = 6,
        None = 0xFF
    }
}
