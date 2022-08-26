//---------------------------------------------------------------------------
//
// <copyright file="StreamResourceInfo.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
// Description:
//      Class definition for StreamResourceInfo, it will keep the 
//      information for a given stream resource, such as .jpg, .ico
//      etc.
//
// Spec:  http://avalon/app/Navigation/Resource%20Loading%20Spec.doc
//              
// History:
//  03/24/2003: weibz  Created
//  06/12/2003: weibz  Moved to WCP tree.
//
//---------------------------------------------------------------------------
using System.IO;

using System;

namespace System.Windows.Resources
{
    /// <summary>
    /// Class StreamResourceInfo
    /// </summary>
    public class StreamResourceInfo
    {

        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        /// <summary>
        /// default costructor
        /// </summary>
        public StreamResourceInfo()
        {
        }

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="stream"></param>
        /// <param name="contentType"></param>
        /// <returns></returns>
        public StreamResourceInfo(Stream stream, String contentType)
        {
            _stream = stream;
            _contentType = contentType;
        }
        
        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        #region Public Properties

        /// <summary>
        /// ContentType
        /// </summary>
        public string ContentType 
        { 
            get { return _contentType;  }
        }
 
        /// <summary>
        /// Stream for the resource
        /// </summary>
        public Stream Stream
        { 
            get { return _stream;  }
        }

        #endregion Public Properties

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private string  _contentType;
        private Stream  _stream;

        #endregion Private Fields
    }

    /// <summary>
    /// class ContentTypes
    /// </summary>
    public sealed class ContentTypes
    {
        /// <summary>
        /// XamlContenType
        /// </summary>
        public const string XamlContentType = "applicaton/xaml+xml" ;
    }
}
