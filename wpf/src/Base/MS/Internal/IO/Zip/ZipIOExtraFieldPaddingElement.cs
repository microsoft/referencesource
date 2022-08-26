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
// <copyright file="ZipIoExtraFieldPaddingElement.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  This is an internal class that is used to implement parsing and editing of a padding extra field
//  structure. This extra field is used to optimize the performance dealing with Zip64 extra field.
//  When we need to create Zip64 extra field the extra padding bytes are used to create such
//  structure so that the subsequent bytes don't have to be moved.
//
// History:
//  03/23/2006: Microsoft: Added support for Padding Extra Field
//
//-----------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.IO;
using System.Windows;

namespace MS.Internal.IO.Zip
{
    /// <summary>
    /// This class is used to represent and parse the Padding Extra Field
    ///
    /// The Padding Extra Field is defined as:
    /// Header ID               UInt16 (2 bytes)    0xA220
    /// Length of the field     UInt16 (2 bytes)
    /// Signature               UInt16 (2 bytes)    0xA028 (for verification)
    /// Padding Initial Value   UInt16 (2 bytes)    Padding size originally requested by the caller
    /// Padding                 ? bytes             Padding respresented by null characters
    /// </summary>                
    internal class ZipIOExtraFieldPaddingElement  : ZipIOExtraFieldElement
    {
        // creates a brand new empty Padding extra field element 
        internal static ZipIOExtraFieldPaddingElement CreateNew()
        {
            ZipIOExtraFieldPaddingElement newElement = new ZipIOExtraFieldPaddingElement();

            return newElement;
        }

        // parse Padding extra field element
        internal override void ParseDataField(BinaryReader reader, UInt16 size)
        {
            // The signature is already read and validated
            Debug.Assert(size >= MinimumSize);

            // It is guaranteed that there is enough bytes to read in UInt16
            //  since it is already checked from ZipIOExtraFieldElement.Parse()
            _initialRequestedPaddingSize = reader.ReadUInt16();

            // need to subtract the size of the signature and size fields
            // We don't need to use checked{} since size is guaranteed to be bigger than
            //  MinimumFieldDataSize by the called (ZipIOExtraFieldElement.Parse)
            size -= _minimumFieldDataSize;

            _paddingSize = size;

            // Skip the padding
            if (_paddingSize != 0)
            {
                reader.BaseStream.Seek(size, SeekOrigin.Current);
            }
        }

        internal static bool MatchesPaddingSignature(byte[] sniffiedBytes)
        {
            if (sniffiedBytes.Length < _signatureSize)
            {
                return false;
            }

            Debug.Assert(sniffiedBytes.Length == _signatureSize);

            if (BitConverter.ToUInt16(sniffiedBytes, 0) != _signature)
            {
                return false;
            }

            return true;
        }

        internal override void Save(BinaryWriter writer)
        {
            writer.Write(_constantFieldId);
            writer.Write(SizeField);
            writer.Write(_signature);
            writer.Write(_initialRequestedPaddingSize);

            for (int i = 0; i < _paddingSize; ++i)
            {
                writer.Write((byte) 0);
            }
        }

        // This property calculates size of the field on disk (how many bytes need to be allocated on the disk)
        //  id + size field + data field (SizeField)
        internal override UInt16 Size
        {
            get
            {
                return checked((UInt16) (SizeField + MinimumSize));
            }
        }

        // This property calculates the value of the size record whch holds the size without the Id and without the size itself.
        // we are always guranteed that Size == SizeField + 2 * sizeof(UInt16))
        internal override UInt16 SizeField
        {
            get
            {
                return checked((UInt16) (_minimumFieldDataSize + _paddingSize));
            }
        }

        static internal UInt16 ConstantFieldId
        {
            get
            {
                return _constantFieldId; 
            }
        }
 
        static internal UInt16 MinimumFieldDataSize
        {
            get
            {
                return _minimumFieldDataSize; 
            }
        }
 
        static internal UInt16 SignatureSize
        {
            get
            {
                return _signatureSize; 
            }
        }
 
        internal UInt16 PaddingSize
        {
            get
            {
                return _paddingSize; 
            }
            set
            {
                _paddingSize = value;
            }
        }

        //------------------------------------------------------
        //
        //  Private Constructor 
        //
        //------------------------------------------------------
        internal ZipIOExtraFieldPaddingElement() : base(_constantFieldId)
        {
            _initialRequestedPaddingSize = _newInitialPaddingSize;
            _paddingSize = _initialRequestedPaddingSize;
        }

        //------------------------------------------------------
        //
        //  Private Methods 
        //
        //------------------------------------------------------

        //------------------------------------------------------
        //
        //  Private fields 
        //
        //------------------------------------------------------
        private const UInt16 _constantFieldId = 0xA220;
        private const UInt16 _signature = 0xA028;
        // 20 is the maximum size of Zip64 Extra Field element we can use for Local File Header
        //  (Id, Size, Compressed and UnCompressed Sizes; 2 + 2 + 8 + 8 = 20)
        //  DiskNumber and Relative Offset of Local File Header is not counted since we don't use
        //  it in Local File Header
        private const UInt16 _newInitialPaddingSize = 20;

        // UInt16 signature
        // UInt16 initial requested padding size
        private static readonly UInt16 _minimumFieldDataSize = 2 * sizeof(UInt16);
        private static readonly UInt16 _signatureSize = sizeof(UInt16);

        private UInt16 _paddingSize;
        private UInt16 _initialRequestedPaddingSize;
    }
}

            
