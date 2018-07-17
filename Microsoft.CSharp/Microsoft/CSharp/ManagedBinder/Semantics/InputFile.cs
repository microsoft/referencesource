// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Collections.Generic;
using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // InputFile
    //
    // InputFile - a symbol that represents an input file, either source
    // code or meta-data, of a file we may read. Its parent is the output
    // file it contributes to. MetaData files have no parent.
    // This should be split in to two classes, one for metadata files 
    // and another for source files.
    // ----------------------------------------------------------------------------

    class InputFile : FileRecord
    {
        // Which aliases this INFILE is in. For source INFILESYMs, only bits kaidThisAssembly and kaidGlobal
        // should be set.
        private HashSet<KAID> bsetFilter;
        private KAID aid;
        //#if DEBUG
        //        private bool fUnionCalled;
        //#endif

        public bool isSource;               // If true, source code, if false, metadata
        // and on the module of added .netmodules

        public InputFile()
        {
            bsetFilter = new HashSet<KAID>();
        }

        public void SetAssemblyID(KAID aid)
        {
            Debug.Assert(this.aid == default(KAID));
            Debug.Assert(KAID.kaidThisAssembly <= aid && aid < KAID.kaidMinModule);

            this.aid = aid;
            bsetFilter.Add(aid);
            if (aid == KAID.kaidThisAssembly)
                bsetFilter.Add(KAID.kaidGlobal);
        }

        public void AddToAlias(KAID aid)
        {
            Debug.Assert(0 <= aid && aid < KAID.kaidMinModule);

            // NOTE: Anything in this assembly should not be added to other aliases!
            Debug.Assert(this.aid > KAID.kaidThisAssembly);
            Debug.Assert(bsetFilter.Contains(this.aid));

            bsetFilter.Add(aid);
        }

        public void UnionAliasFilter(ref HashSet<KAID> bsetDst)
        {
            bsetDst.UnionWith(bsetFilter);
            //        #if DEBUG
            //            fUnionCalled = true;
            //        #endif
        }

        public KAID GetAssemblyID()
        {
            Debug.Assert(aid >= KAID.kaidThisAssembly);
            return aid;
        }

        public bool InAlias(KAID aid)
        {
            Debug.Assert(0 <= aid);
            return bsetFilter.Contains(aid);
        }
    }
}