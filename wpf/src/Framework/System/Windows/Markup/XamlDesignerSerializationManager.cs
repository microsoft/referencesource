//---------------------------------------------------------------------------
//
// File: XamlDesignerSerializationManager.cs
//
// Description:
//   Manages the ContextStack for a particular run of Serialization.
//
// Copyright (C) 2003 by Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Reflection;
using System.Xml;

namespace System.Windows.Markup
{
    /// <summary>
    ///     The serialization manager offers three services
    ///     1. To store all of the context information 
    ///         for the current run of serialization on a stack. 
    ///     2. To query a given type for its serializer.
    ///     3. To get and set the serialization mode for 
    ///         a given Expression type
    /// </summary>
    /// <remarks>
    ///     As a measure of optimization it also 
    ///     maintains a cache mapping types to 
    ///     serializers, to avoid the overhead of 
    ///     reflecting for the attribute on every 
    ///     query.
    /// 
    ///     
    /// </remarks>
    //





    public class XamlDesignerSerializationManager : ServiceProviders
    {        
        #region Construction

        /// <summary>
        ///     Constructor for XamlDesignerSerializationManager
        /// </summary>
        /// <param name="xmlWriter">
        ///     XmlWriter
        /// </param>
        public XamlDesignerSerializationManager(XmlWriter xmlWriter)
        {
            _xamlWriterMode = XamlWriterMode.Value;
            _xmlWriter = xmlWriter;
        }

        #endregion Construction

        #region Properties

        /// <summary>
        ///     The mode of serialization for 
        ///     all Expressions
        /// </summary>
        public XamlWriterMode XamlWriterMode
        {
            get
            {
                return _xamlWriterMode;
            }

            set
            {
                // Validate Input Arguments
                if (!IsValidXamlWriterMode(value)) 
                {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(XamlWriterMode));
                }

                _xamlWriterMode = value;
            }
        }

        /// <summary>
        ///     XmlWriter
        /// </summary>
        internal XmlWriter XmlWriter
        {
            get { return _xmlWriter; }
        }

        #endregion Properties

        #region Internal Methods

        internal void ClearXmlWriter()
        {
            _xmlWriter = null;
        }
        
        #endregion

        #region Private Methods

        private static bool IsValidXamlWriterMode(XamlWriterMode value)
        {
            return value == XamlWriterMode.Value 
                || value == XamlWriterMode.Expression;
        }

        #endregion


        #region Data

        private XamlWriterMode _xamlWriterMode; // Serialization modes
        private XmlWriter _xmlWriter; //XmlWriter

        #endregion Data

    }
}

