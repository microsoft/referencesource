//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2005
//
//  File:           InterleavedZipPartStream.cs
//
//  Description:    The class InterleavedZipPartStream is used to wrap one or more Zip
//                  part streams for an interleaved part. It hides the interleaving
//                  from its callers by offering the abstraction of a continuous stream
//                  across pieces.
//
//  History:        05/15/05 - johnlarc - initial implementation
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.IO;
using System.IO.Packaging;                  // For ZipPackagePart, etc.
using MS.Internal.IO.Zip;                   // For ZipFileInfo.
using System.Windows;                       // for ExceptionStringTable
using System.Collections.Generic;           // For List<>
using MS.Internal;                          // for Invariant
using MS.Internal.WindowsBase;

namespace MS.Internal.IO.Packaging
{
    /// <summary>
	/// The class InterleavedZipPartStream is used to wrap one or more Zip part streams
    /// for an interleaved part. It hides the interleaving from its callers by offering
    /// the abstraction of a continuous stream across pieces.
    /// </summary>
    /// <remarks>
    /// This class is defined for the benefit of ZipPackage, ZipPackagePart and
    /// InternalRelationshipCollection.
    /// Although it is quite specialized, it would hardly make sense to nest its definition in any
    /// of these clases.
    /// </remarks>
    internal partial class InterleavedZipPartStream : Stream
    {
        #region Constructors

        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------
        /// <summary>
        /// Build a System.IO.Stream on a part that possibly consists of multiple files
        /// An InterleavedZipPartStream gets created by ZipPackagePart.GetStreamCore when the part
        /// is interleaved. It wraps one or more Zip streams (one per piece).
        /// (pieces).
        /// </summary>
        /// <param name="mode">Mode (create, etc.) in which piece streams should be opened</param>
        /// <param name="access">Access (read, write, etc.) with which piece streams should be opened</param>
        /// <param name="owningPart">
        /// The part to build a stream on. It contains all ZipFileInfo descriptors for the part's pieces
        /// (see ZipPackage.GetPartsCore).
        /// </param>
        internal InterleavedZipPartStream(ZipPackagePart owningPart, FileMode mode, FileAccess access)
            : this(PackUriHelper.GetStringForPartUri(owningPart.Uri), 
                owningPart.PieceDescriptors,
                mode, access)
        {
        }

