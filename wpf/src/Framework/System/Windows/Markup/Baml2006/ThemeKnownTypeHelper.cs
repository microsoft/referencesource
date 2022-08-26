using System;
using System.Collections.Generic;
using System.Text;
using System.Xaml;

namespace System.Windows.Baml2006
{
    internal abstract class ThemeKnownTypeHelper
    {
        public abstract XamlType GetKnownXamlType(string name);
    }
}
