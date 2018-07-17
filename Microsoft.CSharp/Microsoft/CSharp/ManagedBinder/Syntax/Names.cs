// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Syntax
{
    internal class Name
    {
        private string _text;

        public Name(string text)
        {
            this._text = text;
        }

        public string Text
        {
            get { return this._text; }
        }

        public virtual PredefinedName PredefinedName
        {
            get { return PredefinedName.PN_COUNT; }
        }

        public override string ToString()
        {
            return this._text;
        }
    }
}
