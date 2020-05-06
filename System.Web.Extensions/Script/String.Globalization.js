#if COPYRIGHT
//------------------------------------------------------------------------------
// <copyright file="String.Globalization.js" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#endif

String.localeFormat = function(format, args) {
    /// <summary>
    ///     Replaces the format items in a specified String with the text equivalents of the values of
    ///     corresponding object instances.  The current culture will be used to format dates and numbers.
    /// </summary>
    /// <param name="format" type="String">A format string.</param>
    /// <param name="args" parameterArray="true" mayBeNull="true">The objects to format.</param>
    /// <returns type="String">
    ///     A copy of format in which the format items have been replaced by the
    ///     string equivalent of the corresponding instances of object arguments.
    /// </returns>
    return String._toFormattedString(true, arguments);
}
