using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

/// <summary>
///     Provides asynchronous wrappers for .NET Framework operations.
/// </summary>
public static partial class AsyncExtensions
{
    ///<summary>
    ///Reads a sequence of bytes from the current stream and advances the position within the stream by the number of bytes read.
    ///</summary>
    ///<returns>A Task that represents the asynchronous read.</returns>
    ///<param name="source">The source.</param>
    ///<param name="buffer">The buffer to read data into. </param>
    ///<param name="offset">The byte offset in  at which to begin reading. </param>
    ///<param name="count">The maximum number of bytes to read. </param>
    ///<exception cref="T:System.ArgumentException">The array length minus  is less than <paramref name="count" />. </exception>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException"> or <paramref name="count" /> is negative. </exception>
    ///<exception cref="T:System.IO.IOException">An asynchronous read was attempted past the end of the file. </exception>
    public static Task<int> ReadAsync(this System.IO.Stream source, byte[] buffer, int offset, int count)
    {
        return ReadAsync(source, buffer, offset, count, System.Threading.CancellationToken.None);
    }

    ///<summary>
    ///Reads a sequence of bytes from the current stream and advances the position within the stream by the number of bytes read.
    ///</summary>
    ///<returns>A Task that represents the asynchronous read.</returns>
    ///<param name="source">The source.</param>
    ///<param name="buffer">The buffer to read data into. </param>
    ///<param name="offset">The byte offset in  at which to begin reading. </param>
    ///<param name="count">The maximum number of bytes to read. </param>
    /// <param name="cancellationToken">The cancellation token.</param>
    ///<exception cref="T:System.ArgumentException">The array length minus  is less than <paramref name="count" />. </exception>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException"> or <paramref name="count" /> is negative. </exception>
    ///<exception cref="T:System.IO.IOException">An asynchronous read was attempted past the end of the file. </exception>
    public static Task<int> ReadAsync(this System.IO.Stream source, byte[] buffer, int offset, int count, System.Threading.CancellationToken cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested)
            return TaskServices.FromCancellation<int>(cancellationToken);
        return Task<int>.Factory.FromAsync(source.BeginRead, source.EndRead, buffer, offset, count, null);
    }

    ///<summary>
    ///Writes asynchronously a sequence of bytes to the current stream and advances the current position within this stream by the number of bytes written.
    ///</summary>
    ///<returns>A Task that represents the asynchronous write.</returns>
    ///<param name="source">The source.</param>
    ///<param name="buffer">The buffer containing data to write to the current stream.</param>
    ///<param name="offset">The zero-based byte offset in  at which to begin copying bytes to the current stream.</param>
    ///<param name="count">The maximum number of bytes to write. </param>
    ///<exception cref="T:System.ArgumentException"> length minus <paramref name="offset" /> is less than <paramref name="count" />. </exception>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException"> or <paramref name="count" /> is negative. </exception>
    ///<exception cref="T:System.NotSupportedException">The stream does not support writing. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The stream is closed. </exception>
    ///<exception cref="T:System.IO.IOException">An I/O error occurred. </exception>
    public static Task WriteAsync(this System.IO.Stream source, byte[] buffer, int offset, int count)
    {
        return WriteAsync(source, buffer, offset, count, System.Threading.CancellationToken.None);
    }

    ///<summary>
    ///Writes asynchronously a sequence of bytes to the current stream and advances the current position within this stream by the number of bytes written.
    ///</summary>
    ///<returns>A Task that represents the asynchronous write.</returns>
    ///<param name="source">The source.</param>
    ///<param name="buffer">The buffer containing data to write to the current stream.</param>
    ///<param name="offset">The zero-based byte offset in  at which to begin copying bytes to the current stream.</param>
    ///<param name="count">The maximum number of bytes to write. </param>
    /// <param name="cancellationToken">The cancellation token.</param>
    ///<exception cref="T:System.ArgumentException"> length minus <paramref name="offset" /> is less than <paramref name="count" />. </exception>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException"> or <paramref name="count" /> is negative. </exception>
    ///<exception cref="T:System.NotSupportedException">The stream does not support writing. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The stream is closed. </exception>
    ///<exception cref="T:System.IO.IOException">An I/O error occurred. </exception>
    public static Task WriteAsync(this System.IO.Stream source, byte[] buffer, int offset, int count, System.Threading.CancellationToken cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested)
            return TaskServices.FromCancellation(cancellationToken);
        return Task.Factory.FromAsync(source.BeginWrite, source.EndWrite, buffer, offset, count, null);
    }

    ///<summary>
    ///Flushes asynchronously the current stream.
    ///</summary>
    ///<returns>A Task that represents the asynchronous flush.</returns>
    public static Task FlushAsync(this System.IO.Stream source)
    {
        return FlushAsync(source, System.Threading.CancellationToken.None);
    }

    ///<summary>
    ///Flushes asynchronously the current stream.
    ///</summary>
    ///<returns>A Task that represents the asynchronous flush.</returns>
    public static Task FlushAsync(this System.IO.Stream source, System.Threading.CancellationToken cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested)
            return TaskServices.FromCancellation(cancellationToken);
        return Task.Factory.StartNew(s => ((System.IO.Stream)s).Flush(), source, cancellationToken, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>
    /// Reads all the bytes from the current stream and writes them to the destination stream.
    /// </summary>
    /// <param name="source">The source stream.</param>
    /// <param name="destination">The stream that will contain the contents of the current stream.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task CopyToAsync(this Stream source, Stream destination)
    {
        return CopyToAsync(source, destination, 0x1000);
    }

    /// <summary>
    /// Reads all the bytes from the current stream and writes them to the destination stream.
    /// </summary>
    /// <param name="source">The source stream.</param>
    /// <param name="destination">The stream that will contain the contents of the current stream.</param>
    /// <param name="bufferSize">The size of the buffer. This value must be greater than zero. The default size is 4096.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task CopyToAsync(this Stream source, Stream destination, int bufferSize)
    {
        return CopyToAsync(source, destination, bufferSize, CancellationToken.None);
    }

    /// <summary>
    /// Reads all the bytes from the current stream and writes them to the destination stream.
    /// </summary>
    /// <param name="source">The source stream.</param>
    /// <param name="destination">The stream that will contain the contents of the current stream.</param>
    /// <param name="bufferSize">The size of the buffer. This value must be greater than zero. The default size is 4096.</param>
    /// <param name="cancellationToken">The cancellation token to use to cancel the asynchronous operation.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task CopyToAsync(this Stream source, Stream destination, int bufferSize, CancellationToken cancellationToken)
    {
        if (source == null) throw new ArgumentNullException("source");
        if (destination == null) throw new ArgumentNullException("destination");
        if (bufferSize <= 0) throw new ArgumentOutOfRangeException("bufferSize");
        if (!source.CanRead && !source.CanWrite) throw new ObjectDisposedException("source");
        if (!destination.CanRead && !destination.CanWrite) throw new ObjectDisposedException("destination");
        if (!source.CanRead) throw new NotSupportedException();
        if (!destination.CanWrite) throw new NotSupportedException();

        return CopyToAsyncInternal(source, destination, bufferSize, cancellationToken);
    }

    /// <summary>
    /// Reads all the bytes from the current stream and writes them to the destination stream.
    /// </summary>
    /// <param name="source">The source stream.</param>
    /// <param name="destination">The stream that will contain the contents of the current stream.</param>
    /// <param name="bufferSize">The size of the buffer. This value must be greater than zero. The default size is 4096.</param>
    /// <param name="cancellationToken">The cancellation token to use to cancel the asynchronous operation.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    private static async Task CopyToAsyncInternal(this Stream source, Stream destination, int bufferSize, CancellationToken cancellationToken)
    {
        byte[] buffer = new byte[bufferSize];
        int bytesRead;
        while ((bytesRead = await source.ReadAsync(buffer, 0, buffer.Length, cancellationToken).ConfigureAwait(false)) > 0)
        {
            await destination.WriteAsync(buffer, 0, bytesRead, cancellationToken).ConfigureAwait(false);
        }
    }

    /// <summary>
    /// Reads a maximum of count characters from the reader asynchronously and writes
    /// the data to buffer, beginning at index.
    /// </summary>
    /// <param name="buffer">
    /// When the operation completes, contains the specified character array with the
    /// values between index and (index + count - 1) replaced by the characters read
    /// from the current source.
    /// </param>
    /// <param name="count">
    /// The maximum number of characters to read. If the end of the stream is reached
    /// before count of characters is read into buffer, the current method returns.
    /// </param>
    /// <param name="index">The place in buffer at which to begin writing.</param>
    /// <param name="source">the source reader.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task<int> ReadAsync(this TextReader source, char[] buffer, int index, int count)
    {
        if (source == null) throw new ArgumentNullException("source");
        return Task.Factory.StartNew(() => source.Read(buffer, index, count), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>
    /// Reads asynchronously a maximum of count characters from the current stream, and writes the
    /// data to buffer, beginning at index.
    /// </summary>
    /// <param name="source">The source reader.</param>
    /// <param name="buffer">
    /// When this method returns, this parameter contains the specified character
    /// array with the values between index and (index + count -1) replaced by the
    /// characters read from the current source.
    /// </param>
    /// <param name="index">The position in buffer at which to begin writing.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task<int> ReadBlockAsync(this TextReader source, char[] buffer, int index, int count)
    {
        if (source == null) throw new ArgumentNullException("source");
        return Task.Factory.StartNew(() => source.ReadBlock(buffer, index, count), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>
    /// Reads a line of characters from the reader and returns the string asynchronously.
    /// </summary>
    /// <param name="source">the source reader.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task<string> ReadLineAsync(this TextReader source)
    {
        if (source == null) throw new ArgumentNullException("source");
        return Task.Factory.StartNew(() => source.ReadLine(), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>
    /// Reads all characters from the current position to the end of the TextReader
    /// and returns them as one string asynchronously.
    /// </summary>
    /// <param name="source">the source reader.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task<string> ReadToEndAsync(this TextReader source)
    {
        if (source == null) throw new ArgumentNullException("source");
        return Task.Factory.StartNew(() => source.ReadToEnd(), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a string asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="value">The string to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteAsync(this TextWriter target, string value)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.Write(value), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a char asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="value">The char to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteAsync(this TextWriter target, char value)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.Write(value), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a char array asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="buffer">The buffer to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteAsync(this TextWriter target, char[] buffer)
    {
        return WriteAsync(target, buffer, 0, buffer.Length);
    }

    /// <summary>Writes a subarray of characters asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="buffer">The buffer to write.</param>
    /// <param name="index">Starting index in the buffer.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteAsync(this TextWriter target, char[] buffer, int index, int count)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.Write(buffer, index, count), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a line terminator asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteLineAsync(this TextWriter target)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.WriteLine(), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a string followed by a line terminator asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="value">The string to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteLineAsync(this TextWriter target, string value)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.WriteLine(value), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a char followed by a line terminator asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="value">The char to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteLineAsync(this TextWriter target, char value)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.WriteLine(value), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>Writes a char array followed by a line terminator asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="buffer">The buffer to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteLineAsync(this TextWriter target, char[] buffer)
    {
        return WriteLineAsync(target, buffer, 0, buffer.Length);
    }

    /// <summary>Writes a subarray of characters followed by a line terminator asynchronously to a text stream.</summary>
    /// <param name="target">The writer.</param>
    /// <param name="buffer">The buffer to write.</param>
    /// <param name="index">Starting index in the buffer.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>A Task representing the asynchronous write.</returns>
    public static Task WriteLineAsync(this TextWriter target, char[] buffer, int index, int count)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.WriteLine(buffer, index, count), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    /// <summary>
    /// Clears all buffers for the current writer and causes any buffered data to
    /// be written to the underlying device.
    /// </summary>
    /// <param name="target">The writer.</param>
    /// <returns>A Task representing the asynchronous flush.</returns>
    public static Task FlushAsync(this TextWriter target)
    {
        if (target == null) throw new ArgumentNullException("target");
        return Task.Factory.StartNew(() => target.Flush(), CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default);
    }

    ///<summary>Starts an asynchronous request for a web resource.</summary>
    ///<returns>Task that represents the asynchronous request.</returns>
    ///<exception cref="T:System.InvalidOperationException">The stream is already in use by a previous call to . </exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task<System.Net.WebResponse> GetResponseAsync(this System.Net.WebRequest source)
    {
        return Task.Factory.StartNew(() =>
            Task<System.Net.WebResponse>.Factory.FromAsync(source.BeginGetResponse, source.EndGetResponse, null),
            CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default).Unwrap();
    }

    ///<summary>Starts an asynchronous request for a  object to use to write data.</summary>
    ///<returns>Task that represents the asynchronous request.</returns>
    ///<exception cref="T:System.Net.ProtocolViolationException">The  property is GET and the application writes to the stream. </exception>
    ///<exception cref="T:System.InvalidOperationException">The stream is being used by a previous call to . </exception>
    ///<exception cref="T:System.ApplicationException">No write stream is available. </exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task<System.IO.Stream> GetRequestStreamAsync(this System.Net.WebRequest source)
    {
        return Task.Factory.StartNew(() =>
            Task<System.IO.Stream>.Factory.FromAsync(source.BeginGetRequestStream, source.EndGetRequestStream, null),
            CancellationToken.None, TaskCreationOptions.None, TaskScheduler.Default).Unwrap();
    }
}
