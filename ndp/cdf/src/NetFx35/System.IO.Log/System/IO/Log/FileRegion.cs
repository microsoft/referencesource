//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Diagnostics;
    using System.IO;

    public sealed class FileRegion
    {
        long fileLength;
        string path;
        long offset;
        long length;
        byte[] fixedContent;

        internal FileRegion(long fileLength,
                            string path,
                            long offset,
                            long length)
        {
            this.fileLength = fileLength;
            this.path = path;
            this.offset = offset;
            this.length = length;
        }

        internal FileRegion(long fileLength,
                            string path,
                            long offset,
                            byte[] fixedContent)
        {
            this.fileLength = fileLength;
            this.path = path;
            this.offset = offset;
            this.length = fixedContent.LongLength;
            this.fixedContent = fixedContent;
        }

        public long FileLength
        {
            get
            {
                return this.fileLength;
            }
        }

        public long Offset
        {
            get
            {
                return this.offset;
            }
        }

        public string Path
        {
            get
            {
                return this.path;
            }
        }

        public Stream GetStream()
        {
            if (this.fixedContent != null)
            {
                return new MemoryStream(this.fixedContent,
                                        0,
                                        this.fixedContent.Length,
                                        false,
                                        false);
            }
            else
            {
                Stream innerStream = new FileStream(this.path,
                                                    FileMode.Open,
                                                    FileAccess.Read,
                                                    FileShare.ReadWrite);
                return new Substream(innerStream,
                                     this.offset,
                                     this.length);
            }
        }

        class Substream : Stream
        {
            Stream innerStream;
            long offset;
            long length;

            internal Substream(Stream innerStream,
                               long offset,
                               long length)
            {
                this.innerStream = innerStream;
                this.offset = offset;
                this.length = length;
            }

            public override bool CanRead { get { return true; } }
            public override bool CanSeek { get { return true; } }
            public override bool CanWrite { get { return false; } }

            public override long Length
            {
                get
                {
                    return this.length;
                }
            }

            public override long Position
            {
                get
                {
                    return this.innerStream.Position - this.offset;
                }

                set
                {
                    this.innerStream.Position = value + this.offset;
                }
            }

            public override void Close()
            {
                this.innerStream.Close();
                base.Close();
            }

            public override void Flush()
            {
                this.innerStream.Flush();
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                // Truncate read if it goes past the length of the
                // stream...
                //
                if (this.Length - this.Position < count)
                {
                    count = checked((int)(this.Length - this.Position));
                    if (count < 0)
                        return 0;
                }

                return this.innerStream.Read(buffer,
                                             offset,
                                             count);
            }

            public override long Seek(long offset, SeekOrigin origin)
            {
                if (origin == SeekOrigin.Begin)
                {
                    offset += this.offset;
                }
                else if (origin == SeekOrigin.End)
                {
                    offset = this.offset + this.length - offset;
                    origin = SeekOrigin.Begin;
                }

                return this.innerStream.Seek(offset, origin);
            }

            public override void SetLength(long value)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
            }

            public override void WriteByte(byte value)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(Error.NotSupported());
            }
        }
    }
}
