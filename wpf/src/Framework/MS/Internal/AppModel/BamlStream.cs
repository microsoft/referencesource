//-----------------------------------------------------------------------------
//
// <copyright file="BamlStream.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
// BamlStream is stream wrapper, It contains a raw baml stream and 
// the assembly which hosts the baml stream.
//
// History:
//  05/17/2006: weibz: Initial creation.
//
//-----------------------------------------------------------------------------

using System;
using System.IO;
using System.Reflection;
using System.Resources;
using System.Globalization;
using System.Runtime.Remoting;
using System.Windows.Markup;
using System.Security;

namespace MS.Internal.AppModel
{

    // <summary>
    // BamlStream is stream wrapper, It contains a raw baml stream and 
    // the assembly which hosts the baml stream.
    // </summary>
    internal class BamlStream : Stream, IStreamInfo
    {
        //------------------------------------------------------
        //
        //  Constructor
        //
        //------------------------------------------------------

        #region Constructor

        /// <SecurityNote>
        ///     Critical - because it stores the assembly passed in to it the _assembly field that is marked
        ///                SecurityCriticalDataForSet, and this field is used by the BamlRecordReader to
        ///                allow legitimate internal types in Partial Trust.
        /// </SecurityNote>
        [SecurityCritical]
        internal BamlStream(Stream stream, Assembly assembly)
        {
            _assembly.Value = assembly;
            _stream = stream;
        }
        #endregion

        //------------------------------------------------------
        //
        //  Internal Property
        //
        //------------------------------------------------------

        #region Internal Property

        //
        // Assembly which contains the Baml stream data.
        //
        Assembly IStreamInfo.Assembly
        {
            get { return _assembly.Value; }
        }

        #endregion

        //------------------------------------------------------
        //
        //  Overridden Properties
        //
        //------------------------------------------------------

        #region Overridden Properties

        // <summary>
        // Overridden CanRead Property
        // </summary>
        public override bool CanRead
        {
            get { return _stream.CanRead; }
        }

        // <summary>
        // Overridden CanSeek Property
        // </summary>
        public override bool CanSeek
        {
            get { return _stream.CanSeek; }
        }

        // <summary>
        // Overridden CanWrite Property
        // </summary>
        public override bool CanWrite
        {
            get { return _stream.CanWrite; }
        }

        // <summary>
        // Overridden Length Property
        // </summary>
        public override long Length
        {
            get { return _stream.Length;  }
        }

        // <summary>
        // Overridden Position Property
        // </summary>
        public override long Position
        {
            get {  return _stream.Position;  }
            set {  _stream.Position = value; }
        }

        #endregion Overridden Properties

        #region Overridden Public Methods

        // <summary>
        // Overridden BeginRead method
        // </summary>
        public override IAsyncResult BeginRead(
            byte[] buffer,
            int offset,
            int count,
            AsyncCallback callback,
            object state
            )
        {
            return _stream.BeginRead(buffer, offset, count, callback, state);
        }

        // <summary>
        // Overridden BeginWrite method
        // </summary>
        public override IAsyncResult BeginWrite(
            byte[] buffer,
            int offset,
            int count,
            AsyncCallback callback,
            object state
            )
        {
            return _stream.BeginWrite(buffer, offset, count, callback, state);
        }

        // <summary>
        // Overridden Close method
        // </summary>
        public override void Close()
        {
            _stream.Close();
        }

        // <summary>
        // Overridden EndRead method
        // </summary>
        public override int EndRead(
            IAsyncResult asyncResult
            )
        {
            return _stream.EndRead(asyncResult);
        }

        // <summary>
        // Overridden EndWrite method
        // </summary>
        public override void EndWrite(
            IAsyncResult asyncResult
            )
        {
            _stream.EndWrite(asyncResult);
        }

        // <summary>
        // Overridden Equals method
        // </summary>
        public override bool Equals(
            object obj
            )
        {
            return _stream.Equals(obj);
        }

        // <summary>
        // Overridden Flush method
        // </summary>
        public override void Flush()
        {
            _stream.Flush();
        }

        // <summary>
        // Overridden GetHashCode method
        // </summary>
        public override int GetHashCode()
        {
            return _stream.GetHashCode();
        }

        // <summary>
        // Overridden Read method
        // </summary>
        public override int Read(
            byte[] buffer,
            int offset,
            int count
            )
        {
            return _stream.Read(buffer, offset, count);
        }

        // <summary>
        // Overridden ReadByte method
        // </summary>
        public override int ReadByte()
        {
            return _stream.ReadByte();
        }

        // <summary>
        // Overridden Seek method
        // </summary>
        public override long Seek(
            long offset,
            SeekOrigin origin
            )
        {
            return _stream.Seek(offset, origin);
        }

        // <summary>
        // Overridden SetLength method
        // </summary>
        public override void SetLength(
            long value
            )
        {
            _stream.SetLength(value);
        }

        // <summary>
        // Overridden ToString method
        // </summary>
        public override string ToString()
        {
            return _stream.ToString();
        }

        // <summary>
        // Overridden Write method
        // </summary>
        public override void Write(
            byte[] buffer,
            int offset,
            int count
            )
        {
            _stream.Write(buffer, offset, count);
        }

        // <summary>
        // Overridden WriteByte method
        // </summary>
        public override void WriteByte(
            byte value
            )
        {
            _stream.WriteByte(value);
        }

        #endregion Overridden Public Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Members

        private SecurityCriticalDataForSet<Assembly> _assembly;
        private Stream   _stream = null;
        
        #endregion Private Members
    }
}

