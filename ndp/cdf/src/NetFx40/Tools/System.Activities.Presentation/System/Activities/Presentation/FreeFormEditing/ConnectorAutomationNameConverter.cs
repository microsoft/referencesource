//----------------------------------------------------------------
// <copyright company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------

namespace System.Activities.Presentation.FreeFormEditing
{
    using System.Globalization;
    using System.Runtime;
    using System.Windows;
    using System.Windows.Data;
    using System.Windows.Media;

    class ConnectorAutomationNameConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (null == values || values.Length != 2 || !(values[0] is string) || !(values[1] is string))
            {
                return DependencyProperty.UnsetValue;
            }

            string automationName = (string)values[0];
            string labelText = (string)values[1];
            if (String.IsNullOrEmpty(labelText))
            {
                return automationName;
            }
            else
            {
                return String.Format(SR.ConnectorWithLabelAutomationName, labelText);
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            throw FxTrace.Exception.AsError(new NotSupportedException());
        }
    }
}
