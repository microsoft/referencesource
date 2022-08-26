using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection.MetadataReader
{
    internal sealed class StreamDesc
    {
        private UInt32 offset;    // Offset from metadata root
        private UInt32 position;    // Absolute start in file
        internal String Name;

        internal StreamDesc(BinaryReader B, UInt32 metadataRoot)
        {
            this.offset = B.ReadUInt32();
            this.position = offset + metadataRoot;
            /*Size =*/ B.ReadUInt32();
            int i = 1;
            StringBuilder sb = new StringBuilder();
            char c;
            while ((c = (char)B.ReadByte()) != 0)
            {
                sb.Append(c);
                i += 1;
            }
            while ((i++ & 0x3) != 0)
            {
                B.ReadByte();
            }
            this.Name = sb.ToString();
            // Console.WriteLine("Stream {0}", this.Name);
        }

        internal void SeekTo(BinaryReader B, UInt32 Nth)
        {
            B.BaseStream.Seek(position + Nth, SeekOrigin.Begin);
        }
    }

    internal sealed class PEFileReader : IDisposable
    {
        private static readonly UTF8Encoding Encoder = new UTF8Encoding(false, true);

        internal BinaryReader B;
        private SectionDesc[] Sections;
        private UInt32 MetadataRoot; // Absolute start of metadata in file
        private StreamDesc[] Streams;

        private UInt32 MetaDataDDRVA;
        private MDTables metaData;

        private bool parsedMetaData;
        private string _fileName;

        public PEFileReader(String fileName)
        {
            _fileName = fileName;
            FileStream TheFile = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.RandomAccess);
            B = new BinaryReader(TheFile);
            B.BaseStream.Seek(0x3C, SeekOrigin.Begin);     // Pointer to signature
            UInt32 signatureAt = B.ReadUInt32();
            B.BaseStream.Seek(signatureAt, SeekOrigin.Begin);
            if ((B.ReadByte() != (byte)'P') || (B.ReadByte() != (byte)'E') ||
                (B.ReadByte() != 0) || (B.ReadByte() != 0))
                throw new BadImageFormatException(String.Format(CultureInfo.CurrentCulture, Res.BadPEFile, fileName));

            // Read the magic number to determine whether this is a PE32 or a PE32+ file
            UInt32 MagicAt = signatureAt + 24;
            B.BaseStream.Seek(MagicAt, SeekOrigin.Begin);
            UInt16 magic = B.ReadUInt16();

            UInt16 dataDirectoriesOffset = 0;
            if (magic == 0x10B) {
                dataDirectoriesOffset = 96;  // PE32
            }
            else if (magic == 0x20B) {
                dataDirectoriesOffset = 112;  // PE32+ (applies to /platform:x64)
            }
            else
                throw new BadImageFormatException(String.Format(CultureInfo.CurrentCulture, Res.BadPEFile, fileName));

            UInt32 PEFileHeaderAt = signatureAt + 4;
            UInt32 NSectionsAt = PEFileHeaderAt + 2;
            UInt32 PEOptionalHeaderAt = PEFileHeaderAt + 20;
            UInt32 CLIHeaderDDAt = PEOptionalHeaderAt + dataDirectoriesOffset + 112;  
            B.BaseStream.Seek(CLIHeaderDDAt, SeekOrigin.Begin);
            UInt32 CLIHeaderRVA = B.ReadUInt32();
            MetaDataDDRVA = CLIHeaderRVA + 8;
            UInt32 sectionHeadersAt = PEOptionalHeaderAt + dataDirectoriesOffset + 128;
            B.BaseStream.Seek(NSectionsAt, SeekOrigin.Begin);
            UInt16 NSections = B.ReadUInt16();
            B.BaseStream.Seek(sectionHeadersAt, SeekOrigin.Begin);
            Sections = new SectionDesc[NSections];
            for (UInt16 i = 0; i < NSections; i++)
            {
                B.BaseStream.Seek(sectionHeadersAt + i * 40, SeekOrigin.Begin);
                Sections[i].Read(B);
            }
        }

        public void Dispose()
        {
            if (B != null) {
                B.Close();
                B = null;
            }
        }

        private struct SectionDesc
        {
            private String Name;
            UInt32 VirtualAddress;
            UInt32 Size;
            UInt32 FileOffset;

            internal UInt32 RVAToFileAddress(UInt32 RVA)
            { // returns 0 if not in this section
                if ((RVA < VirtualAddress) || (RVA >= VirtualAddress + Size)) return 0;
                return (RVA - VirtualAddress) + FileOffset;
            }

            private static String ReadString(BinaryReader B, UInt32 NChars)
            {
                StringBuilder sb = new StringBuilder((Int32)NChars);
                for (int i = 1; i <= NChars; i++)
                {
                    byte Next = B.ReadByte();
                    sb.Append(Next == 0 ? ' ' : (char)Next);
                }
                return sb.ToString();
            }

            internal void Read(BinaryReader B)
            {
                this.Name = ReadString(B, 8);
                // Console.WriteLine("Section {0}", Name);
                this.Size = B.ReadUInt32();
                this.VirtualAddress = B.ReadUInt32();
                B.ReadUInt32(); // SizeOfRawData
                this.FileOffset = B.ReadUInt32();
            }
        }

        internal MDTables MetaData {
            get {
                if (!parsedMetaData)
                    InitMetaData();
                return metaData;
            }
        }

        private bool FindStream(String name, out StreamDesc result)
        {
            foreach (StreamDesc s in Streams) {
                if (s.Name.Equals(name)) {
                    result = s;
                    return true;
                }
            }
            result = null;
            return false;
        }

        private String ReadUTF8String(UInt32 NChars)
        {
            return Encoder.GetString(B.ReadBytes((Int32) NChars));
        }

        private UInt32 RVAToFileAddress(UInt32 RVA)
        {
            foreach (SectionDesc s in Sections)
            {
                UInt32 FileOffset = s.RVAToFileAddress(RVA);
                if (FileOffset != 0) return FileOffset;
            }
            throw new BadImageFormatException(String.Format(CultureInfo.CurrentCulture, Res.BadRVA, _fileName));
        }

        private void SeekToRVA(UInt32 RVA)
        {
            B.BaseStream.Seek(RVAToFileAddress(RVA), SeekOrigin.Begin);
        }

        /*
        private static UInt32 PadTo4(UInt32 RVA)
        {
            UInt32 Cleared = RVA & ~0x3U;
            return (RVA == Cleared) ? RVA : RVA + 0x4U;
        }
        */

        internal void InitMetaData()
        {
            if (B == null)
                throw new ObjectDisposedException(null);
            if (parsedMetaData)
                return;

            SeekToRVA(MetaDataDDRVA);
            UInt32 MetaDataRVA = B.ReadUInt32();
            UInt32 MetaDataSize = B.ReadUInt32();
            if ((MetaDataRVA == 0) || (MetaDataSize == 0))
                throw new BadImageFormatException();
            MetadataRoot = RVAToFileAddress(MetaDataRVA);

            UInt32 VersionLengthRVA = MetaDataRVA + 12;
            UInt32 VersionStringRVA = VersionLengthRVA + 4;
            SeekToRVA(VersionLengthRVA);
            UInt32 VersionLength = B.ReadUInt32();
            String Version = ReadUTF8String(VersionLength);
            UInt32 FlagsRVA = VersionStringRVA + VersionLength;
            UInt32 NStreamsRVA = FlagsRVA + 2;
            SeekToRVA(NStreamsRVA);
            UInt16 NStreams = B.ReadUInt16();

            Streams = new StreamDesc[NStreams];
            for (int i = 0; i < NStreams; i++) Streams[i] = new StreamDesc(B, MetadataRoot);
            StreamDesc stringStream, blobStream, TablesStream;
            if (!FindStream("#~", out TablesStream)) 
                if (!FindStream("#-", out TablesStream))
                    throw new BadImageFormatException();
            if (!FindStream("#Strings", out stringStream))
                throw new BadImageFormatException();
            if (!FindStream("#Blob", out blobStream))
                throw new BadImageFormatException();
            TablesStream.SeekTo(B, 0);
            metaData = new MDTables(B, stringStream, blobStream);
            // metaData.DumpStringHeap();
            parsedMetaData = true;
        }

        private static String HexChar(int x)
        {
            switch (x)
            {
                case 0: return "0";
                case 1: return "1";
                case 2: return "2";
                case 3: return "3";
                case 4: return "4";
                case 5: return "5";
                case 6: return "6";
                case 7: return "7";
                case 8: return "8";
                case 9: return "9";
                case 10: return "a";
                case 11: return "b";
                case 12: return "c";
                case 13: return "d";
                case 14: return "e";
                case 15: return "f";
                default: throw new ArgumentOutOfRangeException("x");
            }
        }

        internal static String ByteToHexString(byte b)
        {
            String result = HexChar((b & 0xF0) >> 4) + HexChar(b & 0xF);
            return result;
        }

        internal static String BytesToHexString(byte[] bytes)
        {
            if (bytes == null) return null;
            StringBuilder sb = new StringBuilder(2 * bytes.Length);
            for (int i = 0; i < bytes.Length; i++)
                sb.Append(ByteToHexString(bytes[i]));
            return sb.ToString();
        }

        public bool GetAssemblyInfo(ref AssemblyInfo result)
        {
            InitMetaData();
            // Bunches of test code below here; may be out of date!
            /*
            UInt32 Flags;
            String Name;
            MDTables.MetadataToken Parent;
            
                         
            MetaData.SeekToRowOfTable(MDTables.Tables.XModule, 0);
            UInt16 Generation = B.ReadUInt16();
             Name = MetaData.ReadString();
            UInt32 MvidIndex = MetaData.ReadGUIDIndex();
            UInt32 EncIdIndex = MetaData.ReadGUIDIndex();
            UInt32 EncBaseIdIndex = MetaData.ReadGUIDIndex();

            MetaData.DumpStringHeap(Console.Out);

            for (UInt32 i = 0; i < MetaData.RowsInTable(MDTables.Tables.TypeRef); i++)
            {
                MetaData.SeekToRowOfTable(MDTables.Tables.TypeRef, i);
                MDTables.MetadataToken Scope = MetaData.ReadMetadataToken(MDTables.Encodings.ResolutionScope);
                Name = MetaData.ReadString();
                String NameSpace = MetaData.ReadString();
                Console.WriteLine("Type ref {0} to {1}, {2} in scope {3}", i, NameSpace, Name, Scope);
            }

            UInt32 NRows = MetaData.RowsInTable(MDTables.Tables.MethodDef);
            for (UInt32 i = 0; i < NRows; i++)
            { 
                MetaData.SeekToRowOfTable(MDTables.Tables.MethodDef, i);
                UInt32 RVA = B.ReadUInt32();
                UInt16 ImplFlags = B.ReadUInt16();
                Flags = B.ReadUInt16();
                Name = MetaData.ReadString();
                UInt32 SigIndex = MetaData.ReadBlobIndex();
                UInt32 ParamIndex = MetaData.ReadRowIndex(MDTables.Tables.ParamDef);
                Console.WriteLine("Method {0}: {1}", i, Name);
            }

            for (UInt32 i = 0; i < MetaData.RowsInTable(MDTables.Tables.ParamDef); i++)
            {
                MetaData.SeekToRowOfTable(MDTables.Tables.ParamDef, i);
                Flags = B.ReadUInt16();
                UInt16 Sequence = B.ReadUInt16();
                Name = MetaData.ReadString();
                Console.WriteLine("Param {0} named {1}", i, Name);
            }
            
            MetaData.SeekToRowOfTable(MDTables.Tables.InterfaceImpl, 0U);
            UInt32 Class = MetaData.ReadRowIndex(MDTables.Tables.TypeDef);
            MDTables.MetadataToken Iface = MetaData.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);
            
            MetaData.SeekToRowOfTable(MDTables.Tables.MemberRef, 3);
            MDTables.MetadataToken MemberRefClass = MetaData.ReadMetadataToken(MDTables.Encodings.MemberRefParent);
            Name = MetaData.ReadString();
            UInt32 Signature = MetaData.ReadBlobIndex();

            MetaData.SeekToRowOfTable(MDTables.Tables.FieldMarshal, 0x10U);
            Parent = MetaData.ReadMetadataToken(MDTables.Encodings.HasFieldMarshall);
            UInt32 NativeType = MetaData.ReadBlobIndex();

            MetaData.SeekToRowOfTable(MDTables.Tables.FieldMarshal, 3U);
            Parent = MetaData.ReadMetadataToken(MDTables.Encodings.HasFieldMarshall);
            NativeType = MetaData.ReadBlobIndex();

            MetaData.SeekToRowOfTable(MDTables.Tables.XEvent, 0U);
            Flags = B.ReadUInt16();
            Name = MetaData.ReadString();
            MDTables.MetadataToken EventType = MetaData.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);
            */ 
            
            
            MetaData.SeekToRowOfTable(MDTables.Tables.XAssembly, 0U);

            UInt32 HashAlgorithm = B.ReadUInt32();
            UInt16 Major  = B.ReadUInt16();
            UInt16 Minor = B.ReadUInt16();
            UInt16 Build = B.ReadUInt16();
            UInt16 Revision = B.ReadUInt16();
            Version V = new Version(Major, Minor, Build, Revision);
            UInt32 AssemblyFlags = B.ReadUInt32();
            byte[] PublicKey = MetaData.ReadBlob();
            String SimpleName = MetaData.ReadString();
            String Culture = MetaData.ReadString();
            if ((Culture != null) && (Culture.Length == 0)) Culture = null;
            result = new AssemblyInfo(V, AssemblyFlags, PublicKey, SimpleName, Culture);
            return true;
        }

        /*
        public AssemblyInfo[] GetReferencedAssemblies()
        {
            InitMetaData();

            UInt32 NRefs = MetaData.RowsInTable(MDTables.Tables.AssemblyRef);
            AssemblyInfo[] result = new AssemblyInfo[NRefs];
            for (UInt32 i = 0; i < NRefs; i++)
            {
                MetaData.SeekToRowOfTable(MDTables.Tables.AssemblyRef, i);
                UInt16 Major = B.ReadUInt16();
                UInt16 Minor = B.ReadUInt16();
                UInt16 Build = B.ReadUInt16();
                UInt16 Revision = B.ReadUInt16();
                Version V = new Version(Major, Minor, Build, Revision);
                UInt32 Flags = B.ReadUInt32();
                byte[] PublicKey = MetaData.ReadBlob();
                String Name = MetaData.ReadString();
                String Culture = MetaData.ReadString();
                if ((Culture != null) && (Culture.Length == 0)) Culture = null;
                // Ignore HashValue, index into Blob heap, which follows
                result[i] = new AssemblyInfo(V, Flags, PublicKey, Name, Culture);
            }
            return result;
        }
        */
    }
}
