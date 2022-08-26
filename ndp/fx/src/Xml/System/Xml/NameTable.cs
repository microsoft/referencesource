//------------------------------------------------------------------------------
// <copyright file="NameTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">Microsoft</owner>
//------------------------------------------------------------------------------

using System;

namespace System.Xml {

    /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable"]/*' />
    /// <devdoc>
    ///    <para>
    ///       XmlNameTable implemented as a simple hash table.
    ///    </para>
    /// </devdoc>
    public class NameTable : XmlNameTable {
//
// Private types
//
        class Entry {
            internal string str;
            internal int    hashCode;
            internal Entry  next;

            internal Entry( string str, int hashCode, Entry next ) {
                this.str = str;
                this.hashCode = hashCode;
                this.next = next;
            }
        }

//
// Fields
//
        Entry[] entries;
        int     count;
        int     mask;
#pragma warning disable 169
        int hashCodeRandomizer; // Used only on Silverlight but still defined for compatibility
#pragma warning restore 169

#if !SILVERLIGHT
        ulong   marvinHashSeed;
#endif

//
// Constructor
//
        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.NameTable"]/*' />
        /// <devdoc>
        ///      Public constructor.
        /// </devdoc>
        public NameTable() {
            mask = 31;
            entries = new Entry[mask+1];
#if SILVERLIGHT
            hashCodeRandomizer = Environment.TickCount;
#else
            marvinHashSeed = MarvinHash.DefaultSeed;
#endif
        }

//
// XmlNameTable public methods
//
        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Add"]/*' />
        /// <devdoc>
        ///      Add the given string to the NameTable or return
        ///      the existing string if it is already in the NameTable.
        /// </devdoc>
        public override string Add( string key ) {
            if ( key == null ) {
                throw new ArgumentNullException( "key" );
            }
            int len = key.Length;            
            if ( len == 0 ) {
                return string.Empty;
            }

            int hashCode = ComputeHash32(key);

            for ( Entry e = entries[hashCode & mask]; e != null; e = e.next ) {
                if ( e.hashCode == hashCode && e.str.Equals( key ) ) {
                    return e.str;
                }
            }
            return AddEntry( key, hashCode );
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Add1"]/*' />
        /// <devdoc>
        ///      Add the given string to the NameTable or return
        ///      the existing string if it is already in the NameTable.
        /// </devdoc>
        public override string Add( char[] key, int start, int len ) {
            if ( len == 0 ) {
                return string.Empty;
            }

            // Compatibility check to ensure same exception as previous versions
            // independently of any exceptions throw by the hashing function.
            // note that NullReferenceException is the first one if key is null.
            if (start >= key.Length || start < 0 || (long)start + len > (long)key.Length) {
                throw new IndexOutOfRangeException();
            }

            // Compatibility check for len < 0, just throw the same exception as new string(key, start, len)
            if (len < 0) {
                throw new ArgumentOutOfRangeException();
            }

            int hashCode = ComputeHash32(key, start, len);

            for ( Entry e = entries[hashCode & mask]; e != null; e = e.next ) {
                if ( e.hashCode == hashCode && TextEquals( e.str, key, start, len ) ) {
                    return e.str;
                }
            }
            return AddEntry( new string( key, start, len ), hashCode );
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Get"]/*' />
        /// <devdoc>
        ///      Find the matching string in the NameTable.
        /// </devdoc>
        public override string Get( string value ) {
            if ( value == null ) {
                throw new ArgumentNullException("value");
            }
            if ( value.Length == 0 ) {
                return string.Empty;
            }

            int hashCode = ComputeHash32(value);

            for ( Entry e = entries[hashCode & mask]; e != null; e = e.next ) {
                if ( e.hashCode == hashCode && e.str.Equals( value ) ) {
                    return e.str;
                }
            }
            return null;
        }

        /// <include file='doc\NameTable.uex' path='docs/doc[@for="NameTable.Get1"]/*' />
        /// <devdoc>
        ///      Find the matching string atom given a range of
        ///      characters.
        /// </devdoc>
        public override string Get( char[] key, int start, int len ) {
            if ( len == 0 ) {
                return string.Empty;
            }

            // Compatibility check to ensure same exception as previous versions
            // independently of any exceptions throw by the hashing function.
            // note that NullReferenceException is the first one if key is null.
            if (start >= key.Length || start < 0 || (long)start + len > (long)key.Length) {
                throw new IndexOutOfRangeException();
            }

            // Compatibility check for len < 0, just return null
            if (len < 0) {
                return null;
            }

            int hashCode = ComputeHash32(key, start, len);

            for ( Entry e = entries[hashCode & mask]; e != null; e = e.next ) {
                if ( e.hashCode == hashCode && TextEquals( e.str, key, start, len ) ) {
                    return e.str;
                }
            }
            return null;
        }

//
// Private methods
//

        private string AddEntry( string str, int hashCode ) {
            int index = hashCode & mask;
            Entry e = new Entry( str, hashCode, entries[index] );
            entries[index] = e;
            if ( count++ == mask ) {
                Grow();
            }
            return e.str;
        }

        private void Grow() {
            int newMask = mask * 2 + 1;
            Entry[] oldEntries = entries;
            Entry[] newEntries = new Entry[newMask+1];

            // use oldEntries.Length to eliminate the rangecheck            
            for ( int i = 0; i < oldEntries.Length; i++ ) {
                Entry e = oldEntries[i];
                while ( e != null ) {
                    int newIndex = e.hashCode & newMask;
                    Entry tmp = e.next;
                    e.next = newEntries[newIndex];
                    newEntries[newIndex] = e;
                    e = tmp;
                }
            }
            
            entries = newEntries;
            mask = newMask;
        }

        private static bool TextEquals( string str1, char[] str2, int str2Start, int str2Length ) {
            if ( str1.Length != str2Length ) {
                return false;
            }
            // use array.Length to eliminate the rangecheck
            for ( int i = 0; i < str1.Length; i++ ) {
                if ( str1[i] != str2[str2Start+i] ) {
                    return false;
                }
            }
            return true;
        }

#if SILVERLIGHT
        // Marvin hash is not being added to Silverlight keep on legacy hashing
        private int ComputeHash32(string key)
        {
            int hashCode = key.Length + hashCodeRandomizer;
            // use key.Length to eliminate the rangecheck
            for ( int i = 0; i < key.Length; i++ ) {
                hashCode += ( hashCode << 7 ) ^ key[i];
            }
            // mix it a bit more
            hashCode -= hashCode >> 17; 
            hashCode -= hashCode >> 11; 
            hashCode -= hashCode >> 5;

            return hashCode;
        }

        private int ComputeHash32(char[] key, int start, int len)
        {
            int hashCode = len + hashCodeRandomizer;
            hashCode += (hashCode << 7) ^ key[start];   // this will throw IndexOutOfRangeException in case the start index is invalid
            int end = start + len;
            for (int i = start + 1; i < end; i++)
            {
                hashCode += (hashCode << 7) ^ key[i];
            }
            // mix it a bit more
            hashCode -= hashCode >> 17;
            hashCode -= hashCode >> 11;
            hashCode -= hashCode >> 5;

            return hashCode;
        }
#else
        private int ComputeHash32(string key)
        {
            return MarvinHash.ComputeHash32(key, marvinHashSeed);
        }

        private int ComputeHash32(char[] key, int start, int len)
        {
            return MarvinHash.ComputeHash32(key, start, len, marvinHashSeed);
        }
#endif
    }
}
