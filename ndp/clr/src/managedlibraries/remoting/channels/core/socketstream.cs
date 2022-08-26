// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       SocketStream.cs
//
//  Summary:    Stream used for reading from a socket by remoting channels.
//
//==========================================================================

using System;
using System.IO;
using System.Runtime.Remoting;
using System.Net;
using System.Net.Sockets;


namespace System.Runtime.Remoting.Channels
{

    // Basically the same as NetworkStream, but adds support for timeouts.
    internal sealed class SocketStream : Stream
    {
        private Socket _socket;        
        private int    _timeout = 0; // throw timout exception if a read takes longer than this many milliseconds
        // as per http://support.microsoft.com/default.aspx?scid=kb%3ben-us%3b201213
        // we shouldn't write more than 64K synchronously to a socket
        const int maxSocketWrite = 64 * 1024;   // 64k
        const int maxSocketRead =  4 * 1024 * 1024;  // 4 MB

        
        public SocketStream(Socket socket) 
        {
            if (socket == null)
                throw new ArgumentNullException("socket");

            _socket = socket;
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
                    return _socket.Receive(buffer, offset, Math.Min(size, maxSocketRead), SocketFlags.None);
            }
            else
            {
                IAsyncResult ar = _socket.BeginReceive(buffer, offset, Math.Min(size, maxSocketRead), SocketFlags.None, null, null);
                if (_timeout>0 && !ar.IsCompleted) 
                {
                    ar.AsyncWaitHandle.WaitOne(_timeout, false);
                    if (!ar.IsCompleted)
                        throw new RemotingTimeoutException();
                    
                }
                return _socket.EndReceive(ar);
            }
        } // Read

        public override void Write(byte[] buffer, int offset, int count) 
        {
            int bytesToWrite = count;

            while (bytesToWrite > 0)
            {
                count = Math.Min(bytesToWrite, maxSocketWrite);
                _socket.Send(buffer, offset, count, SocketFlags.None);
                bytesToWrite -= count;
                offset += count;
            }
        } // Write

        protected override void Dispose(bool disposing)
        {
            try {
                if (disposing)
                    _socket.Close();
            }
            finally {
                base.Dispose(disposing);
            }
        }
        
        public override void Flush() {}

      
        public override IAsyncResult BeginRead(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            Object state) 
        {
            IAsyncResult asyncResult =
                _socket.BeginReceive(
                    buffer,
                    offset,
                    Math.Min(size, maxSocketRead),
                    SocketFlags.None,
                    callback,
                    state);

            return asyncResult;
        } // BeginRead


        public override int EndRead(IAsyncResult asyncResult)
        {
            return _socket.EndReceive(asyncResult);
        } // EndRead

  
        public override IAsyncResult BeginWrite(
            byte[] buffer,
            int offset,
            int size,
            AsyncCallback callback,
            Object state) 
        {
            IAsyncResult asyncResult =
                _socket.BeginSend(
                    buffer,
                    offset,
                    size,
                    SocketFlags.None,
                    callback,
                    state);

                return asyncResult;
        } // BeginWrite


        public override void EndWrite(IAsyncResult asyncResult) 
        {
            _socket.EndSend(asyncResult);
        } // EndWrite


        public override void SetLength(long value) { throw new NotSupportedException(); }
        
    } // class SocketStream
    
} // namespace System.Runtime.Remoting.Channels
