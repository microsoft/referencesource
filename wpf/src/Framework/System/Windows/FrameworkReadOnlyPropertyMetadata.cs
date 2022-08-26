using System;

namespace System.Windows
{
    internal class ReadOnlyFrameworkPropertyMetadata : FrameworkPropertyMetadata
    {
        public ReadOnlyFrameworkPropertyMetadata(object defaultValue, GetReadOnlyValueCallback getValueCallback) :
            base(defaultValue)
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
