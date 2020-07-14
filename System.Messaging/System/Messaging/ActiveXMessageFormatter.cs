//------------------------------------------------------------------------------
// <copyright file="ActiveXMessageFormatter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Globalization;
    using System.ComponentModel;
    using System.Messaging.Interop;

    /// <include file='doc\ActiveXMessageFormatter.uex' path='docs/doc[@for="ActiveXMessageFormatter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Formatter class that serializes and deserializes
    ///       primitives, classes, enumeration, and other objects into and from <see cref='System.Messaging.MessageQueue'/>
    ///       messages using binary format.
    ///    </para>
    /// </devdoc>
    public class ActiveXMessageFormatter : IMessageFormatter
    {
        internal const short VT_ARRAY = 0x2000;
        internal const short VT_BOOL = 11;
        internal const short VT_BSTR = 8;
        internal const short VT_CLSID = 72;
        internal const short VT_CY = 6;
        internal const short VT_DATE = 7;
        internal const short VT_I1 = 16;
        internal const short VT_I2 = 2;
        internal const short VT_I4 = 3;
        internal const short VT_I8 = 20;
        internal const short VT_LPSTR = 30;
        internal const short VT_LPWSTR = 31;
        internal const short VT_NULL = 1;
        internal const short VT_R4 = 4;
        internal const short VT_R8 = 5;
        internal const short VT_STREAMED_OBJECT = 68;
        internal const short VT_STORED_OBJECT = 69;
        internal const short VT_UI1 = 17;
        internal const short VT_UI2 = 18;
        internal const short VT_UI4 = 19;
        internal const short VT_UI8 = 21;
        internal const short VT_VECTOR = 0x1000;
        private byte[] internalBuffer;
        private UnicodeEncoding unicodeEncoding;
        private ASCIIEncoding asciiEncoding;
        private char[] internalCharBuffer;

        /// <include file='doc\ActiveXMessageFormatter.uex' path='docs/doc[@for="ActiveXMessageFormatter.CanRead"]/*' />
        /// <devdoc>
        ///    When this method is called, the formatter will attempt to determine 
        ///    if the contents of the message are something the formatter can deal with.
        /// </devdoc>
        public bool CanRead(Message message)
        {
            if (message == null)
                throw new ArgumentNullException("message");

            int variantType = message.BodyType;
            if (variantType != VT_BOOL && variantType != VT_CLSID &&
                variantType != VT_CY && variantType != VT_DATE &&
                variantType != VT_I1 && variantType != VT_UI1 &&
                variantType != VT_I2 && variantType != VT_UI2 &&
                variantType != VT_I4 && variantType != VT_UI4 &&
                variantType != VT_I8 && variantType != VT_UI8 &&
                variantType != VT_NULL && variantType != VT_R4 &&
                variantType != VT_I8 && variantType != VT_STREAMED_OBJECT &&
                variantType != VT_STORED_OBJECT &&
                variantType != (VT_VECTOR | VT_UI1) &&
                variantType != VT_LPSTR && variantType != VT_LPWSTR &&
                variantType != VT_BSTR && variantType != VT_R8)
                return false;

            return true;
        }

        /// <include file='doc\ActiveXMessageFormatter.uex' path='docs/doc[@for="ActiveXMessageFormatter.Clone"]/*' />
        /// <devdoc>
        ///    This method is needed to improve scalability on Receive and ReceiveAsync scenarios.  Not requiring 
        ///     thread safety on read and write.
        /// </devdoc>
        public object Clone()
        {
            return new ActiveXMessageFormatter();
        }

        /// <include file='doc\ActiveXMessageFormatter.uex' path='docs/doc[@for="ActiveXMessageFormatter.InitStreamedObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void InitStreamedObject(object streamedObject)
        {
            IPersistStreamInit persistStreamInit = streamedObject as IPersistStreamInit;
            if (persistStreamInit != null)
                persistStreamInit.InitNew();
        }

        /// <include file='doc\ActiveXMessageFormatter.uex' path='docs/doc[@for="ActiveXMessageFormatter.Read"]/*' />
        /// <devdoc>
        ///    This method is used to read the contents from the given message 
        ///     and create an object.
        /// </devdoc>
        public object Read(Message message)
        {
            if (message == null)
                throw new ArgumentNullException("message");

            Stream stream;
            byte[] bytes;
            byte[] newBytes;
            int size;
            int variantType = message.BodyType;
            switch (variantType)
            {
                case VT_LPSTR:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    size = message.properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE);

                    if (this.internalCharBuffer == null || this.internalCharBuffer.Length < size)
                        this.internalCharBuffer = new char[size];

                    if (asciiEncoding == null)
                        this.asciiEncoding = new ASCIIEncoding();

                    this.asciiEncoding.GetChars(bytes, 0, size, this.internalCharBuffer, 0);
                    return new String(this.internalCharBuffer, 0, size);
                case VT_BSTR:
                case VT_LPWSTR:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    size = message.properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE) / 2;

                    if (this.internalCharBuffer == null || this.internalCharBuffer.Length < size)
                        this.internalCharBuffer = new char[size];

                    if (unicodeEncoding == null)
                        this.unicodeEncoding = new UnicodeEncoding();

                    this.unicodeEncoding.GetChars(bytes, 0, size * 2, this.internalCharBuffer, 0);
                    return new String(this.internalCharBuffer, 0, size);
                case VT_VECTOR | VT_UI1:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    size = message.properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE);
                    newBytes = new byte[size];
                    Array.Copy(bytes, newBytes, size);

                    return newBytes;
                case VT_BOOL:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    newBytes = new byte[1];
                    Array.Copy(bytes, newBytes, 1);
                    if (bytes[0] != 0)
                        return true;

                    return false;
                case VT_CLSID:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    newBytes = new byte[16];
                    Array.Copy(bytes, newBytes, 16);
                    return new Guid(newBytes);
                case VT_CY:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    newBytes = new byte[8];
                    Array.Copy(bytes, newBytes, 8);
                    return Decimal.FromOACurrency(BitConverter.ToInt64(newBytes, 0));
                case VT_DATE:
                    bytes = message.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);
                    newBytes = new byte[8];
                    Array.Copy(bytes, newBytes, 8);
                    return new DateTime(BitConverter.ToInt64(newBytes, 0));
                case VT_I1:
                case VT_UI1:
                    stream = message.BodyStream;
                    bytes = new byte[1];
                    stream.Read(bytes, 0, 1);
                    return bytes[0];
                case VT_I2:
                    stream = message.BodyStream;
                    bytes = new byte[2];
                    stream.Read(bytes, 0, 2);
                    return BitConverter.ToInt16(bytes, 0);
                case VT_UI2:
                    stream = message.BodyStream;
                    bytes = new byte[2];
                    stream.Read(bytes, 0, 2);
                    return BitConverter.ToUInt16(bytes, 0);
                case VT_I4:
                    stream = message.BodyStream;
                    bytes = new byte[4];
                    stream.Read(bytes, 0, 4);
                    return BitConverter.ToInt32(bytes, 0);
                case VT_UI4:
                    stream = message.BodyStream;
                    bytes = new byte[4];
                    stream.Read(bytes, 0, 4);
                    return BitConverter.ToUInt32(bytes, 0);
                case VT_I8:
                    stream = message.BodyStream;
                    bytes = new byte[8];
                    stream.Read(bytes, 0, 8);
                    return BitConverter.ToInt64(bytes, 0);
                case VT_UI8:
                    stream = message.BodyStream;
                    bytes = new byte[8];
                    stream.Read(bytes, 0, 8);
                    return BitConverter.ToUInt64(bytes, 0);
                case VT_R4:
                    stream = message.BodyStream;
                    bytes = new byte[4];
                    stream.Read(bytes, 0, 4);
                    return BitConverter.ToSingle(bytes, 0);
                case VT_R8:
                    stream = message.BodyStream;
                    bytes = new byte[8];
                    stream.Read(bytes, 0, 8);
                    return BitConverter.ToDouble(bytes, 0);
                case VT_NULL:
                    return null;
                case VT_STREAMED_OBJECT:
                    stream = message.BodyStream;
                    ComStreamFromDataStream comStream = new ComStreamFromDataStream(stream);
                    return NativeMethods.OleLoadFromStream(comStream, ref NativeMethods.IID_IUnknown);
                case VT_STORED_OBJECT:
                    throw new NotSupportedException(Res.GetString(Res.StoredObjectsNotSupported));
                default:
                    throw new InvalidOperationException(Res.GetString(Res.InvalidTypeDeserialization));
            }
        }

        /// <include file='doc\ActiveXMessageFormatter.uex' path='docs/doc[@for="ActiveXMessageFormatter.Write"]/*' />
        /// <devdoc>
        ///    This method is used to write the given object into the given message.  
        ///     If the formatter cannot understand the given object, an exception is thrown.
        /// </devdoc>
        public void Write(Message message, object obj)
        {
            if (message == null)
                throw new ArgumentNullException("message");

            Stream stream;
            int variantType;
            if (obj is string)
            {
                int size = ((string)obj).Length * 2;
                if (this.internalBuffer == null || this.internalBuffer.Length < size)
                    this.internalBuffer = new byte[size];

                if (unicodeEncoding == null)
                    this.unicodeEncoding = new UnicodeEncoding();

                this.unicodeEncoding.GetBytes(((string)obj).ToCharArray(), 0, size / 2, this.internalBuffer, 0);
                message.properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, this.internalBuffer);
                message.properties.AdjustSize(NativeMethods.MESSAGE_PROPID_BODY, size);
                message.properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE, size);
                message.properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_TYPE, VT_LPWSTR);
                return;
            }
            else if (obj is byte[])
            {
                byte[] bytes = (byte[])obj;
                if (this.internalBuffer == null || this.internalBuffer.Length < bytes.Length)
                    this.internalBuffer = new byte[bytes.Length];

                Array.Copy(bytes, this.internalBuffer, bytes.Length);
                message.properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, this.internalBuffer);
                message.properties.AdjustSize(NativeMethods.MESSAGE_PROPID_BODY, bytes.Length);
                message.properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE, bytes.Length);
                message.properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_TYPE, VT_UI1 | VT_VECTOR);
                return;
            }
            else if (obj is char[])
            {
                char[] chars = (char[])obj;
                int size = chars.Length * 2;
                if (this.internalBuffer == null || this.internalBuffer.Length < size)
                    this.internalBuffer = new byte[size];

                if (unicodeEncoding == null)
                    this.unicodeEncoding = new UnicodeEncoding();

                this.unicodeEncoding.GetBytes(chars, 0, size / 2, this.internalBuffer, 0);
                message.properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, this.internalBuffer);
                message.properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE, size);
                message.properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_TYPE, VT_LPWSTR);
                return;
            }
            else if (obj is byte)
            {
                stream = new MemoryStream(1);
                stream.Write(new byte[] { (byte)obj }, 0, 1);
                variantType = VT_UI1;
            }
            else if (obj is bool)
            {
                stream = new MemoryStream(1);
                if ((bool)obj)
                    stream.Write(new byte[] { 0xff }, 0, 1);
                else
                    stream.Write(new byte[] { 0x00 }, 0, 1);
                variantType = VT_BOOL;
            }
            else if (obj is char)
            {
                stream = new MemoryStream(2);
                byte[] bytes = BitConverter.GetBytes((Char)obj);
                stream.Write(bytes, 0, 2);
                variantType = VT_UI2;
            }
            else if (obj is Decimal)
            {
                stream = new MemoryStream(8);
                byte[] bytes = BitConverter.GetBytes(Decimal.ToOACurrency((Decimal)obj));
                stream.Write(bytes, 0, 8);
                variantType = VT_CY;
            }
            else if (obj is DateTime)
            {
                stream = new MemoryStream(8);
                byte[] bytes = BitConverter.GetBytes(((DateTime)obj).Ticks);
                stream.Write(bytes, 0, 8);
                variantType = VT_DATE;
            }
            else if (obj is Double)
            {
                stream = new MemoryStream(8);
                byte[] bytes = BitConverter.GetBytes((Double)obj);
                stream.Write(bytes, 0, 8);
                variantType = VT_R8;
            }
            else if (obj is Int16)
            {
                stream = new MemoryStream(2);
                byte[] bytes = BitConverter.GetBytes((short)obj);
                stream.Write(bytes, 0, 2);
                variantType = VT_I2;
            }
            else if (obj is UInt16)
            {
                stream = new MemoryStream(2);
                byte[] bytes = BitConverter.GetBytes((UInt16)obj);
                stream.Write(bytes, 0, 2);
                variantType = VT_UI2;
            }
            else if (obj is Int32)
            {
                stream = new MemoryStream(4);
                byte[] bytes = BitConverter.GetBytes((int)obj);
                stream.Write(bytes, 0, 4);
                variantType = VT_I4;
            }
            else if (obj is UInt32)
            {
                stream = new MemoryStream(4);
                byte[] bytes = BitConverter.GetBytes((UInt32)obj);
                stream.Write(bytes, 0, 4);
                variantType = VT_UI4;
            }
            else if (obj is Int64)
            {
                stream = new MemoryStream(8);
                byte[] bytes = BitConverter.GetBytes((Int64)obj);
                stream.Write(bytes, 0, 8);
                variantType = VT_I8;
            }
            else if (obj is UInt64)
            {
                stream = new MemoryStream(8);
                byte[] bytes = BitConverter.GetBytes((UInt64)obj);
                stream.Write(bytes, 0, 8);
                variantType = VT_UI8;
            }
            else if (obj is Single)
            {
                stream = new MemoryStream(4);
                byte[] bytes = BitConverter.GetBytes((float)obj);
                stream.Write(bytes, 0, 4);
                variantType = VT_R4;
            }
            else if (obj is IPersistStream)
            {
                IPersistStream pstream = (IPersistStream)obj;
                ComStreamFromDataStream comStream = new ComStreamFromDataStream(new MemoryStream());
                NativeMethods.OleSaveToStream(pstream, comStream);
                stream = comStream.GetDataStream();
                variantType = VT_STREAMED_OBJECT;
            }
            else if (obj == null)
            {
                stream = new MemoryStream();
                variantType = VT_NULL;
            }
            else
            {
                throw new InvalidOperationException(Res.GetString(Res.InvalidTypeSerialization));
            }

            message.BodyStream = stream;
            message.BodyType = variantType;
        }

        [ComVisible(false)]
        private class ComStreamFromDataStream : IStream
        {
            private Stream dataStream;

            // to support seeking ahead of the stream length...
            private long virtualPosition = -1;

            public ComStreamFromDataStream(Stream dataStream)
            {
                if (dataStream == null) throw new ArgumentNullException("dataStream");
                this.dataStream = dataStream;
            }


            private void ActualizeVirtualPosition()
            {
                if (virtualPosition == -1) return;

                if (virtualPosition > dataStream.Length)
                    dataStream.SetLength(virtualPosition);

                dataStream.Position = virtualPosition;

                virtualPosition = -1;
            }

            public IStream Clone()
            {
                NotImplemented();
                return null;
            }

            public void Commit(int grfCommitFlags)
            {
                dataStream.Flush();
                // Extend the length of the file if needed.
                ActualizeVirtualPosition();
            }

            public long CopyTo(IStream pstm, long cb, long[] pcbRead)
            {
                int bufSize = 4096;
                IntPtr buffer = Marshal.AllocHGlobal((IntPtr)bufSize);
                if (buffer == IntPtr.Zero) throw new OutOfMemoryException();
                long written = 0;
                try
                {
                    while (written < cb)
                    {
                        int toRead = bufSize;
                        if (written + toRead > cb) toRead = (int)(cb - written);
                        int read = Read(buffer, toRead);
                        if (read == 0) break;
                        if (pstm.Write(buffer, read) != read)
                        {
                            throw EFail(Res.GetString(Res.IncorrectNumberOfBytes));
                        }
                        written += read;
                    }
                }
                finally
                {
                    Marshal.FreeHGlobal(buffer);
                }
                if (pcbRead != null && pcbRead.Length > 0)
                {
                    pcbRead[0] = written;
                }

                return written;
            }

            public Stream GetDataStream()
            {
                return dataStream;
            }

            public void LockRegion(long libOffset, long cb, int dwLockType)
            {
            }

            protected static ExternalException EFail(string msg)
            {
                ExternalException e = new ExternalException(msg, NativeMethods.E_FAIL);
                throw e;
            }

            protected static void NotImplemented()
            {
                ExternalException e = new ExternalException(Res.GetString(Res.NotImplemented), NativeMethods.E_NOTIMPL);
                throw e;
            }

            public int Read(IntPtr buf, int length)
            {
                byte[] buffer = new byte[length];
                int count = Read(buffer, length);
                Marshal.Copy(buffer, 0, buf, length);
                return count;
            }

            public int Read(byte[] buffer, int length)
            {
                ActualizeVirtualPosition();
                return dataStream.Read(buffer, 0, length);
            }

            public void Revert()
            {
                NotImplemented();
            }

            public long Seek(long offset, int origin)
            {
                long pos = virtualPosition;
                if (virtualPosition == -1)
                {
                    pos = dataStream.Position;
                }
                long len = dataStream.Length;
                switch (origin)
                {
                    case NativeMethods.STREAM_SEEK_SET:
                        if (offset <= len)
                        {
                            dataStream.Position = offset;
                            virtualPosition = -1;
                        }
                        else
                        {
                            virtualPosition = offset;
                        }
                        break;
                    case NativeMethods.STREAM_SEEK_END:
                        if (offset <= 0)
                        {
                            dataStream.Position = len + offset;
                            virtualPosition = -1;
                        }
                        else
                        {
                            virtualPosition = len + offset;
                        }
                        break;
                    case NativeMethods.STREAM_SEEK_CUR:
                        if (offset + pos <= len)
                        {
                            dataStream.Position = pos + offset;
                            virtualPosition = -1;
                        }
                        else
                        {
                            virtualPosition = offset + pos;
                        }
                        break;
                }
                if (virtualPosition != -1)
                {
                    return virtualPosition;
                }
                else
                {
                    return dataStream.Position;
                }
            }

            public void SetSize(long value)
            {
                dataStream.SetLength(value);
            }

            public void Stat(IntPtr pstatstg, int grfStatFlag)
            {
                // GpStream has a partial implementation, but it's so partial rather 
                // restrict it to use with GDI+
                NotImplemented();
            }

            public void UnlockRegion(long libOffset, long cb, int dwLockType)
            {
            }

            public int Write(IntPtr buf, int length)
            {
                byte[] buffer = new byte[length];
                Marshal.Copy(buf, buffer, 0, length);
                return Write(buffer, length);
            }

            public int Write(byte[] buffer, int length)
            {
                ActualizeVirtualPosition();
                dataStream.Write(buffer, 0, length);
                return length;
            }
        }
    }
}
