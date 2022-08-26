

/***************************************************************************\
*
* File: DateTimeValueSerializerContext.cs
*
\***************************************************************************/

using System.Globalization;
using System.Text;
using System.ComponentModel;

namespace System.Windows.Markup
{

    // This is a helper class used by the DateTimeConverter2 to call the DateTimeValueSerializer.
    // It provides no functionality.

    internal class DateTimeValueSerializerContext : IValueSerializerContext
    {
        public ValueSerializer GetValueSerializerFor(PropertyDescriptor descriptor)
        {
            return null;
        }

        public ValueSerializer GetValueSerializerFor(Type type)
        {
            return null;
        }


        public IContainer Container
        {
            get { return null; }
        }

        public object Instance
        {
            get { return null; }
        }

        public void OnComponentChanged()
        {
        }

        public bool OnComponentChanging()
        {
            return false;
        }

        public PropertyDescriptor PropertyDescriptor
        {
            get { return null; }
        }

        public object GetService(Type serviceType)
        {
            return null;
        }

    }


}

