// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       PipeStream.cs
//
//  Summary:    Stream used for reading from a named pipe
//  Author:     Microsoft@microsoft.com
//==========================================================================

using System;
using System.IO;
using System.Runtime.Remoting;

namespace System.Runtime.Remoting.Channels.Ipc
{

    // Basically the same as NetworkStream, but adds support for timeouts.
    internal sealed class  PipeStream : Stream
    {
        private IpcPort _port;        
        private int    _timeout = 0; // throw timout exception if a read takes longer than this many milliseconds

        
        public PipeStream(IpcPort port) 
        {
            if (port == null)
                throw new ArgumentNullException("port");

            _port = port;
        } // SocketStream

        // Stream implementation
        public override bool CanRead { get { return true; } }
        public override bool CanSeek { get { return false; } }
        public override bool CanWrite { get { return true; } }

        public override long Length { get { throw new NotSupportedException(); } }

        public override long Position 
        {
            get { throw new NotSupportedException(); }
            set { throw new NotSupportedException(); }
        } // Position

        public override long Seek(long offset, SeekOrigin origin) 
        {
            throw new NotSupportedException();
        }
    
        public override int Read(byte[] buffer, int offset, int size) 
        {
            if (_timeout <= 0)
            {
                return _port.Read(buffer, offset, size);
            }
            else
            {
                IAsyncResult ar = _port.BeginRead(buffer, offset, size, null, null);
                if (_timeout>0 && !ar.IsCompleted) 
                {
                    ar.AsyncWaitHandle.WaitOne(_timeout, false);
                    if (!ar.IsCompleted)
                        throw new RemotingTimeoutException();
                    
                }
                return _port.EndRead(ar);
            }
        } // Read

        public override void Write(byte[] buffer, int offset, int count) 
        {
            _port.Write(buffer, offset, count);
        } // Write

        protected override void Dispose(bool disposing)
        {
            try {
                if (disposing)
                    _port.Dispose();
            }
            finally {
                base.Dispose(disposing);
            }
        }
        
        public override void Flush() { }

      
        public override IAsyncResult BeginRead(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            Object state) 
        {
            IAsyncResult asyncResult =
                _port.BeginRead(
                    buffer,
                    offset,
                    size,
                    callback,
                    state);

            return asyncResult;
        } // BeginRead


        public override int EndRead(IAsyncResult asyncResult)
        {
            return _port.EndRead(asyncResult);
        } // EndRead

  
        public override IAsyncResult BeginWrite(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            Object state) 
        {
            throw new NotSupportedException();
        } // BeginWrite


        public override void EndWrite(IAsyncResult asyncResult) 
        {
            throw new NotSupportedException();
        } // EndWrite


        public override void SetLength(long value) { throw new NotSupportedException(); }
        
    } // class SocketStream
    
} // namespace System.Runtime.Remoting.Channels
