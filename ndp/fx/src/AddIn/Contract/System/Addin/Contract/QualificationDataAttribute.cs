/// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Purpose: Attribute on any pipeline component for use in qualification.
** 
===========================================================*/

using System;

namespace System.AddIn.Pipeline
{
    [AttributeUsage(AttributeTargets.Interface | AttributeTargets.Class, AllowMultiple = true)]
    public sealed class QualificationDataAttribute : Attribute
    {
        private String m_name;
        private String m_value;

        public QualificationDataAttribute(String name, String value) 
        { 
            m_name = name;
            m_value = value;
        }

        public String Name
        {
            get { return m_name; }
        }

        public String Value
        {
            get { return m_value; }
        }
    }
}

