// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Syntax
{
    internal partial class NameManager
    {
        private NameTable names;

        internal NameManager()
            : this(new NameTable())
        {
        }

        internal NameManager(NameTable nameTable)
        {
            names = nameTable;
            this.InitKnownNames();
        }

        internal Name Add(string key)
        {
            if (key == null)
            {
                throw Error.InternalCompilerError();
            }
            Name name = _knownNames.Lookup(key);
            if (name == null)
            {
                name = this.names.Add(key);
            }
            return name;
        }

        internal Name Lookup(string key)
        {
            if (key == null)
            {
                throw Error.InternalCompilerError();
            }
            Name name = _knownNames.Lookup(key);
            if (name == null)
            {
                name = this.names.Lookup(key);
            }
            return name;
        }

        internal Name GetPredefinedName(PredefinedName id)
        {
            return _predefinedNames[(int)id];
        }

        internal Name GetPredefName(PredefinedName id)
        {
            return GetPredefinedName(id);
        }
    }
}
