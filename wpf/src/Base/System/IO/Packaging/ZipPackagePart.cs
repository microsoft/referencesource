//-----------------------------------------------------------------------------
//
// <copyright file="ZipPackagePart.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  This is a subclass for the abstract PackagePart class.
//  This implementation is specific to Zip file format.
//
// History:
//  12/28/2004: SarjanaS: Initial creation. [BruceMac provided some of the
//                                           initial code]
//
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using MS.Internal.IO.Zip;
using MS.Internal.IO.Packaging;
using MS.Internal;                          // for Invariant

namespace System.IO.Packaging
{
    /// <summary>
    /// This class represents a Part within a Zip container.
    /// This is a part of the Packaging Layer APIs
    /// </summary>
   public sealed class ZipPackagePart : PackagePart
   {
        //------------------------------------------------------
        //
        //  Public Constructors
        //
        //------------------------------------------------------
        // None
        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------
        // None
        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        /// <summary>
        /// Custom Implementation for the GetStream Method
        /// </summary>
        /// <param name="mode">Mode in which the stream should be opened</param>
        /// <param name="access">Access with which the stream should be opened</param>
        /// <returns>Stream Corresponding to this part</returns>
        protected override Stream GetStreamCore(FileMode mode, FileAccess access)
        {
            if (Package.InStreamingCreation)
            {
                // Convert Metro CompressionOption to Zip CompressionMethodEnum.
                CompressionMethodEnum compressionMethod;
                DeflateOptionEnum deflateOption;
                ZipPackage.GetZipCompressionMethodFromOpcCompressionOption(this.CompressionOption,
                    out compressionMethod, out deflateOption);

                // Mode and access get validated in StreamingZipPartStream().
                return new StreamingZipPartStream(
                    PackUriHelper.GetStringForPartUri(this.Uri),
                    _zipArchive,
                    compressionMethod, deflateOption,
                    mode, access);
            }
            else if (_zipFileInfo != null)
            {
                // Case of an atomic part.
                return _zipFileInfo.GetStream(mode, access);
            }
            else
            {
                // Case of an interleaved part.
                Invariant.Assert(_pieces != null);
                return new InterleavedZipPartStream(this, mode, access);
            }
        }

        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------
        // None
        //------------------------------------------------------
        //
        //  Internal Constructors
        //
        //------------------------------------------------------
    
        #region Internal Constructors
        
        /// <summary>
        /// Constructs a ZipPackagePart for an atomic (i.e. non-interleaved) part.
        /// This is called from the ZipPackage class as a result of GetPartCore,
        /// GetPartsCore or CreatePartCore methods     
        /// </summary>
        /// <param name="container"></param>
        /// <param name="zipFileInfo"></param>
        /// <param name="partUri"></param>
        /// <param name="compressionOption"></param>
        /// <param name="contentType"></param>
        internal ZipPackagePart(ZipPackage container, 
            ZipFileInfo zipFileInfo,
            PackUriHelper.ValidatedPartUri partUri, 
            string contentType,
            CompressionOption compressionOption)
            :base(container, partUri, contentType, compressionOption)
        {
            _zipArchive = zipFileInfo.ZipArchive;
            _zipFileInfo = zipFileInfo;
        }

        /// <summary>
        /// Constructs a ZipPackagePart. This is called from ZipPackage.CreatePartCore in streaming
        /// production.
        /// No piece is created until the first write operation on the associated stream. Therefore
        /// this constructor does not take a ZipFileInfo.
        /// </summary>
        /// <param name="container"></param>
        /// <param name="zipArchive"></param>
        /// <param name="partUri"></param>
        /// <param name="compressionOption"></param>
        /// <param name="contentType"></param>
        internal ZipPackagePart(ZipPackage container, 
            ZipArchive zipArchive,
            PackUriHelper.ValidatedPartUri partUri, 
            string contentType,
            CompressionOption compressionOption)
            :base(container, partUri, contentType, compressionOption)
        {
            _zipArchive = zipArchive;
        }

        /// <summary>
        /// Constructs a ZipPackagePart for an interleaved part. This is called outside of streaming
        /// production when an interleaved part is encountered in the package.
        /// </summary>
        /// <param name="container"></param>
        /// <param name="zipArchive"></param>
        /// <param name="pieces"></param>
        /// <param name="partUri"></param>
        /// <param name="compressionOption"></param>
        /// <param name="contentType"></param>
        internal ZipPackagePart(ZipPackage container, 
            ZipArchive zipArchive,
            List<PieceInfo> pieces,
            PackUriHelper.ValidatedPartUri partUri, 
            string contentType,
            CompressionOption compressionOption)
            :base(container, partUri, contentType, compressionOption)
        {
            _zipArchive = zipArchive;
            _pieces = pieces;
        }

        #endregion Internal Constructors

        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------

       #region Internal Properties

       /// <summary>
       /// Obtain the sorted array of piece descriptors for an interleaved part.
       /// </summary>
       internal List<PieceInfo> PieceDescriptors
       {
           get
           {
               return _pieces;
           }
       }

       /// <summary>
       /// Obtain the ZipFileInfo descriptor of an atomic part.
       /// </summary>
       internal ZipFileInfo ZipFileInfo
       {
           get
           {
               return _zipFileInfo;
           }
       }

       #endregion Internal Properties

        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------
        // None
        //------------------------------------------------------
        //
        //  Internal Events
        //
        //------------------------------------------------------
        // None
        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------
        // None
        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------
        
        #region Private Variables

        // Zip item info for an atomic part.
        private ZipFileInfo     _zipFileInfo;

        // Piece descriptors for an interleaved part.
        private List<PieceInfo> _pieces;

        //ZipArhive
        private ZipArchive     _zipArchive;

        #endregion Private Variables
        
        //------------------------------------------------------
    }
}
