using System;

namespace System.Windows
{
    internal class ReadOnlyPropertyMetadata : PropertyMetadata
    {
        public ReadOnlyPropertyMetadata(object defaultValue, 
                                        GetReadOnlyValueCallback getValueCallback,
                                        PropertyChangedCallback propertyChangedCallback) :
                                        base(defaultValue, propertyChangedCallback)
        {
            _getValueCallback = getValueCallback;
        }

        internal override GetReadOnlyValueCallback GetReadOnlyValueCallback
        {
            get
            {
                return _getValueCallback;
            }
        }

        private GetReadOnlyValueCallback _getValueCallback;
    }
}

