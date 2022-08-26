//---------------------------------------------------------------------
// <copyright file="UriHelper.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Utility function for writing out URIs
// </summary>
//
// @owner  aconrad
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    /// <summary>utility class for helping construct uris</summary>
    internal static class UriHelper
    {
        /// <summary>forwardslash character</summary>
        internal const char FORWARDSLASH = '/';

        /// <summary>leftparan character</summary>
        internal const char LEFTPAREN = '(';

        /// <summary>rightparan character</summary>
        internal const char RIGHTPAREN = ')';

        /// <summary>questionmark character</summary>
        internal const char QUESTIONMARK = '?';

        /// <summary>ampersand character</summary>
        internal const char AMPERSAND = '&';

        /// <summary>equals character</summary>
        internal const char EQUALSSIGN = '=';

        /// <summary>dollar sign character</summary>
        internal const char DOLLARSIGN = '$';

        /// <summary>dollar sign character</summary>
        internal const char SPACE = ' ';

        /// <summary>dollar sign character</summary>
        internal const char COMMA = ',';

        /// <summary>single quote</summary>
        internal const char QUOTE = '\'';

        /// <summary>asterisk</summary>
        internal const char ASTERISK = '*';

        /// <summary>top</summary>
        internal const string OPTIONTOP = "top";

        /// <summary>skip</summary>
        internal const string OPTIONSKIP = "skip";

        /// <summary>orderby</summary>
        internal const string OPTIONORDERBY = "orderby";

        /// <summary>where</summary>
        internal const string OPTIONFILTER = "filter";

        /// <summary>desc</summary>
        internal const string OPTIONDESC = "desc";

        /// <summary>expand</summary>
        internal const string OPTIONEXPAND = "expand";

        /// <summary>inlinecount</summary>
        internal const string OPTIONCOUNT = "inlinecount";

        /// <summary>select</summary>
        internal const string OPTIONSELECT = "select";

        /// <summary>allpages</summary>
        internal const string COUNTALL = "allpages";

        /// <summary>value</summary>
        internal const string COUNT = "count";

        /// <summary>and</summary>
        internal const string AND = "and";

        /// <summary>or</summary>
        internal const string OR = "or";

        /// <summary>eq</summary>
        internal const string EQ = "eq";

        /// <summary>ne</summary>
        internal const string NE = "ne";

        /// <summary>lt</summary>
        internal const string LT = "lt";

        /// <summary>le</summary>
        internal const string LE = "le";

        /// <summary>gt</summary>
        internal const string GT = "gt";

        /// <summary>ge</summary>
        internal const string GE = "ge";

        /// <summary>add</summary>
        internal const string ADD = "add";

        /// <summary>sub</summary>
        internal const string SUB = "sub";

        /// <summary>mul</summary>
        internal const string MUL = "mul";

        /// <summary>div</summary>
        internal const string DIV = "div";

        /// <summary>mod</summary>
        internal const string MOD = "mod";

        /// <summary>negate</summary>
        internal const string NEGATE = "-";

        /// <summary>not</summary>
        internal const string NOT = "not";

        /// <summary>null</summary>
        internal const string NULL = "null";

        /// <summary>isof</summary>
        internal const string ISOF = "isof";

        /// <summary>cast</summary>
        internal const string CAST = "cast";
    }
}
