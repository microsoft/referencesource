//------------------------------------------------------------------------------
// <copyright file="SerializableRegistryKey.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using Microsoft.Win32;
    using System.Runtime.Serialization;
    
    [Serializable()]
    internal class SerializableRegistryKey {
        // these fields are public so that they can be serialized.
        public string[] ValueNames;
        public object[] Values;
        [OptionalField(VersionAdded=2)]
        public RegistryValueKind[] ValueKinds;
        public string[] KeyNames;
        public SerializableRegistryKey[] Keys;

        public SerializableRegistryKey(RegistryKey keyToSave) {
            CopyFromRegistry(keyToSave);
        }

        public void CopyFromRegistry(RegistryKey keyToSave) {
            if (keyToSave == null)
                throw new ArgumentNullException("keyToSave");

            ValueNames = keyToSave.GetValueNames();
            //RegistryKey might return null on this call, if no values are
            //available.
            if (ValueNames == null)
                ValueNames = new string[0];
            Values = new object[ValueNames.Length];
            ValueKinds = new RegistryValueKind[ValueNames.Length];
            for (int i = 0; i < ValueNames.Length; i++) {
                Values[i] = keyToSave.GetValue(ValueNames[i], null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                ValueKinds[i] = keyToSave.GetValueKind(ValueNames[i]);
            }

            KeyNames = keyToSave.GetSubKeyNames();
            //RegistryKey might return null on this call, if no values are
            //available.
            if (KeyNames == null)
                KeyNames = new string[0];
            Keys = new SerializableRegistryKey[KeyNames.Length];
            for (int i = 0; i < KeyNames.Length; i++)
                Keys[i] = new SerializableRegistryKey(keyToSave.OpenSubKey(KeyNames[i]));
        }

        public void CopyToRegistry(RegistryKey baseKey) {
            if (baseKey == null)
                throw new ArgumentNullException("baseKey");

            if (Values != null) {
                for (int i = 0; i < Values.Length; i++) {
                    if(ValueKinds != null) {
                        baseKey.SetValue(ValueNames[i], Values[i], ValueKinds[i]);
                    } else {
                        baseKey.SetValue(ValueNames[i], Values[i]);
                    }
                }
            }

            if (Keys != null) {
                for (int i = 0; i < Keys.Length; i++) {
                    RegistryKey key = baseKey.CreateSubKey(KeyNames[i]);
                    Keys[i].CopyToRegistry(key);
                }
            }
        }
    }
}

  
