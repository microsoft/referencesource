// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       ChunkedMemoryStream.cs
//
//  Summary:    Memory stream that doesn't need to be resized.
//
//==========================================================================

using System;
using System.IO;
using System.Runtime.Remoting.Channels;

namespace System.Runtime.Remoting.Channels
{

    internal class ChunkedMemoryStream : Stream
    {                               
        private class MemoryChunk
        {
            public byte[] Buffer = null;
            public MemoryChunk Next = null;
        }

        // state
        private MemoryChunk     _chunks = null;      // data
        private IByteBufferPool _bufferPool = null;  // pool of byte buffers to use

        private bool        _bClosed = false;   // has the stream been closed.        
        
        private MemoryChunk _writeChunk = null; // current chunk to write to
        private int         _writeOffset = 0; // offset into chunk to write to
        private MemoryChunk _readChunk = null; // current chunk to read from
        private int         _readOffset = 0;  // offset into chunk to read from


        public ChunkedMemoryStream(IByteBufferPool bufferPool)
        {
            _bufferPool = bufferPool;
        } // ChunkedMemoryStream


        public override bool CanRead { get { return true; } }
        public override bool CanSeek { get { return true; } }
        public override bool CanWrite { get { return true; } }
        
        public override long Length  
        { 
            get 
            { 
                if (_bClosed)
                {                
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
                }

            
                int length = 0;
                MemoryChunk chunk = _chunks;
                while (chunk != null)
                {
                    MemoryChunk next = chunk.Next;
                    if (next != null)
                        length += chunk.Buffer.Length;
                    else
                        length += _writeOffset;
        
                    chunk = next;
                }

                return (long)length;
            }
        } // Length        

        public override long Position
        {
             get 
             {
                if (_bClosed)
                {                
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
                }
                
                if (_readChunk == null)
                    return 0;

                int pos = 0;
                MemoryChunk chunk = _chunks;
                while (chunk != _readChunk)
                {
                    pos += chunk.Buffer.Length;
                    chunk = chunk.Next;
                }
                pos += _readOffset;

                return (long)pos;
             }
             
             set 
             {
                if (_bClosed)
                {                
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
                }
                
                if (value < 0)
                    throw new ArgumentOutOfRangeException("value");

                // back up current position in case new position is out of range
                MemoryChunk backupReadChunk = _readChunk;
                int backupReadOffset = _readOffset;

                _readChunk = null;
                _readOffset = 0;
                                    
                int leftUntilAtPos = (int)value;
                MemoryChunk chunk = _chunks;
                while (chunk != null)
                {
                    if ((leftUntilAtPos < chunk.Buffer.Length) ||
                            ((leftUntilAtPos == chunk.Buffer.Length) &&
                             (chunk.Next == null)))
                    {
                        // the desired position is in this chunk
                        _readChunk = chunk;
                        _readOffset = leftUntilAtPos; 
                        break;
                    }

                    leftUntilAtPos -= chunk.Buffer.Length;
                    chunk = chunk.Next;
                }

                if (_readChunk == null)
                {
                    // position is out of range
                    _readChunk = backupReadChunk;
                    _readOffset = backupReadOffset;
                    throw new ArgumentOutOfRangeException("value");
                }                          
             }
        } // Position

        public override long Seek(long offset, SeekOrigin origin) 
        { 
            if (_bClosed)
            {                
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
            }
            
            switch(origin) 
            {
            case SeekOrigin.Begin: 
                Position = offset; 
                break;
    			
            case SeekOrigin.Current:
                Position = Position + offset;
    			break;
    			
    		case SeekOrigin.End:
    		    Position = Length + offset;
    		    break;
    	    }

    		return Position;
        } // Seek

        
        public override void SetLength(long value) { throw new NotSupportedException(); }

        protected override void Dispose(bool disposing) 
        {
            try {
                _bClosed = true;
                if (disposing)
                    ReleaseMemoryChunks(_chunks);
                _chunks = null;
                _writeChunk = null;
                _readChunk = null;
            }
            finally {
                base.Dispose(disposing);
            }
        } // Close

