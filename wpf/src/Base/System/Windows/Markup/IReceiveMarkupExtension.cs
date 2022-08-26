using System;
using System.ComponentModel;
using System.Globalization;

namespace System.Windows.Markup
{
    [Obsolete("IReceiveMarkupExtension has been deprecated. This interface is no longer in use.")]
    public interface IReceiveMarkupExtension
    {
        void ReceiveMarkupExtension(String property, MarkupExtension markupExtension, IServiceProvider serviceProvider);
    }   
}
