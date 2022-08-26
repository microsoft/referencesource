//-------------------------------------------------------
// Class that defines all the non-localizable constants 
// in Baml PomParser.
//  
// Created: Garyyang @ 12/1/2003
//
//-------------------------------------------------------
namespace MS.Internal.Globalization
{ 
    internal static class BamlConst
    {
        internal const string ContentSuffix              ="$Content";
        internal const string LiteralContentSuffix       ="$LiteralContent";                
        // parsing $Content.
        internal const char KeySeperator                 = ':';
        internal const char ChildStart                   = '#';
        internal const char ChildEnd                     = ';';
        internal const char EscapeChar                   = '\\';
    }        
}
