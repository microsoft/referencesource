//---------------------------------------------------------------------------
//
// File: PropertyRecord.cs
//
// Description: DependencyProperty/value pair struct used by TextContainer undo.
//
// History:  
//  03/03/2004 : Microsoft - Created
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Documents
{
    // This struct records DependencyProperty/value pairs. We use the struct
    // extensively because LocalValueEnumerators may not be cached safely.
    // It is identical to base's LocalValueEntry except that it adds setters.
    internal struct PropertyRecord
    {
        internal DependencyProperty Property
        {
            get { return _property; }
            set { _property = value; }
        }

        internal object Value
        {
            get { return _value; }
            set { _value = value; }
        }

        private DependencyProperty _property;

        private object _value;
    }
}
