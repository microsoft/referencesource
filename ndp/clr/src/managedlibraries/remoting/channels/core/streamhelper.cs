// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//============================================================
//
// File:    StreamHelper.cs
//
// Summary: Helper methods for streams.
//
//===========================================================


using System;
using System.IO;
using System.Runtime.Remoting;
using System.Threading;

namespace System.Runtime.Remoting.Channels
{

    internal static class StreamHelper
    {
        private static AsyncCallback _asyncCopyStreamReadCallback = new AsyncCallback(AsyncCopyStreamReadCallback);
        private static AsyncCallback _asyncCopyStreamWriteCallback = new AsyncCallback(AsyncCopyStreamWriteCallback);

        internal static void CopyStream(Stream source, Stream target)
        {
            if (source == null)
                return;

            // see if this is a ChunkedMemoryStream (we can do a direct write)
            ChunkedMemoryStream chunkedMemStream = source as ChunkedMemoryStream;
            if (chunkedMemStream != null)
            {
                chunkedMemStream.WriteTo(target);
            }
            else
            {
                // see if this is a MemoryStream (we can do a direct write)
                MemoryStream memContentStream = source as MemoryStream;
                if (memContentStream != null)
                {
                    memContentStream.WriteTo(target);
                }
                else                    
                {
                    // otherwise, we need to copy the data through an intermediate buffer
                
                    byte[] buffer = CoreChannel.BufferPool.GetBuffer();
                    int bufferSize = buffer.Length;
                    int readCount = source.Read(buffer, 0, bufferSize);
                    while (readCount > 0)
                    {
                        target.Write(buffer, 0, readCount);
                        readCount = source.Read(buffer, 0, bufferSize);
                    }   
                    CoreChannel.BufferPool.ReturnBuffer(buffer);
                }
            }
            
        } // CopyStream       
        


        internal static void BufferCopy(byte[] source, int srcOffset, 
                                        byte[] dest, int destOffset,
                                        int count)
        {
            if (count > 8)
            {
                Buffer.BlockCopy(source, srcOffset, dest, destOffset, count);
            }
            else
            {
                for (int co = 0; co < count; co++)
                    dest[destOffset + co] = source[srcOffset + co];
            }
                } // BufferCopy




        internal static IAsyncResult BeginAsyncCopyStream(
            Stream source, Stream target, 
            bool asyncRead, bool asyncWrite,
            bool closeSource, bool closeTarget,
            AsyncCallback callback, Object state)
        {   
            AsyncCopyStreamResult streamState = new AsyncCopyStreamResult(callback, state);

            byte[] buffer = CoreChannel.BufferPool.GetBuffer();

            streamState.Source = source;
            streamState.Target = target;
            streamState.Buffer = buffer;
            streamState.AsyncRead = asyncRead;
            streamState.AsyncWrite = asyncWrite;
            streamState.CloseSource = closeSource;
            streamState.CloseTarget = closeTarget;

            try
            {
                AsyncCopyReadHelper(streamState);
            } 
            catch (Exception e)
            {
                streamState.SetComplete(null, e);
            }

            return streamState;
        } // BeginAsyncCopyStream

        internal static void EndAsyncCopyStream(IAsyncResult iar)
        {
            AsyncCopyStreamResult asyncResult = (AsyncCopyStreamResult)iar;
        
            if (!iar.IsCompleted)
            {
                iar.AsyncWaitHandle.WaitOne();
            }

            if (asyncResult.Exception != null)
            {
                throw asyncResult.Exception;
            }
        } // EndAsyncCopyStream


        private static void AsyncCopyReadHelper(AsyncCopyStreamResult streamState)
        {
            // There is no try-catch here because the calling method always has a try-catch.
        
            if (streamState.AsyncRead)
            {
                byte[] buffer = streamState.Buffer;
                streamState.Source.BeginRead(buffer, 0, buffer.Length, _asyncCopyStreamReadCallback, streamState);
            }
            else
            {
                byte[] buffer = streamState.Buffer;
                int bytesRead = streamState.Source.Read(buffer, 0, buffer.Length);
                if (bytesRead == 0)
                {
                    streamState.SetComplete(null, null);
                }
                else
                if (bytesRead < 0)
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_Stream_UnknownReadError"));
                }
                else
                {
                    AsyncCopyWriteHelper(streamState, bytesRead);
                }
            }
        } // AsyncCopyReadHelper


        private static void AsyncCopyWriteHelper(AsyncCopyStreamResult streamState, int bytesRead)
        {
            // There is no try-catch here because the calling method always has a try-catch.
        
            if (streamState.AsyncWrite)
            {
                byte[] buffer = streamState.Buffer;
                streamState.Target.BeginWrite(buffer, 0, bytesRead, _asyncCopyStreamWriteCallback, streamState);
            }
            else
            {
                byte[] buffer = streamState.Buffer;
                streamState.Target.Write(buffer, 0, bytesRead);
                
                AsyncCopyReadHelper(streamState);
            }
        } // AsyncCopyWriteHelper
        

        private static void AsyncCopyStreamReadCallback(IAsyncResult iar)
        {            
            AsyncCopyStreamResult state = (AsyncCopyStreamResult)iar.AsyncState;

            try
            {
                Stream source = state.Source;

                int bytesRead = source.EndRead(iar);
                if (bytesRead == 0)
                {
                    state.SetComplete(null, null);
                }
                else
                if (bytesRead < 0)
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_Stream_UnknownReadError"));
                }
                else
                {
                    AsyncCopyWriteHelper(state, bytesRead);
                }           
            }
            catch (Exception e)
            {
                state.SetComplete(null, e);                
            }
        } // AsyncCopyStreamReadCallback


        private static void AsyncCopyStreamWriteCallback(IAsyncResult iar)
        {            
            AsyncCopyStreamResult state = (AsyncCopyStreamResult)iar.AsyncState;

            try
            {
                state.Target.EndWrite(iar);

                AsyncCopyReadHelper(state);
            }
            catch (Exception e)
            {
                state.SetComplete(null, e);                
            }
        } // AsyncCopyStreamWriteCallback
        
    } // class StreamHelper



    internal class AsyncCopyStreamResult : BasicAsyncResult
    {
        internal Stream Source;
        internal Stream Target;
        internal byte[] Buffer;
        internal bool AsyncRead;
        internal bool AsyncWrite;
        internal bool CloseSource;
        internal bool CloseTarget;

        internal AsyncCopyStreamResult(AsyncCallback callback, Object state) :
            base(callback, state)
        {
        }

        internal override void CleanupOnComplete()
        {
            if (Buffer != null)
                CoreChannel.BufferPool.ReturnBuffer(Buffer);

            if (CloseSource)
                Source.Close();
            if (CloseTarget)
                Target.Close();
                
        } // CleanupOnComplete
        
    } // class AsyncCopyStreamResult
    

} // namespace System.Runtime.Remoting.Channels
