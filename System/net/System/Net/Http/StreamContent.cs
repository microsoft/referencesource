using System.Diagnostics.Contracts;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http
{
    public class StreamContent : HttpContent
    {
        private const int defaultBufferSize = 4096;

        private Stream content;
        private int bufferSize;
        private bool contentConsumed;
        private long start;

        public StreamContent(Stream content)
            : this(content, defaultBufferSize)
        {
        }

        public StreamContent(Stream content, int bufferSize)
        {
            if (content == null)
            {
                throw new ArgumentNullException("content");
            }
            if (bufferSize <= 0)
            {
                throw new ArgumentOutOfRangeException("bufferSize");
            }

            this.content = content;
            this.bufferSize = bufferSize;
            if (content.CanSeek)
            {
                start = content.Position;
            }
            if (Logging.On) Logging.Associate(Logging.Http, this, content);
        }

        protected override Task SerializeToStreamAsync(Stream stream, TransportContext context)
        {
            Contract.Assert(stream != null);

            PrepareContent();
            // If the stream can't be re-read, make sure that it gets disposed once it is consumed.
            StreamToStreamCopy sc = new StreamToStreamCopy(content, stream, bufferSize, !content.CanSeek);
            return sc.StartAsync();
        }

        protected internal override bool TryComputeLength(out long length)
        {
            if (content.CanSeek)
            {
                length = content.Length - start;
                return true;
            }
            else
            {
                length = 0;
                return false;
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                content.Dispose();
            }
            base.Dispose(disposing);
        }

        protected override Task<Stream> CreateContentReadStreamAsync()
        {
            // Wrap the stream with a read-only stream to prevent someone from writing to the stream. Note that the
            // caller can still write to the stream since he has a reference to it. However, if the content gets 
            // passed to other components (e.g. channel), they should not be able to write to the stream.
#if NET_4
            TaskCompletionSource<Stream> tcs = new TaskCompletionSource<Stream>();
            tcs.TrySetResult(new ReadOnlyStream(content));
            return tcs.Task;
#else
            return Task.FromResult<Stream>(new ReadOnlyStream(content));
#endif
        }

        private void PrepareContent()
        {
            if (contentConsumed)
            {
                // If the content needs to be written to a target stream a 2nd time, then the stream must support
                // seeking (e.g. a FileStream), otherwise the stream can't be copied a second time to a target 
                // stream (e.g. a NetworkStream).
                if (content.CanSeek)
                {
                    content.Position = start;
                }
                else
                {
                    throw new InvalidOperationException(SR.net_http_content_stream_already_read);
                }
            }

            contentConsumed = true;
        }

        private class ReadOnlyStream : DelegatingStream
        {
            public override bool CanWrite
            {
                get { return false; }
            }

            public override int WriteTimeout
            {
                get { throw new NotSupportedException(SR.net_http_content_readonly_stream); }
                set { throw new NotSupportedException(SR.net_http_content_readonly_stream); }
            }

            public ReadOnlyStream(Stream innerStream) 
                : base(innerStream) 
            { 
            }

            public override void Flush()
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }
#if !NET_4
            public override Task FlushAsync(CancellationToken cancellationToken)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }
#endif
            public override void SetLength(long value)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }

            public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }

            public override void EndWrite(IAsyncResult asyncResult)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }

            public override void WriteByte(byte value)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }
#if !NET_4
            public override Task WriteAsync(byte[] buffer, int offset, int count, Threading.CancellationToken cancellationToken)
            {
                throw new NotSupportedException(SR.net_http_content_readonly_stream);
            }
#endif
        }
    }
}