        public override void Flush()
        {
        } // Flush

        
        public override int Read(byte[] buffer, int offset, int count)
        {
            if (_bClosed)
            {                
                throw new RemotingException(
                CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
            }        
            
            if (_readChunk == null)
            {
                if (_chunks == null)
                    return 0;
                _readChunk = _chunks;
                _readOffset = 0;
            }
            
            byte[] chunkBuffer = _readChunk.Buffer;
            int chunkSize = chunkBuffer.Length;
            if (_readChunk.Next == null)
                chunkSize = _writeOffset;

            int bytesRead = 0;
        
            while (count > 0)
            {
                if (_readOffset == chunkSize)
                {
                    // exit if no more chunks are currently available
                    if (_readChunk.Next == null)
                        break;
                        
                    _readChunk = _readChunk.Next;
                    _readOffset = 0;
                    chunkBuffer = _readChunk.Buffer;
                    chunkSize = chunkBuffer.Length;
                    if (_readChunk.Next == null)
                        chunkSize = _writeOffset;
                }

                int readCount = Math.Min(count, chunkSize - _readOffset);
                Buffer.BlockCopy(chunkBuffer, _readOffset, buffer, offset, readCount);
                offset += readCount;
                count -= readCount;
                _readOffset += readCount;
                bytesRead += readCount;
            }

            return bytesRead;
        } // Read

        public override int ReadByte()
        {
            if (_bClosed)
            {                
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
            }        
            
            if (_readChunk == null)
            {
                if (_chunks == null)
                    return 0;
                _readChunk = _chunks;
                _readOffset = 0;
            }
            
            byte[] chunkBuffer = _readChunk.Buffer;
            int chunkSize = chunkBuffer.Length;
            if (_readChunk.Next == null)
                chunkSize = _writeOffset;

            if (_readOffset == chunkSize)
            {
                // exit if no more chunks are currently available
                if (_readChunk.Next == null)
                    return -1;
                        
                _readChunk = _readChunk.Next;
                _readOffset = 0;
                chunkBuffer = _readChunk.Buffer;
                chunkSize = chunkBuffer.Length;
                if (_readChunk.Next == null)
                    chunkSize = _writeOffset;
            }

            return chunkBuffer[_readOffset++];
        } // ReadByte
                
        public override void Write(byte[] buffer, int offset, int count)
        {
            if (_bClosed)
            {                
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
            }
        
            if (_chunks == null)
            {
                _chunks = AllocateMemoryChunk();
                _writeChunk = _chunks;
                _writeOffset = 0;
            }            

            byte[] chunkBuffer = _writeChunk.Buffer;
            int chunkSize = chunkBuffer.Length;
    
            while (count > 0)
            {
                if (_writeOffset == chunkSize)
                {
                    // allocate a new chunk if the current one is full
                    _writeChunk.Next = AllocateMemoryChunk();
                    _writeChunk = _writeChunk.Next;
                    _writeOffset = 0;
                    chunkBuffer = _writeChunk.Buffer;
                    chunkSize = chunkBuffer.Length;
                }
                             
                int copyCount = Math.Min(count, chunkSize - _writeOffset);
                Buffer.BlockCopy(buffer, offset, chunkBuffer, _writeOffset, copyCount);
                offset += copyCount;
                count -= copyCount;
                _writeOffset += copyCount;
            }
            
        } // Write

        public override void WriteByte(byte value)
        {
            if (_bClosed)
            {                
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
            }
        
            if (_chunks == null)
            {
                _chunks = AllocateMemoryChunk();
                _writeChunk = _chunks;
                _writeOffset = 0;
            }            

            byte[] chunkBuffer = _writeChunk.Buffer;
            int chunkSize = chunkBuffer.Length;
    
            if (_writeOffset == chunkSize)
            {
                // allocate a new chunk if the current one is full
                _writeChunk.Next = AllocateMemoryChunk();
                _writeChunk = _writeChunk.Next;
                _writeOffset = 0;
                chunkBuffer = _writeChunk.Buffer;
                chunkSize = chunkBuffer.Length;
            }
            
            chunkBuffer[_writeOffset++] = value;
        } // WriteByte


        // copy entire buffer into an array
        public virtual byte[] ToArray() 
        {
            int length = (int)Length; // this will throw if stream is closed
            byte[] copy = new byte[Length];

            MemoryChunk backupReadChunk = _readChunk;
            int backupReadOffset = _readOffset;

            _readChunk = _chunks;
            _readOffset = 0;            
            Read(copy, 0, length);

            _readChunk = backupReadChunk;
            _readOffset = backupReadOffset;           
            
            return copy;
        } // ToArray      


        // write remainder of this stream to another stream
        public virtual void WriteTo(Stream stream)
        {
            if (_bClosed)
            {                
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Stream_StreamIsClosed"));
            }
            
            if (stream == null)
                throw new ArgumentNullException("stream");

            if (_readChunk == null)
            {
                if (_chunks == null)
                    return;

                _readChunk = _chunks;
                _readOffset = 0;
            }

            byte[] chunkBuffer = _readChunk.Buffer;
            int chunkSize = chunkBuffer.Length;
            if (_readChunk.Next == null)
                chunkSize = _writeOffset;

            // following code mirrors Read() logic (_readChunk/_readOffset should
            //   point just past last byte of last chunk when done)

            for (;;) // loop until end of chunks is found
            {
                if (_readOffset == chunkSize)
                {
                    // exit if no more chunks are currently available
                    if (_readChunk.Next == null)
                        break;
                        
                    _readChunk = _readChunk.Next;
                    _readOffset = 0;
                    chunkBuffer = _readChunk.Buffer;
                    chunkSize = chunkBuffer.Length;
                    if (_readChunk.Next == null)
                        chunkSize = _writeOffset;
                }

                int writeCount = chunkSize - _readOffset;
                stream.Write(chunkBuffer, _readOffset, writeCount);
                _readOffset = chunkSize;
            }
                
        } // WriteTo



        private MemoryChunk AllocateMemoryChunk()
        {
            MemoryChunk chunk = new MemoryChunk();
            chunk.Buffer = _bufferPool.GetBuffer();
            chunk.Next = null;

            return chunk;
        } // AllocateMemoryChunk

        private void ReleaseMemoryChunks(MemoryChunk chunk)
        {
            // If the buffer pool always allocates a new buffer,
            //   there's no point to trying to return all of the buffers. 
            if (_bufferPool is ByteBufferAllocator)
                return;

            while (chunk != null)
            {
                _bufferPool.ReturnBuffer(chunk.Buffer);
                chunk = chunk.Next;
            }
                
        } // FreeMemoryChunk

    
    } // ChunkedMemoryStream


} // namespace System.IO

