//------------------------------------------------------------------------------
// <copyright file="SpecialFolderEnumConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Collections;
    
    internal class SpecialFolderEnumConverter : AlphaSortedEnumConverter {
        public SpecialFolderEnumConverter(Type type) : base(type) {
        }

        /// <include file='doc\SpecialFolderEnumConverter.uex' path='docs/doc[@for="SpecialFolderEnumConverter.GetStandardValues"]/*' />
        /// See VSWhidbey #376570. Personal appears twice in type editor because its numeric value matches with MyDocuments.
        /// This code filters out the duplicate value.
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            StandardValuesCollection values = base.GetStandardValues(context);
            ArrayList list = new ArrayList();
            int count = values.Count;
            bool personalSeen = false;
            for (int i = 0; i < count; i++) {
                 if (values[i] is System.Environment.SpecialFolder &&
                    values[i].Equals(System.Environment.SpecialFolder.Personal)) {
                    if (!personalSeen) {
                        personalSeen = true;
                        list.Add(values[i]);
                    }
                }
                else {
                    list.Add(values[i]);
                }
            }
            return new StandardValuesCollection(list);
        }
    }
}
