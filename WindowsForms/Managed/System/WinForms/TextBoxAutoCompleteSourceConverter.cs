//------------------------------------------------------------------------------
// <copyright file="TextBoxAutoCompleteSourceConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Collections;
    
    internal class TextBoxAutoCompleteSourceConverter : EnumConverter {
        public TextBoxAutoCompleteSourceConverter(Type type) : base(type) {
        }

        /// <include file='doc\TextBoxAutoCompleteSourceConverter.uex' path='docs/doc[@for="TextBoxAutoCompleteSourceConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            StandardValuesCollection values = base.GetStandardValues(context);
            ArrayList list = new ArrayList();
            int count = values.Count;
            for (int i=0; i<count; i++)
            {
              string currentItemText = values[i].ToString();
              if (!currentItemText.Equals("ListItems"))
              {
                list.Add(values[i]);
              }
            }
            return new StandardValuesCollection(list);
            
        }
    }
}

