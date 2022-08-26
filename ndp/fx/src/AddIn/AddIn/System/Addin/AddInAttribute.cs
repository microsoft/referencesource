// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Purpose: Attributes for the AddIn model
** 
===========================================================*/
using System;
using System.Diagnostics.Contracts;

namespace System.AddIn
{
    // Note that attributes for the other pipeline components
    // have been moved to Pipeline\AddInPipelineAttributes.cs

    [AttributeUsage(AttributeTargets.Class)]
    public sealed class AddInAttribute : Attribute
    {
        private String _name;
        private String _publisher;
        private String _version;
        private String _description;

#if LOCALIZABLE_ADDIN_ATTRIBUTE
        // For localization
        private String _resMgrBaseName;
        private String _nameResource;
        private String _publisherResource;
        private String _descriptionResource;
#endif

        public AddInAttribute(String name)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (name.Length <= 0)
                throw new ArgumentException(Res.AddInNameEmpty);
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            _name = name;
        }

#if LOCALIZABLE_ADDIN_ATTRIBUTE
        // For localization
        public AddInAttribute(String resourceManagerBaseName, String addinNameResourceName)
        {
            if (resourceManagerBaseName == null)
                throw new ArgumentNullException("resourceManagerBaseName");
            if (addinNameResourceName == null)
                throw new ArgumentNullException("addinNameResourceName");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            _resMgrBaseName = resourceManagerBaseName;
            _nameResource = addinNameResourceName;
        }
#endif

        public String Name {
            get { return _name; }
        }

        public String Publisher {
            get { return _publisher; }
            set { _publisher = value; }
        }

        public String Version {
            get { return _version; }
            set { _version = value; }
        }

        public String Description {
            get { return _description; }
            set { _description = value; }
        }

#if LOCALIZABLE_ADDIN_ATTRIBUTE
        // For localization
        public String ResourceManagerBaseName {
            get { return _resMgrBaseName; }
        }

        public String NameResourceName {
            get { return _nameResource; }
        }

        public String PublisherResourceName {
            get { return _publisherResource; }
            set { _publisherResource = value; }
        }

        public String DescriptionResourceName {
            get { return _descriptionResource; }
            set { _descriptionResource = value; }
        }
#endif // LOCALIZABLE_ADDIN_ATTRIBUTE
    }
}
