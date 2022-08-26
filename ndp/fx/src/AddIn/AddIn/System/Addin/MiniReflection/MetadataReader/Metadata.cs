using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.IO;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection.MetadataReader
{
    [Serializable]
    internal struct MetadataToken
    {
        public MDTables.Tables Table;
        public UInt32 Index;   // 1 to N, not 0 to N-1.

        public MetadataToken(MDTables.Tables table, UInt32 index)
        {
            System.Diagnostics.Contracts.Contract.Requires((index & 0xFF000000U) == 0);

            Table = table;
            Index = index;
        }

        public override String ToString()
        {
            return String.Format(CultureInfo.InvariantCulture, "Table {0} ({1}), entry {2}", MDTables.Names[(Int32)Table], (UInt32)Table, Index);
        }

        public String ToMDToken()
        {
            return String.Format(CultureInfo.InvariantCulture, "{0:x2}{1:x6}", (UInt32)Table, Index);
        }
    }


    internal sealed class MDTables
    {
        internal static readonly String[] Names = new String[(Int32)Tables.MaxTable + 1];
        internal static readonly Boolean[] IsDefined = new Boolean[(Int32)Tables.MaxTable + 1];
        private static readonly UTF8Encoding Encoder = new UTF8Encoding(false, true);

        // Per-Instance data
        internal BinaryReader B;
        private StreamDesc stringStream, blobStream;

        private UInt32[] lengths;   // Indexed by table, gives length of row in bytes
        private UInt32[] tableAt;   // Indexed by table, gives location in file
        private UInt32[] NRows;     // Indexed by table, gives number of rows
        private UInt32 stringIndex, blobIndex, GUIDIndex; // Bytes required to reference these items

        
        internal enum Tables
        {
            Invalid = -1,
            XModule = 0,
            TypeRef = 1,
            TypeDef = 2,
            FieldPtr = 3,		// Not public
            FieldDef = 4,
            MethodPtr  = 5,		// Not public
            MethodDef = 6,
            ParamPtr = 7,		// Not public
            ParamDef = 8,
            InterfaceImpl = 9,
            MemberRef = 10,
            Constant = 11,
            CustomAttribute = 12,
            FieldMarshal = 13,
            DeclSecurity = 14,
            ClassLayout = 15,
            FieldLayout = 16,
            StandAloneSig = 17,
            EventMap = 18,
            EventPtr = 19,		// Not public
            XEvent = 20,
            PropertyMap = 21,
            PropertyPtr = 22,	// Not public
            XProperty = 23,
            MethodSemantics = 24,
            MethodImpl = 25,
            ModuleRef = 26,
            TypeSpec = 27,
            ImplMap = 28,
            FieldRVA = 29,
            // Unused 0x1E = 30 Edit&Continue Log
            // Unused 0x1F = 31 Edit&Continue Map
            XAssembly = 32,
            AssemblyProcessor = 33,
            AssemblyOS = 34,
            AssemblyRef = 35,
            AssemblyRefProcessor = 36,
            AssemblyRefOS = 37,
            File = 38,
            ExportedType = 39,
            ManifestResource = 40,
            NestedClass = 41,
            GenericParam = 42,
            GenericMethod = 43,
            GenericConstraint = 44,
            //From 45 through 63, inclusive, are not used
            MaxTable = 63
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1810:InitializeReferenceTypeStaticFieldsInline", Justification="Not possible")]
        static MDTables()
        {
            Names[(Int32)Tables.AssemblyOS] = "AssemblyOS";
            Names[(Int32)Tables.AssemblyProcessor] = "AssemblyProcessor";
            Names[(Int32)Tables.AssemblyRef] = "AssemblyRef";
            Names[(Int32)Tables.AssemblyRefOS] = "AssemblyRefOS";
            Names[(Int32)Tables.AssemblyRefProcessor] = "AssemblyRefProcessor";
            Names[(Int32)Tables.ClassLayout] = "ClassLayout";
            Names[(Int32)Tables.Constant] = "Constant";
            Names[(Int32)Tables.CustomAttribute] = "CustomAttribute";
            Names[(Int32)Tables.DeclSecurity] = "DeclSecurity";
            Names[(Int32)Tables.EventMap] = "EventMap";
            Names[(Int32)Tables.ExportedType] = "ExportedType";
            Names[(Int32)Tables.FieldDef] = "FieldDef";
            Names[(Int32)Tables.FieldLayout] = "FieldLayout";
            Names[(Int32)Tables.FieldMarshal] = "FieldMarshal";
            Names[(Int32)Tables.FieldRVA] = "FieldRVA";
            Names[(Int32)Tables.File] = "File";
            Names[(Int32)Tables.GenericParam] = "GenericParam";
            Names[(Int32)Tables.GenericMethod] = "GenericMethod";
            Names[(Int32)Tables.GenericConstraint] = "GenericConstraint";
            Names[(Int32)Tables.ImplMap] = "ImplMap";
            Names[(Int32)Tables.InterfaceImpl] = "InterfaceImpl";
            Names[(Int32)Tables.ManifestResource] = "ManifestResource";
            Names[(Int32)Tables.MemberRef] = "MemberRef";
            Names[(Int32)Tables.MethodDef] = "MethodDef";
            Names[(Int32)Tables.MethodImpl] = "MethodImpl";
            Names[(Int32)Tables.MethodSemantics] = "MethodSemantics";
            Names[(Int32)Tables.ModuleRef] = "ModuleRef";
            Names[(Int32)Tables.NestedClass] = "NestedClass";
            Names[(Int32)Tables.ParamDef] = "ParamDef";
            Names[(Int32)Tables.PropertyMap] = "PropertyMap";
            Names[(Int32)Tables.StandAloneSig] = "StandAloneSig";
            Names[(Int32)Tables.TypeDef] = "TypeDef";
            Names[(Int32)Tables.TypeRef] = "TypeRef";
            Names[(Int32)Tables.TypeSpec] = "TypeSpec";
            Names[(Int32)Tables.XAssembly] = "Assembly";
            Names[(Int32)Tables.XEvent] = "Event";
            Names[(Int32)Tables.XModule] = "Module";
            Names[(Int32)Tables.XProperty] = "Property";
// Not public
            Names[(Int32)Tables.FieldPtr] = "FieldPointer";
            Names[(Int32)Tables.MethodPtr] = "MethodPointer";
            Names[(Int32)Tables.ParamPtr] = "ParamPointer";
            Names[(Int32)Tables.EventPtr] = "EventPointer";
            Names[(Int32)Tables.PropertyPtr] = "PropertyPointer";
		    

            for (Int32 i = 0; i <= (Int32)Tables.MaxTable; i++)
            {
                IsDefined[i] = Names[i] != null;
                if (!IsDefined[i])
                    Names[i] = String.Format(CultureInfo.InvariantCulture, "<<{0}>>", i);
            }
        }

        internal enum Encodings
        {
            TypeDefOrRef, HasConstant, HasCustomAttribute, HasFieldMarshall,
            HasDeclSecurity, MemberRefParent, HasSemantics, MethodDefOrRef,
            MemberForwarded, Implementation, CustomAttributeType,
            ResolutionScope, TypeOrMethodDef,
            //Max = TypeOrMethodDef
        }

        private struct EncodingDesc
        {
            String Name;
            internal int TagBits;
            internal Tables[] WhichTables;

            internal EncodingDesc(String N, int T, Tables[] W)
            {
                Name = N;
                TagBits = T;
                WhichTables = W;
            }
            /*
            public override String ToString()
            {
                return String.Format("Encoding {0} has {1} tag bits for {2} tables", 
                    Name, TagBits, WhichTables.Length);
            }
             */
        }

        private static readonly EncodingDesc[] EncodingDescs = new EncodingDesc[]
            // These are ordered to match their definitions in MD\Runtime\Metamodel.h
            {
                new EncodingDesc("TypeDefOrRef", 2, 
                  new Tables[] { Tables.TypeDef, Tables.TypeRef, Tables.TypeSpec }),
                new EncodingDesc("HasConstant", 2, 
                  new Tables[] { Tables.FieldDef, Tables.ParamDef, Tables.XProperty }),
                new EncodingDesc("HasCustomAttribute", 5, 
                  new Tables[] { Tables.MethodDef, Tables.FieldDef, Tables.TypeRef, Tables.TypeDef, 
                      Tables.ParamDef, Tables.InterfaceImpl, Tables.MemberRef, Tables.XModule,
                      Tables.DeclSecurity, Tables.XProperty, Tables.XEvent, Tables.StandAloneSig,
                      Tables.ModuleRef, Tables.TypeSpec, Tables.XAssembly, Tables.AssemblyRef,
                      Tables.File, Tables.ExportedType, Tables.ManifestResource,
                      // These last three aren't documented, but are allowed
                      Tables.GenericParam, Tables.GenericConstraint, Tables.GenericParam}),
                new EncodingDesc("HasFieldMarshall", 1, 
                  new Tables[] { Tables.FieldDef, Tables.ParamDef }),
                new EncodingDesc("HasDeclSecurity", 2, 
                  new Tables[] { Tables.TypeDef, Tables.MethodDef, Tables.XAssembly }),
                new EncodingDesc("MemberRefParent", 3,
                  // TypeDef isn't documented, but it's allowed
                  new Tables[] { Tables.TypeDef, Tables.TypeRef, Tables.ModuleRef, Tables.MethodDef, Tables.TypeSpec }),
                new EncodingDesc("HasSemantics", 1, 
                  new Tables[] { Tables.XEvent, Tables.XProperty }),
                new EncodingDesc("MethodDefOrRef", 1, 
                  new Tables[] { Tables.MethodDef, Tables.MemberRef }),
                new EncodingDesc("MemberForwarded", 1, 
                  new Tables[] { Tables.FieldDef, Tables.MethodDef }),
                new EncodingDesc("Implementation", 2,
                  new Tables[] { Tables.File, Tables.AssemblyRef, Tables.ExportedType }),
                new EncodingDesc("CustomAttributeType", 3, 
                  new Tables[] { Tables.Invalid, Tables.Invalid, Tables.MethodDef, Tables.MemberRef }),
                new EncodingDesc("ResolutionScope", 2,
                  new Tables[] { Tables.XModule, Tables.ModuleRef, Tables.AssemblyRef, Tables.TypeRef }),
                new EncodingDesc("TypeOrMethodDef", 1,
                  new Tables[] { Tables.TypeDef, Tables.MethodDef })
            };

        internal MDTables(BinaryReader reader, StreamDesc stringSD, StreamDesc blobSD)
        { // Positioned at start of #~ stream
            stringStream = stringSD;
            blobStream = blobSD;
            B = reader;
            B.BaseStream.Seek(4, SeekOrigin.Current);   // Skip reserved
            byte Major = B.ReadByte();
            byte Minor = B.ReadByte();
            // Console.WriteLine("Table schema version {0}.{1}", Major, Minor);
            byte HeapSizes = B.ReadByte();
            stringIndex = ((HeapSizes & 0x01) == 0) ? 2U : 4U;
            GUIDIndex = ((HeapSizes & 0x02) == 0) ? 2U : 4U;
            blobIndex = ((HeapSizes & 0x04) == 0) ? 2U : 4U;
            B.ReadByte();   // Reserved
            UInt64 Valid = B.ReadUInt64();
            UInt64 Sorted = B.ReadUInt64();
            NRows = new UInt32[(Int32)Tables.MaxTable + 1];
            int NTables = 0;
            UInt64 VBit = 1UL;
            for (int Table = 0; Table <= (int)Tables.MaxTable; Table++, VBit <<= 1)
            {
                if ((Valid & VBit) != 0)
                {
                    NRows[Table] = B.ReadUInt32();
                    if (NRows[Table] == 0)
                    {
                        //Console.WriteLine("Table {0} is valid with 0 rows", Table);
                    }
                    // Console.WriteLine("Table {0} ({1}) has {2} rows", Names[Table], Table, NRows[Table]);
                    NTables += 1;
                }
            }
            // Console.WriteLine("Found {0} tables", NTables);
            ComputeRowLengths();
            /*
            for (int i = 0; i <= (int) Tables.MaxTable; i++)
                Console.WriteLine("Table {0} ({1}) has {2} bytes per row",
                    Names[i], i, lengths[i]); 
             */
            tableAt = new UInt32[(Int32)Tables.MaxTable + 1];
            VBit = 1UL;
            UInt32 Offset = (UInt32)B.BaseStream.Position;
            for (int Table = 0; Table <= (int)Tables.MaxTable; Table++, VBit <<= 1)
            {
                tableAt[Table] = Offset;
                // Console.WriteLine("Table {2} ({0}) at offset 0x{1:x}", Table, Offset, Names[(UInt32)Table]);
                Offset += lengths[Table] * NRows[Table];
                if (((Valid & VBit) != 0) && (NRows[Table] != 0) && (lengths[Table] == 0))
                    throw new BadImageFormatException(String.Format(CultureInfo.CurrentCulture, Res.UnknownMetadataTable, Table));
            }
        }

        UInt32 MetadataTokenSize(Encodings Which)
        {
            EncodingDesc E = EncodingDescs[(int) Which];
            int TagBits = E.TagBits;
            UInt32 MaxRows = 0;
            foreach (Tables Table in E.WhichTables)
            {
                if (Table == Tables.Invalid) continue;
                UInt32 N = NRows[(int)Table];
                if (N > MaxRows) MaxRows = N;
            }
            return (MaxRows < (1 << (16 - TagBits))) ? 2U : 4U;
        }

        UInt32 RowSize(Tables Table)
        {
            return (NRows[(int) Table] < (1 << 16)) ? 2U : 4U;
        }

        private void ComputeRowLengths()
        {
            lengths = new UInt32[(Int32)Tables.MaxTable + 1];
            lengths[(Int32)Tables.XAssembly] = 4 + 4 * 2 + 4 + blobIndex + 2 * stringIndex;
            lengths[(Int32)Tables.AssemblyOS] = 4 * 3;
            lengths[(Int32)Tables.AssemblyProcessor] = 4;
            lengths[(Int32)Tables.AssemblyRef] = 4 * 2 + 4 + 2 * blobIndex + 2 * stringIndex;
            lengths[(Int32)Tables.AssemblyRefOS] = 3 * 4 + RowSize(Tables.AssemblyRef);
            lengths[(Int32)Tables.AssemblyRefProcessor] = 4 + RowSize(Tables.AssemblyRef);
            lengths[(Int32)Tables.ClassLayout] = 2 + 4 + RowSize(Tables.TypeDef);
            lengths[(Int32)Tables.Constant] = 1 + 1 + MetadataTokenSize(Encodings.HasConstant) + blobIndex;
            lengths[(Int32)Tables.CustomAttribute] = MetadataTokenSize(Encodings.HasCustomAttribute) +
                MetadataTokenSize(Encodings.CustomAttributeType) + blobIndex;
            lengths[(Int32)Tables.DeclSecurity] = 2 + MetadataTokenSize(Encodings.HasDeclSecurity) + blobIndex;
            lengths[(Int32)Tables.EventMap] = RowSize(Tables.TypeDef) + RowSize(Tables.XEvent);
            lengths[(Int32)Tables.XEvent] = 2 + stringIndex + MetadataTokenSize(Encodings.TypeDefOrRef);
            lengths[(Int32)Tables.ExportedType] = 4 + 4 + stringIndex*2 + MetadataTokenSize(Encodings.Implementation);
            lengths[(Int32)Tables.FieldDef] = 2 + stringIndex + blobIndex;
            lengths[(Int32)Tables.FieldLayout] = 4 + RowSize(Tables.FieldDef);
            lengths[(Int32)Tables.FieldMarshal] = MetadataTokenSize(Encodings.HasFieldMarshall) + blobIndex;
            lengths[(Int32)Tables.FieldRVA] = 4 + RowSize(Tables.FieldDef);
            lengths[(Int32)Tables.File] = 4 + stringIndex + blobIndex;
            lengths[(Int32)Tables.GenericParam] = 2 + 2 + MetadataTokenSize(Encodings.TypeOrMethodDef) + stringIndex;
            lengths[(Int32)Tables.GenericMethod] = MetadataTokenSize(Encodings.MethodDefOrRef) + blobIndex;
            lengths[(Int32)Tables.GenericConstraint] = RowSize(Tables.GenericParam) + MetadataTokenSize(Encodings.TypeDefOrRef);
            lengths[(Int32)Tables.ImplMap] = 2 + MetadataTokenSize(Encodings.MemberForwarded) + stringIndex +
                RowSize(Tables.ModuleRef);
            lengths[(Int32)Tables.InterfaceImpl] = RowSize(Tables.TypeDef) + MetadataTokenSize(Encodings.TypeDefOrRef);
            lengths[(Int32)Tables.ManifestResource] = 4 + 4 + stringIndex + MetadataTokenSize(Encodings.Implementation);
            lengths[(Int32)Tables.MemberRef] = MetadataTokenSize(Encodings.MemberRefParent) + stringIndex +
                blobIndex;
            lengths[(Int32)Tables.MethodDef] = 4 + 2 + 2 + stringIndex + 
                blobIndex + RowSize(Tables.ParamDef);
            lengths[(Int32)Tables.MethodImpl] = RowSize(Tables.TypeDef) + MetadataTokenSize(Encodings.MethodDefOrRef) * 2;
            lengths[(Int32)Tables.MethodSemantics] = 2 + RowSize(Tables.MethodDef) + MetadataTokenSize(Encodings.HasSemantics);
            lengths[(Int32)Tables.XModule] = 2 + stringIndex + GUIDIndex * 3;
            lengths[(Int32)Tables.ModuleRef] = stringIndex;
            lengths[(Int32)Tables.NestedClass] = RowSize(Tables.TypeDef) * 2;
            lengths[(Int32)Tables.ParamDef] = 2 + 2 + stringIndex;
            lengths[(Int32)Tables.XProperty] = 2 + stringIndex + blobIndex;
            lengths[(Int32)Tables.PropertyMap] = RowSize(Tables.TypeDef) + RowSize(Tables.XProperty);
            lengths[(Int32)Tables.StandAloneSig] = blobIndex;
            lengths[(Int32)Tables.TypeDef] = 4 + stringIndex*2 + MetadataTokenSize(Encodings.TypeDefOrRef) +
                RowSize(Tables.FieldDef) + RowSize(Tables.MethodDef);
            lengths[(Int32)Tables.TypeRef] = MetadataTokenSize(Encodings.ResolutionScope) + stringIndex*2;
            lengths[(Int32)Tables.TypeSpec] = blobIndex;
// Non-public
		    lengths[(Int32)Tables.FieldPtr] = RowSize(Tables.FieldDef);
            lengths[(Int32)Tables.MethodPtr] = RowSize(Tables.MethodDef);
            lengths[(Int32)Tables.ParamPtr] = RowSize(Tables.ParamDef);
            lengths[(Int32)Tables.EventPtr] = RowSize(Tables.XEvent);
            lengths[(Int32)Tables.PropertyPtr] = RowSize(Tables.XProperty);
            return;
        }

        // This method is great for iterating over rows, starting at 0.  But metadata tokens start
        // numbering at row 1.  So this can't be used if you've retrieved a token from the metadata!
        internal void SeekToRowOfTable(Tables T, UInt32 Row)
        {
            if (Row >= NRows[(Int32) T])
                throw new BadImageFormatException(
                    String.Format(CultureInfo.CurrentCulture, Res.InvalidMetadataTokenTooManyRows,
                                  Row, T, Names[(Int32) T], NRows[(Int32) T]));
            B.BaseStream.Seek(tableAt[(Int32)T]+Row*lengths[(Int32)T], SeekOrigin.Begin);
        }

        // Use this method whenever using a MetadataToken, otherwise you'll be off by 1!
        internal void SeekToMDToken(MetadataToken token)
        {
            if (token.Index == 0)
                throw new BadImageFormatException(String.Format(CultureInfo.CurrentCulture, Res.NilMetadataToken, token));
            SeekToRowOfTable(token.Table, token.Index - 1);
        }

        internal UInt32 ReadStringIndex()
        {
            if (stringIndex == 2) return (UInt32)B.ReadUInt16();
            else return B.ReadUInt32();
        }

        private String GetString(UInt32 index)
        { // Read a string from the string stream
            stringStream.SeekTo(B, index);
            return ReadNextString();
        }

        private String ReadNextString()
        {
            List<byte> bytes = new List<byte>();
            byte next;
            while ((next = B.ReadByte()) != 0)
                bytes.Add(next);
            return Encoder.GetString(bytes.ToArray());
        }

        /*
        internal void DumpStringHeap(TextWriter output)
        {
            stringStream.SeekTo(B, 0);
            long Start = B.BaseStream.Position;
            long End = Start + stringStream.Size - 1;
            output.WriteLine("String heap: {0}(0x{0:x}) bytes", End - Start);
            for (long i = Start; i < End; i = B.BaseStream.Position)
            {
                output.WriteLine("0x{0:x}: '{1}'", i - Start, ReadNextString());
            }
        }
         */

        internal byte[] ReadBlob()
        {
            UInt32 Index = ReadBlobIndex();
            if (Index == 0) return null;
            UInt32 Position = (UInt32)B.BaseStream.Position;
            blobStream.SeekTo(B, Index);
            UInt32 NBytes = (UInt32)B.ReadByte();
            Byte[] Result = null;
            if ((NBytes & 0x80) != 0)
            {
                if ((NBytes & 0xC0) == 0x80)
                    NBytes = ((NBytes & 0x7F) << 8) + B.ReadByte();
                else NBytes = ((NBytes & 0x7F) << 24) +
                    (UInt16)(B.ReadByte() << 16) +
                    (B.ReadUInt16());
            }
            if (NBytes != 0)
            {
                Result = new byte[NBytes];
                for (UInt32 i = 0; i < NBytes; i++)
                    Result[i] = B.ReadByte();
            }
            B.BaseStream.Seek(Position, SeekOrigin.Begin);
            return Result;
        }

        internal String ReadString()
        {
            UInt32 index = ReadStringIndex();
            UInt32 position = (UInt32) B.BaseStream.Position;
            String result = GetString(index);
            B.BaseStream.Seek(position, SeekOrigin.Begin);
            return result;
        }

        /*
        internal UInt32 ReadGUIDIndex()
        {
            if (GUIDIndex == 2) 
                return (UInt32)B.ReadUInt16();
            else 
                return B.ReadUInt32();
        }
        */

        internal UInt32 ReadBlobIndex()
        {
            if (blobIndex == 2) 
                return (UInt32)B.ReadUInt16();
            else 
                return B.ReadUInt32();
        }

        internal UInt32 ReadRowIndex(Tables T)
        {
            if (RowSize(T) == 2) 
                return (UInt32)B.ReadUInt16();
            else 
                return B.ReadUInt32();
        }

        internal UInt32 RowsInTable(Tables T)
        {
            return NRows[(UInt32)T];
        }

        /*
        internal UInt32 ReadEncodedInt()
        {
            byte first = B.ReadByte();
            if ((first & 0x80) == 0)
                return first;
            byte second = B.ReadByte();
            if ((first & 0xC0) == 0x80)
                return ((uint)(first & 0x7F)) << 8 | second;
            else if ((first & 0xE0) == 0xC0)
            {
                byte third = B.ReadByte();
                byte fourth = B.ReadByte();
                return ((uint)(first & 0x1F) << 24) | ((uint)second) << 16 | ((uint)third) << 8 | (uint)fourth;
            }
            else
                throw new BadImageFormatException(Res.InvalidCompressedInt);
        }
        */

        internal MetadataToken ReadMetadataToken(Encodings E)
        {
            UInt32 Code = (MetadataTokenSize(E) == 2) ? B.ReadUInt16() : B.ReadUInt32();
            MetadataToken Result;
            EncodingDesc Desc = EncodingDescs[(Int32)E];
            Tables[] Ts = Desc.WhichTables;
            UInt32 Mask = (1U << Desc.TagBits) - 1;
            Result.Table = Ts[Code & Mask];
            Result.Index = Code >> Desc.TagBits;
            if (Result.Table == Tables.Invalid)
                throw new BadImageFormatException(Res.InvalidMetadataTokenNilTable);
            return Result;
        }
    }
}
