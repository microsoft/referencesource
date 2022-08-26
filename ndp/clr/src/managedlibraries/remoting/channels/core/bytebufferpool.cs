// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       ByteBufferPool.cs
//
//  Summary:    Stream used for reading from a socket by remoting channels.
//
//==========================================================================

using System;
using System.Threading;


namespace System.IO
{

    internal interface IByteBufferPool
    {
        byte[] GetBuffer();
        void ReturnBuffer(byte[] buffer);
    }


    // This isn't actually a buffer pool. It always creates a new byte buffer.
    internal class ByteBufferAllocator : IByteBufferPool
    {
        private int _bufferSize;

        public ByteBufferAllocator(int bufferSize)
        {
            _bufferSize = bufferSize;
        }

        public byte[] GetBuffer()
        {
            return new byte[_bufferSize];
        }

        public void ReturnBuffer(byte[] buffer)
        {
        }
        
    } // ByteBufferAllocator


    internal class ByteBufferPool : IByteBufferPool
    {
        private byte[][] _bufferPool = null;
    
        private int _current; // -1 for none
        private int _last;
        private int _max;     // maximum number of buffers to pool

        private int _bufferSize;

        private Object _controlCookie = "cookie object";


        public ByteBufferPool(int maxBuffers, int bufferSize)
        {
            _max = maxBuffers;        
            _bufferPool = new byte[_max][];
            _bufferSize = bufferSize;

            _current = -1;
            _last = -1;
        } // ByteBufferPool        
        

        public byte[] GetBuffer()
        {
            Object cookie = null;

            try
            {
                // If a ThreadAbortException gets thrown after the exchange,
                //   but before the result is assigned to cookie, then the
                //   control cookie is lost forever. However, the buffer pool
                //   will still function normally and return everybody a new
                //   buffer each time (that isn't very likely to happen,
                //   so we don't really care).
                cookie = Interlocked.Exchange(ref _controlCookie, null);

                if (cookie != null)
                {
                    // we have the control cookie, so take a buffer
                
                    if (_current == -1)
                    {
                        _controlCookie = cookie;
                        // no pooled buffers available
                        return new byte[_bufferSize];
                    }
                    else
                    {
                        // grab next available buffer
                        byte[] buffer = _bufferPool[_current];
                        _bufferPool[_current] = null;      

                        // update "current" index
                        if (_current == _last)
                        {
                            // this is the last free buffer
                            _current = -1;
                        }
                        else
                        {
                            _current = (_current + 1) % _max;
                        }
    
                        _controlCookie = cookie;
                        return buffer;
                    }              
                }
                else
                {
                    // we don't have the control cookie, so just create a new buffer since
                    //   there will probably be a lot of contention anyway.
                    return new byte[_bufferSize];
                }            
            } 
            catch (ThreadAbortException)
            {
                if (cookie != null)
                {
                    // This should be rare, so just reset
                    //   everything to the initial state.
                    _current = -1;
                    _last = -1;

                    // restore cookie
                    _controlCookie = cookie;
                }
            
                throw;
            }                            
        } // GetBuffer


        public void ReturnBuffer(byte[] buffer)
        {
            if (buffer == null)
                throw new ArgumentNullException("buffer");
                
        
            // The purpose of the buffer pool is to try to reduce the 
            //   amount of garbage generated, so it doesn't matter  if
            //   the buffer gets tossed out. Since we don't want to
            //   take the perf hit of taking a lock, we only return
            //   the buffer if we can grab the control cookie.
            
            Object cookie = null;

            try
            {
                // If a ThreadAbortException gets thrown after the exchange,
                //   but before the result is assigned to cookie, then the
                //   control cookie is lost forever. However, the buffer pool
                //   will still function normally and return everybody a new
                //   buffer each time (that isn't very likely to happen,
                //   so we don't really care).
                cookie = Interlocked.Exchange(ref _controlCookie, null);
                
                if (cookie != null)
                {
                    if (_current == -1)
                    {
                        _bufferPool[0] = buffer;
                        _current = 0;
                        _last = 0;
                    }
                    else
                    {
                        int newLast = (_last + 1) % _max;
                        if (newLast != _current)
                        {
                            // the pool isn't full so store this buffer
                            _last = newLast;
                            _bufferPool[_last] = buffer;
                        }
                    }

                    _controlCookie = cookie;
                }            
            }
            catch (ThreadAbortException)
            {
                if (cookie != null)
                {
                    // This should be rare, so just reset
                    //   everything to the initial state.
                    _current = -1;
                    _last = -1;

                    // restore cookie
                    _controlCookie = cookie;
                }

                throw;            
            }
        } // ReturnBuffer

        

    } // ByteBufferPool


} // namespace System.IO
