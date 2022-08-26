
// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  QualificationDataItem 
**
** A simple struct for key-value pairs on pipeline elements 
**
===========================================================*/
using System;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    [Serializable]
    public struct QualificationDataItem
    {
        AddInSegmentType m_type;
        String m_key;
        String m_value;

        internal QualificationDataItem(AddInSegmentType addInSegmentType, String name, String val)
        {
            m_type = addInSegmentType;
            m_key = name;
            m_value = val;
        }

        public AddInSegmentType Segment 
        {
            get { return m_type; }
        }

        public String Name
        {
            get { return m_key; }
        }

        public String Value
        {
            get { return m_value; }
        }

        public override bool Equals(object obj)
        {
            QualificationDataItem that = (QualificationDataItem)obj;
            return (String.Equals(m_key, that.m_key, StringComparison.Ordinal) &&
                    String.Equals(m_value, that.m_value, StringComparison.Ordinal) &&
                    m_type == that.m_type);
        }

        public override int GetHashCode()
        {
            return m_type.GetHashCode() ^ (Name == null ? 0 : Name.GetHashCode());
        }

        public static bool operator ==(QualificationDataItem item1, QualificationDataItem item2)
        {
            return item1.Equals(item2);
        }

        public static bool operator !=(QualificationDataItem item1, QualificationDataItem item2)
        {
            return !item1.Equals(item2);
        }
    }
}

