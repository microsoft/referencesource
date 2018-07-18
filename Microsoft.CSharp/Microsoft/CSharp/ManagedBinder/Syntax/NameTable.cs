// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Syntax
{
    internal class NameTable
    {
        class Entry
        {
            internal readonly Name name;
            internal readonly int hashCode;
            internal Entry next;

            internal Entry(Name name, int hashCode, Entry next)
            {
                this.name = name;
                this.hashCode = hashCode;
                this.next = next;
            }
        }

        Entry[] entries;
        int count;
        int mask;
        int hashCodeRandomizer;

        internal NameTable()
        {
            mask = 31;
            entries = new Entry[mask + 1];
            //hashCodeRandomizer = Environment.TickCount;
            hashCodeRandomizer = 0;
        }

        public Name Add(string key)
        {
            int hashCode = ComputeHashCode(key);
            for (Entry e = entries[hashCode & mask]; e != null; e = e.next)
            {
                if (e.hashCode == hashCode && e.name.Text.Equals(key))
                {
                    return e.name;
                }
            }
            return this.AddEntry(new Name(key), hashCode);
        }

        internal void Add(Name name)
        {
            int hashCode = ComputeHashCode(name.Text);
            // make sure it doesn't already exist
            for (Entry e = entries[hashCode & mask]; e != null; e = e.next)
            {
                if (e.hashCode == hashCode && e.name.Text.Equals(name.Text))
                {
                    throw Error.InternalCompilerError();
                }
            }
            this.AddEntry(name, hashCode);
        }

        public Name Lookup(string key)
        {
            int hashCode = ComputeHashCode(key);
            for (Entry e = entries[hashCode & mask]; e != null; e = e.next)
            {
                if (e.hashCode == hashCode && e.name.Text.Equals(key))
                {
                    return e.name;
                }
            }
            return null;
        }

        private int ComputeHashCode(string key)
        {
            int len = key.Length;
            int hashCode = len + hashCodeRandomizer;
            // use key.Length to eliminate the rangecheck
            for (int i = 0; i < key.Length; i++)
            {
                hashCode += (hashCode << 7) ^ key[i];
            }
            // mix it a bit more
            hashCode -= hashCode >> 17;
            hashCode -= hashCode >> 11;
            hashCode -= hashCode >> 5;
            return hashCode;
        }

        private Name AddEntry(Name name, int hashCode)
        {
            int index = hashCode & mask;
            Entry e = new Entry(name, hashCode, entries[index]);
            this.entries[index] = e;
            if (count++ == mask)
            {
                this.Grow();
            }
            return e.name;
        }

        private void Grow()
        {
            int newMask = mask * 2 + 1;
            Entry[] oldEntries = entries;
            Entry[] newEntries = new Entry[newMask + 1];

            // use oldEntries.Length to eliminate the rangecheck            
            for (int i = 0; i < oldEntries.Length; i++)
            {
                Entry e = oldEntries[i];
                while (e != null)
                {
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
    }
}
