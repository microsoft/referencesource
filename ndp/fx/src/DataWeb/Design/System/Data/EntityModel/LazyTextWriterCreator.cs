//---------------------------------------------------------------------
// <copyright file="LazyTextWriterCreator.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------
using System.Diagnostics;
using System.IO;

namespace System.Data.Services.Design
{
    /// <summary>
    /// This class is responsible for abstracting the knowledge
    /// of whether the user provided a TextWriter or a FilePath.
    /// 
    /// If the user gave us a filePath we try not to create the TextWriter
    /// till we absolutely need it in order to prevent the file from being created
    /// in error cases.
    /// </summary>
    internal class LazyTextWriterCreator : IDisposable
    {
        private bool _ownTextWriter;
        private TextWriter _writer;
        private string _targetFilePath;

        internal LazyTextWriterCreator(string targetFilePath)
        {
            Debug.Assert(targetFilePath != null, "targetFilePath parameter is null");

            _ownTextWriter = true;
            _targetFilePath = targetFilePath;
        }

        internal LazyTextWriterCreator(TextWriter writer)
        {
            _writer = writer;
        }

        internal TextWriter GetOrCreateTextWriter()
        {
            if (_writer == null)
            {
                // lazy creating the writer
                _writer = new StreamWriter(_targetFilePath);
            }
            return _writer;
        }

        internal string TargetFilePath
        {
            get { return _targetFilePath; }
        }

        public void Dispose()
        {
            if (_ownTextWriter && _writer != null)
            {
                _writer.Dispose();
            }

            GC.SuppressFinalize(this);
        }
    }
}
