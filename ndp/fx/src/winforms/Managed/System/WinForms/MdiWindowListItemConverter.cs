//------------------------------------------------------------------------------
// <copyright file="MdiWindowListItemConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Collections;
    
    internal class MdiWindowListItemConverter : ComponentConverter {
        public MdiWindowListItemConverter(Type type) : base(type) {
        }

        /// <include file='doc\TextBoxAutoCompleteSourceConverter.uex' path='docs/doc[@for="TextBoxAutoCompleteSourceConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            MenuStrip menu = context.Instance as MenuStrip;
            if (menu != null)
            {
                StandardValuesCollection values = base.GetStandardValues(context);
                ArrayList list = new ArrayList();
                int count = values.Count;
                for (int i=0; i<count; i++)
                {
                    ToolStripItem currentItem = values[i] as ToolStripItem;
                    if (currentItem != null && currentItem.Owner == menu)
                    {
                       list.Add(currentItem); 
                    }
                }
                return new StandardValuesCollection(list);
                
            }
            return base.GetStandardValues(context);
        }
    }
}

