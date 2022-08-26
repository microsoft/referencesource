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
// <copyright file="ZipFileInfoCollection.cs" company="Microsoft">
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
using System.Collections.Generic;
using System.Collections;
    
namespace MS.Internal.IO.Zip
{
    /// <summary>
    /// The only reason for existence of this class is to restrict operations that caller of the 
    /// ZipArchive.GetFiles is allowed to perform. We want to prevent any modifications to the 
    /// actual collection of the FileItems as it is supposed to be a read-only data structure. 
    /// Although this is an internal API it seems that the safeguards are warranted.
    /// </summary>
    internal class ZipFileInfoCollection : IEnumerable
    {
        //------------------------------------------------------
        //
        // Internal NON API Constructor (this constructor is marked as internal 
        // and isNOT part of the ZIP IO API surface 
        //
        //------------------------------------------------------
        internal ZipFileInfoCollection(ICollection zipFileInfoCollection)
        {
            _zipFileInfoCollection = zipFileInfoCollection;
        }

        //------------------------------------------------------
        //
        // Internal API Methods (although these methods are marked as 
        // Internal they are part of the internal ZIP IO API surface 
        //
        //------------------------------------------------------
        IEnumerator IEnumerable.GetEnumerator()
        {
            return _zipFileInfoCollection.GetEnumerator();
        }

        //------------------------------------------------------
        //
        //  Private Fields 
        //
        //------------------------------------------------------        
        private ICollection _zipFileInfoCollection;
    }
}
