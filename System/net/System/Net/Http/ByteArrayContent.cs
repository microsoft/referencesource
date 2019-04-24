using System.Diagnostics.Contracts;
using System.IO;
using System.Threading.Tasks;

namespace System.Net.Http
{
    public class ByteArrayContent : HttpContent
    {
        private byte[] content;
        private int offset;
        private int count;

        public ByteArrayContent(byte[] content)
        {
            if (content == null)
            {
                throw new ArgumentNullException("content");
            }

            this.content = content;
            this.offset = 0;
            this.count = content.Length;
        }

        public ByteArrayContent(byte[] content, int offset, int count)
        {
            if (content == null)
            {
                throw new ArgumentNullException("content");
            }
            if ((offset < 0) || (offset > content.Length))
            {
                throw new ArgumentOutOfRangeException("offset");
            }
            if ((count < 0) || (count > (content.Length - offset)))
            {
                throw new ArgumentOutOfRangeException("count");
            }
            
            this.content = content;
            this.offset = offset;
            this.count = count;
        }

        protected override Task SerializeToStreamAsync(Stream stream, TransportContext context)
        {
            Contract.Assert(stream != null);

            return Task.Factory.FromAsync(stream.BeginWrite, stream.EndWrite, content, offset, count, null);
        }

        protected internal override bool TryComputeLength(out long length)
        {
            length = count;
            return true;
        }

        protected override Task<Stream> CreateContentReadStreamAsync()
        {
#if NET_4
            TaskCompletionSource<Stream> tcs = new TaskCompletionSource<Stream>();
            tcs.TrySetResult(new MemoryStream(content, offset, count, false, false));
            return tcs.Task;
#else
            return Task.FromResult<Stream>(new MemoryStream(content, offset, count, false, false));
#endif
        }
    }
}
