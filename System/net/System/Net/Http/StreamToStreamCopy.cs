using System.Diagnostics.Contracts;
using System.IO;
using System.Threading.Tasks;

namespace System.Net.Http
{
    // This helper class is used to copy the content of a source stream to a destination stream using APM methods. 
    // The type verifies if the source and/or destination stream are MemoryStreams (or derived types. If so, sync 
    // read/write is used on the MemoryStream to avoid context switches.
    internal class StreamToStreamCopy
    {
        private byte[] buffer;
        private int bufferSize;
        private Stream source;
        private Stream destination;
        private AsyncCallback bufferReadCallback;
        private AsyncCallback bufferWrittenCallback;
        private TaskCompletionSource<object> tcs;
        private bool sourceIsMemoryStream;
        private bool destinationIsMemoryStream;
        private bool disposeSource;

        public StreamToStreamCopy(Stream source, Stream destination, int bufferSize, bool disposeSource)
        {
            Contract.Requires(source != null);
            Contract.Requires(destination != null);
            Contract.Requires(bufferSize > 0);

            this.buffer = new byte[bufferSize];
            this.source = source;
            this.destination = destination;
            this.sourceIsMemoryStream = source is MemoryStream;
            this.destinationIsMemoryStream = destination is MemoryStream;
            this.bufferSize = bufferSize;
            this.bufferReadCallback = BufferReadCallback;
            this.bufferWrittenCallback = BufferWrittenCallback;
            this.disposeSource = disposeSource;
            this.tcs = new TaskCompletionSource<object>();
        }

        public Task StartAsync()
        {
            // If both streams are MemoryStreams, just copy the whole content at once to avoid context switches.
            // This will not block since it will just result in a memcopy.
            if (sourceIsMemoryStream && destinationIsMemoryStream)
            {
                // Note that we can't use source.GetBuffer() since the MemoryStream may have been created without
                // exposing the buffer publicly (i.e. we would get an UnauthorizedAccessException). Since there is 
                // no property to determine if the buffer is publicly visible, we just use ToArray().
                MemoryStream sourceMemoryStream = source as MemoryStream;
                Contract.Assert(sourceMemoryStream != null);

                // Make sure exceptions are caught and set on the Task.
                try
                {
                    int position = (int)sourceMemoryStream.Position;
                    destination.Write(sourceMemoryStream.ToArray(), position, (int)source.Length - position);
                    SetCompleted(null);
                }
                catch (Exception e)
                {
                    SetCompleted(e);
                }
            }
            else
            {
                StartRead();
            }

            return tcs.Task;
        }

        private void StartRead()
        {
            int bytesRead = 0;
            bool completedSync = false;

            // If an exception is thrown, catch it and set the task as faulted.
            try
            {
                do
                {
                    if (sourceIsMemoryStream)
                    {
                        // If the source is a memory stream, just read sync to avoid context switches.
                        bytesRead = source.Read(buffer, 0, bufferSize);

                        if (bytesRead == 0)
                        {
                            SetCompleted(null);
                            return;
                        }

                        completedSync = TryStartWriteSync(bytesRead);
                    }
                    else
                    {
                        IAsyncResult arSource = source.BeginRead(buffer, 0, bufferSize, bufferReadCallback, null);
                        completedSync = arSource.CompletedSynchronously;

                        if (completedSync)
                        {
                            bytesRead = source.EndRead(arSource);

                            if (bytesRead == 0)
                            {
                                SetCompleted(null);
                                return;
                            }

                            completedSync = TryStartWriteSync(bytesRead);
                        }
                    }

                } while (completedSync);
            }
            catch (Exception e)
            {
                SetCompleted(e);
            }
        }

        private bool TryStartWriteSync(int bytesRead)
        {
            Contract.Requires(bytesRead > 0);

            // This method returns 'true' if the write operation to the destination stream completed sync.

            if (destinationIsMemoryStream)
            {
                // If the destination is a memory stream, just write sync to avoid context switches.
                destination.Write(buffer, 0, bytesRead);
                return true;
            }
            else
            {
                IAsyncResult arDestination = destination.BeginWrite(buffer, 0, bytesRead,
                    bufferWrittenCallback, null);

                if (arDestination.CompletedSynchronously)
                {
                    destination.EndWrite(arDestination);
                    return true;
                }

                return false;
            }
        }

        private void BufferReadCallback(IAsyncResult ar)
        {
            // If we completed sync, then EndRead() is called by the caller of buffer.BeginRead().
            if (!ar.CompletedSynchronously)
            {
                try
                {
                    int bytesRead = source.EndRead(ar);

                    if (bytesRead == 0)
                    {
                        SetCompleted(null);
                    }
                    else
                    {
                        if (TryStartWriteSync(bytesRead))
                        {
                            StartRead();
                        }
                    }
                }
                catch (Exception e)
                {
                    SetCompleted(e); 
                }
            }
        }

        private void BufferWrittenCallback(IAsyncResult ar)
        {
            // If we completed sync, then EndWrite() is called by the caller of buffer.BeginWrite().
            if (!ar.CompletedSynchronously)
            {
                try
                {
                    destination.EndWrite(ar);
                    StartRead();
                }
                catch (Exception e)
                {
                    SetCompleted(e);
                }
            }
        }

        private void SetCompleted(Exception error)
        {
            try
            {
                if (disposeSource)
                {
                    source.Dispose();
                }
            }
            catch (Exception e)
            {
                // Dispose() should never throw, but since we're on an async codepath, make sure to catch the exception.
                if (Logging.On) Logging.Exception(Logging.Http, this, "SetCompleted", e);
            }

            if (error == null)
            {
                tcs.TrySetResult(null);
            }
            else
            {
                tcs.TrySetException(error); // It's OK if false is returned.
            }
        }
    }
}
