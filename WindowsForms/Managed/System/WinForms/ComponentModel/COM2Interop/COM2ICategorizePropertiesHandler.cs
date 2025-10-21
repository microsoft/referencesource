//------------------------------------------------------------------------------
// <copyright file="COM2ICategorizePropertiesHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using Microsoft.Win32;

    [System.Security.SuppressUnmanagedCodeSecurityAttribute()]
    internal class Com2ICategorizePropertiesHandler : Com2ExtendedBrowsingHandler {

        public override Type Interface {
            get {
                return typeof(NativeMethods.ICategorizeProperties);
            }
        }

        private string GetCategoryFromObject(object obj, int dispid) {
            if (obj == null) {
                return null;
            }

            if (obj is NativeMethods.ICategorizeProperties) {
                NativeMethods.ICategorizeProperties catObj = (NativeMethods.ICategorizeProperties)obj;

                try {
                    int categoryID = 0;

                    if (NativeMethods.S_OK == catObj.MapPropertyToCategory(dispid, ref categoryID)) {
                        string categoryName = null;
                        
                        switch (categoryID) {
                            case NativeMethods.ActiveX.PROPCAT_Nil:
                                return "";
                            case NativeMethods.ActiveX.PROPCAT_Misc:
                                return SR.GetString(SR.PropertyCategoryMisc);
                            case NativeMethods.ActiveX.PROPCAT_Font:
                                return SR.GetString(SR.PropertyCategoryFont);
                            case NativeMethods.ActiveX.PROPCAT_Position:
                                return SR.GetString(SR.PropertyCategoryPosition);
                            case NativeMethods.ActiveX.PROPCAT_Appearance:
                                return SR.GetString(SR.PropertyCategoryAppearance);
                            case NativeMethods.ActiveX.PROPCAT_Behavior:
                                return SR.GetString(SR.PropertyCategoryBehavior);
                            case NativeMethods.ActiveX.PROPCAT_Data:
                                return SR.GetString(SR.PropertyCategoryData);
                            case NativeMethods.ActiveX.PROPCAT_List:
                                return SR.GetString(SR.PropertyCategoryList);
                            case NativeMethods.ActiveX.PROPCAT_Text:
                                return SR.GetString(SR.PropertyCategoryText);
                            case NativeMethods.ActiveX.PROPCAT_Scale:
                                return SR.GetString(SR.PropertyCategoryScale);
                            case NativeMethods.ActiveX.PROPCAT_DDE:
                                return SR.GetString(SR.PropertyCategoryDDE);
                        }
                        
                        if (NativeMethods.S_OK == catObj.GetCategoryName(categoryID, CultureInfo.CurrentCulture.LCID, out categoryName)) {
                            return categoryName;
                        }
                    }
                }
                catch {
                }
            }
            return null;
        }

        public override void SetupPropertyHandlers(Com2PropertyDescriptor[] propDesc) {
            if (propDesc == null) {
                return;
            }
            for (int i = 0; i < propDesc.Length; i++) {
                propDesc[i].QueryGetBaseAttributes += new GetAttributesEventHandler(this.OnGetAttributes);
            }
        }

        private void OnGetAttributes(Com2PropertyDescriptor sender, GetAttributesEvent attrEvent) {

            string cat = GetCategoryFromObject(sender.TargetObject, sender.DISPID);

            if (cat != null && cat.Length > 0) {
                attrEvent.Add(new CategoryAttribute(cat));
            }
        }
    }
}
