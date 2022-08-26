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
// <copyright file="ZipIOVersionNeededToExtract.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  This is an internal enumeration that maintains version needed to extract values 
//  for OPC scenarios 
//
// History:
//  05/12/2005: IgorBel: Initial creation.
//
//-----------------------------------------------------------------------------

    
namespace MS.Internal.IO.Zip
{
    internal enum ZipIOVersionNeededToExtract : ushort 
    {
        StoredData = 10,
        VolumeLabel = 11,
        DeflatedData = 20,
        Zip64FileFormat = 45,
    }
}