        /// <summary>
        /// This constructor is provided to be able to interleave other files than just parts,
        /// notably the contents type file.
        /// </summary>
        internal InterleavedZipPartStream(string partName, List<PieceInfo> sortedPieceInfoList,
            FileMode mode, FileAccess access)
        {
            // The PieceDirectory mediates access to pieces.
            // It maps offsets to piece numbers and piece numbers to streams and start offsets.
            // Mode and access are entirely managed by the underlying streams, assumed to be seekable.
            _dir = new PieceDirectory(sortedPieceInfoList, mode, access);

            // GetCurrentPieceNumber is operational from the beginning.
            Invariant.Assert(_dir.GetStartOffset(GetCurrentPieceNumber()) == 0);
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        /// <summary>
        /// Return the bytes requested.
        /// </summary>
        /// <param name="buffer">Destination buffer.</param>
        /// <param name="offset">
        /// The zero-based byte offset in buffer at which to begin storing the data read
        /// from the current stream.
        /// </param>
        /// <param name="count">How many bytes requested.</param>
        /// <returns>How many bytes were written into buffer.</returns>
        public override int Read(byte[] buffer, int offset, int count)
        {
            CheckClosed();

            // Check arguments.
            PackagingUtilities.VerifyStreamReadArgs(this, buffer, offset, count);

            // Leave capability and FileAccess checks up to the underlying stream(s).

            // Reading 0 bytes is a no-op.
            if (count == 0)
                return 0;

            int pieceNumber = GetCurrentPieceNumber();
            int totalBytesRead = 0;

            Stream pieceStream = _dir.GetStream(pieceNumber);

            checked
            {
                //Seek to the correct location in the underlying stream for the current piece
                pieceStream.Seek(_currentOffset - _dir.GetStartOffset(pieceNumber), SeekOrigin.Begin);

                while (totalBytesRead < count)
                {
                    int numBytesRead = pieceStream.Read(
                        buffer,
                        offset + totalBytesRead,
                        count - totalBytesRead);

                    // End of the current stream: try to move to the next stream.
                    if (numBytesRead == 0)
                    {
                        if (_dir.IsLastPiece(pieceNumber))
                            break;

                        ++pieceNumber;
                        Invariant.Assert(_dir.GetStartOffset(pieceNumber) == _currentOffset + totalBytesRead);

                        pieceStream = _dir.GetStream(pieceNumber);

                        //Seek inorder to set the correct pointer for the next piece stream
                        pieceStream.Seek(0, SeekOrigin.Begin);
                    }

                    totalBytesRead += numBytesRead;
                }

                // Advance current position now we know the operation completed successfully.
                _currentOffset += totalBytesRead;
            }

            return totalBytesRead;
        }

        /// <summary>
        /// Seek
        /// </summary>
        /// <param name="offset">Offset in byte.</param>
        /// <param name="origin">Offset origin (start, current, or end).</param>
        public override long Seek(long offset, SeekOrigin origin)
        {
            CheckClosed();

            // Check stream capabilities. (Normally, CanSeek will be false only
            // when the stream is closed.)
            if (!CanSeek)
                throw new NotSupportedException(SR.Get(SRID.SeekNotSupported));

            // Convert offset to a start-based offset.
            switch (origin)
            {
                case SeekOrigin.Begin:
                    break;

                case SeekOrigin.Current:
                    checked { offset += _currentOffset; }
                    break;

                case SeekOrigin.End:
                    checked { offset += Length; }
                    break;

                default:
                    throw new ArgumentOutOfRangeException("origin");
            }

            // Check offset validity.
            if (offset < 0)
                throw new ArgumentException(SR.Get(SRID.SeekNegative));
                
            // OK if _currentOffset points beyond end of stream.

            // Update position field and return.
            _currentOffset = offset;

            return _currentOffset;
        }

        /// <summary>
        /// SetLength
        /// </summary>
        public override void SetLength(long newLength)
        {
            CheckClosed();

            // Check argument and stream capabilities.
            if (newLength < 0)
                throw new ArgumentOutOfRangeException("newLength");
            if (!CanWrite)
                throw new NotSupportedException(SR.Get(SRID.StreamDoesNotSupportWrite));
            if (!CanSeek)
                throw new NotSupportedException(SR.Get(SRID.SeekNotSupported));

            // If some pieces are to be deleted, this is reflected only in memory at present.
            int lastPieceNumber;
            if (newLength == 0)
            {
                // This is special-cased because there is no last offset to speak of, and
                // so the piece directory cannot return any piece by offset.
                lastPieceNumber = 0;
            }
            else
            {
                lastPieceNumber = _dir.GetPieceNumberFromOffset(newLength - 1); // No need to use checked{] since newLength != 0
            }
            _dir.SetLogicalLastPiece(lastPieceNumber); 

            // Adjust last active stream to new size.
            Stream lastPieceStream = _dir.GetStream(lastPieceNumber);

            Debug.Assert(newLength - _dir.GetStartOffset(lastPieceNumber) >= 0);
            long lastPieceStreamSize = newLength - _dir.GetStartOffset(lastPieceNumber);
            lastPieceStream.SetLength(lastPieceStreamSize);

            if (_currentOffset > newLength)
            {
                _currentOffset = newLength;
            }
        }

        /// <summary>
        /// Write. Distribute the bytes to write across several contiguous streams if needed.
        /// </summary>
        /// <remarks>
        /// Zip streams can be assumed seekable so the length will be available for chaining
        /// pieces.
        /// </remarks>
        public override void Write(byte[] buffer, int offset, int count)
        {
            CheckClosed();

            // Check arguments.
            PackagingUtilities.VerifyStreamWriteArgs(this, buffer, offset, count);

            // No check for FileAccess and stream capability (CanWrite). This is the responsibility
            // of the underlying stream(s). 

            // A no-op if zero bytes to write.
            if (count == 0)
                return;
            
            // Write into piece streams, preserving all lengths in non-terminal pieces.
            int totalBytesWritten = 0;
            int pieceNumber = GetCurrentPieceNumber();
            Stream pieceStream = _dir.GetStream(pieceNumber);

            checked
            {
                //Seek to the correct location in the underlying stream for the current piece
                pieceStream.Seek(_currentOffset - _dir.GetStartOffset(pieceNumber), SeekOrigin.Begin);

                while (totalBytesWritten < count)
                {
                    // Compute the number of bytes to write into pieceStream.
                    int numBytesToWriteInCurrentPiece = count - totalBytesWritten;
                    if (!_dir.IsLastPiece(pieceNumber))
                    {
                        // The write should not change the length of an intermediate piece.
                        long currentPosition = _currentOffset + totalBytesWritten;
                        long maxPosition = _dir.GetStartOffset(pieceNumber+1) - 1;
                        if (numBytesToWriteInCurrentPiece > (maxPosition - currentPosition + 1))
                        {
                            // Cast from long to cast is safe in so far as *count*, which is the
                            // absolute max for all byte counts, is a positive int.
                            numBytesToWriteInCurrentPiece = checked((int)(maxPosition - currentPosition + 1));
                        }
                    }

                    // Do the write.
                    pieceStream.Write(buffer, offset + totalBytesWritten, numBytesToWriteInCurrentPiece);

                    // Update the tally.
                    totalBytesWritten += numBytesToWriteInCurrentPiece;

                    // If there is more data to write, get the next piece stream
                    if (!_dir.IsLastPiece(pieceNumber) && totalBytesWritten < count)
                    {
                        // The next write, should involve the next piece.
                        ++pieceNumber;

                        pieceStream = _dir.GetStream(pieceNumber);

                        //Seek inorder to set the correct pointer for the next piece stream
                        pieceStream.Seek(0, SeekOrigin.Begin);
                    }
                }

                // Now we know the operation has completed, the current position can be updated.
                Invariant.Assert(totalBytesWritten == count);
                _currentOffset += totalBytesWritten;
            }
        }

        /// <summary>
        /// Flush all dirty streams and commit pending piece deletions.
        /// </summary>
        /// <remarks>
        /// Flush gets called on all underlying streams ever accessed. If it turned out
        /// this is too inefficient, the PieceDirectory could be made to expose a SetDirty
        /// method that takes a piece number.
        /// </remarks>
        public override void Flush()
        {
            CheckClosed();

            // The underlying streams know whether they are dirty or not;
            // so _dir will indiscriminately flush all the streams that have been accessed.
            // It will also carry out necessary renamings and deletions to reflect calls to
            // SetLogicalLastPiece.
            _dir.Flush();
        }

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        /// <summary>
        /// Is stream readable?
        /// </summary>
        /// <remarks>
        /// <para>
        /// Here, the assumption, as in all capability tests, is that the status of
        /// the first piece reflects the status of all pieces for the part.
        /// This is justified by the fact that (i) all piece streams are opened with the same
        /// parameters against the same archive and (ii) the current piece stream cannot get
        /// closed unless the whole part stream is closed.
        /// </para>
        /// <para>
        /// A further assumption is that, as soon as interleaved zip part stream is initialized, there
        /// is a descriptor for the 1st piece.
        /// </para>
        /// </remarks>
        public override bool CanRead
        {
            get
            {
                return _closed ? false : _dir.GetStream(0).CanRead;
            }
        }

        /// <summary>
        /// Is stream seekable?
        /// </summary>
        /// <remarks>
        /// <para>
        /// Here, the assumption, as in all capability tests, is that the status of
        /// the first piece reflects the status of all pieces for the part.
        /// This is justified by the fact that (i) all piece streams are opened with the same
        /// parameters against the same archive and (ii) the current piece stream cannot get
        /// closed unless the whole part stream is closed.
        /// </para>
        /// <para>
        /// A further assumption is that, as soon as interleaved zip part stream is initialized, there
        /// is a descriptor for the 1st piece.
        /// </para>
        /// </remarks>
        public override bool CanSeek
        {
            get
            {
                return _closed ? false : _dir.GetStream(0).CanSeek;
            }
        }

        /// <summary>
        /// Is stream writable?
        /// </summary>
        /// <remarks>
        /// <para>
        /// Here, the assumption, as in all capability tests, is that the status of
        /// the first piece reflects the status of all pieces for the part.
        /// This is justified by the fact that (i) all piece streams are opened with the same
        /// parameters against the same archive and (ii) the current piece stream cannot get
        /// closed unless the whole part stream is closed.
        /// </para>
        /// <para>
        /// A further assumption is that, as soon as interleaved zip part stream is initialized, there
        /// is a descriptor for the 1st piece.
        /// </para>
        /// </remarks>
        // 
        public override bool CanWrite
        {
            get
            {
                return _closed ? false : _dir.GetStream(0).CanWrite;
            }
        }

        /// <summary>
        /// Logical byte position in this stream.
        /// </summary>
        public override long Position
        {
            get
            {
                CheckClosed();

                // Current offset is systematically updated to reflect the current position.
                return _currentOffset;
            }
            set
            {
                CheckClosed();
                Seek(value, SeekOrigin.Begin);
            }
        }

        /// <summary>
        /// Length.
        /// </summary>
        // 
        public override long Length
        {
            get
            {
                CheckClosed();
                Invariant.Assert(CanSeek);

                long length = 0;
                for (int pieceNumber = 0; pieceNumber < _dir.GetNumberOfPieces(); ++pieceNumber)
                {
                    checked { length += _dir.GetStream(pieceNumber).Length; }
                }
                return length;
            }
        }

        //------------------------------------------------------
        //
        //  Protected Methods
        //
        //------------------------------------------------------

        #region Protected Methods

        /// <summary>
        /// Dispose(bool)
        /// </summary>
        /// <param name="disposing"></param>
        /// <remarks>
        /// An instance of streams' peculiar dispose pattern, whereby
        /// the inherited abstract class implements Close by calling
        /// this virtual protected function.
        /// In turn, each implementation is responsible for calling back
        /// its base's implementation.
        /// </remarks>
        protected override void Dispose(bool disposing)
        {
            

            try
            {
                if (disposing)
                {
                    if (!_closed)
                    {
                        _dir.Close();                      
                    }
                }
            }
            finally
            {
                _closed = true;
                base.Dispose(disposing);
            }
        }

        #endregion Protected Methods

        //------------------------------------------------------
        //
        //   Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private void CheckClosed()
        {
            if (_closed)
                throw new ObjectDisposedException(null, SR.Get(SRID.StreamObjectDisposed));
        }

        /// <summary>
        /// Infer the current piece number from _currentOffset.
        /// </summary>
        /// <remarks>
        /// Storing the current piece number in a field and computing the current offset from it
        /// would also have been possible, but less efficient.
        /// </remarks>
        private int GetCurrentPieceNumber()
        {
            // Since this property is likely to be read more often than _currentOffset
            // gets updated, its value is cached in _currentPieceNumber.
            // The validity of the cached value is monitored using _offsetForCurrentPieceNumber.
            if (_offsetForCurrentPieceNumber != _currentOffset)
            {
                // Cached value is stale. Refresh.
                _currentPieceNumber = _dir.GetPieceNumberFromOffset(_currentOffset);
                _offsetForCurrentPieceNumber = _currentOffset;
            }
            return _currentPieceNumber;
        }

        #endregion Private Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        // High-level object to access the collection of pieces by offset and pieceNumber.
        private PieceDirectory       _dir;

        // Cached value for the current piece number.
        // (Lazily sync'ed to _currentOffset when GetCurrentPieceNumber() is invoked.)
        private int                 _currentPieceNumber;

        // Control value to decide whether to use _currentPieceNumber without updating it.
        private Nullable<long>      _offsetForCurrentPieceNumber;

        // This variable continuously tracks the current stream position.
        private long                _currentOffset = 0;

        // Closed status.
        private bool                _closed = false;

        #endregion Private Fields
    }
}

