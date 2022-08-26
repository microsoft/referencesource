#if !DONOTREFPRINTINGASMMETA
//---------------------------------------------------------------------------
//
// <copyright file=ISerializerFactory.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: Plug-in document serializers implement this interface
//
//              See spec at <Need to post existing spec>
// 
// History:  
//  07/16/2005 : oliverfo - Created
//
//---------------------------------------------------------------------------
namespace System.Windows.Documents.Serialization
{
    using System;
    using System.IO;

    /// <summary>
    /// ISerializerFactory is implemented by an assembly containing a plug-in serializer and provides
    /// functionality to instantiate the associated serializer
    /// </summary>
    public interface ISerializerFactory
    {
        /// <summary>
        /// Create a SerializerWriter on the passed in stream
        /// </summary>
        SerializerWriter CreateSerializerWriter(Stream stream);
        /// <summary>
        /// Return the DisplayName of the serializer.
        /// </summary>
        string DisplayName
        {
            get;
        }
        /// <summary>
        /// Return the ManufacturerName of the serializer.
        /// </summary>
        string ManufacturerName
        {
            get;
        }
        /// <summary>
        /// Return the ManufacturerWebsite of the serializer.
        /// </summary>
        Uri ManufacturerWebsite
        {
            get;
        }
        /// <summary>
        /// Return the DefaultFileExtension of the serializer.
        /// </summary>
        string DefaultFileExtension
        {
            get;
        }
    }
}
#endif
