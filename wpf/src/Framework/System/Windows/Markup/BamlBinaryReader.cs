/***************************************************************************\
*
* File: BamlBinaryReader.cs
*
* Purpose:  Subclass BinaryReader.
*
* Copyright (C) 2006 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

using System;
using System.IO;
using System.Text;

namespace System.Windows.Markup
{

    internal class BamlBinaryReader: BinaryReader
    {
        public BamlBinaryReader(Stream stream, Encoding code)
            :base(stream, code)
        {
        }

        public new int Read7BitEncodedInt()
        {
            return base.Read7BitEncodedInt();
        }
    }
}
