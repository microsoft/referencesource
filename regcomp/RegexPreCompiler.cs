using System.Diagnostics.CodeAnalysis;

#region Global Suppressions

[module: SuppressMessage("Microsoft.Naming","CA1709:IdentifiersShouldBeCasedCorrectly", MessageId="regexprecompiler", Scope="module", Target="regexprecompiler.exe", Justification="Shipped previously.")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="regexprecompiler", Scope="module", Target="regexprecompiler.exe", Justification="Shipped previously.")]

#endregion


namespace System {

    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Text.RegularExpressions;

    // these are needed for some of the custom assembly attributes
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.Web;
    using System.Security.Permissions;

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Rgc", Justification="Shipped previously.")]
    public static class Rgc {
        public static int Main(string[] args) {
            
            // get the current assembly and its attributes
            Assembly assembly = Assembly.GetExecutingAssembly();
            AssemblyName an = assembly.GetName();

            // get the ASP.NET regular expressions
            RegexCompilationInfo[] regexes = GetAspnetRegexes();
            an.Name = "System.Web.RegularExpressions";

            Console.WriteLine();
            Console.WriteLine("Compiling {0} Regexes to assembly.", regexes.Length);
            Regex.CompileToAssembly(regexes, an);
            Console.WriteLine("Completed compiling {0} Regexes to assembly.", regexes.Length);
            Console.WriteLine();

            // We want to log loaded assemblies to make debugging easier if the Regex are built using a wrong runtime version:
            
            Console.WriteLine("Runtime assemblies loaded for the compilation:");
            
            Assembly[] loadedAssemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var loadedAssembly in loadedAssemblies)
                ShowAssembly(loadedAssembly);
            
            return 0;
        }

        private static void ShowAssembly(Assembly assembly) {

            String name = assembly.ToString();
            String location = "no on-disk location";

            try {

                location = assembly.Location;

            } catch (NotSupportedException) { }

            Console.WriteLine(" * Loaded assembly: \"{0}\"", name);
            Console.WriteLine("                    ({0}).", location);
            Console.WriteLine();
        }


        /************************************************
        IMPORTANT: If you make changes to any regular expressions here, you will need to
        manually rebuild System.Web.RegularExpressions.il.  Please see readme.txt file in
        the xsp\regexes directory for details.
        ************************************************/

        private static RegexCompilationInfo[] GetAspnetRegexes() {

            string directiveRegexString = @"<%\s*@" +
                @"(" +
                @"\s*(?<attrname>\w[\w:]*(?=\W))(" +            // Attribute name
                @"\s*(?<equal>=)\s*""(?<attrval>[^""]*)""|" +   // ="bar" attribute value
                @"\s*(?<equal>=)\s*'(?<attrval>[^']*)'|" +      // ='bar' attribute value
                @"\s*(?<equal>=)\s*(?<attrval>[^\s""'%>]*)|" +  // =bar attribute value
                @"(?<equal>)(?<attrval>\s*?)" +                 // no attrib value (with no '=')
                @")" +
                @")*" +
                @"\s*?%>";

            RegexOptions regexOptions = RegexOptions.Singleline | RegexOptions.Multiline;
            RegexCompilationInfo[] regexes = new RegexCompilationInfo[27];

            regexes[0] = new RegexCompilationInfo(@"\G<(?<tagname>[\w:\.]+)" +
                                                  @"(" +
                                                  @"\s+(?<attrname>\w[-\w:]*)(" +       // Attribute name
                                                  @"\s*=\s*""(?<attrval>[^""]*)""|" +   // ="bar" attribute value
                                                  @"\s*=\s*'(?<attrval>[^']*)'|" +      // ='bar' attribute value
                                                  @"\s*=\s*(?<attrval><%#.*?%>)|" +     // =<%#expr%> attribute value
                                                  @"\s*=\s*(?<attrval>[^\s=""'/>]*)|" + // =bar attribute value
                                                  @"(?<attrval>\s*?)" +                 // no attrib value (with no '=')
                                                  @")" +
                                                  @")*" +
                                                  @"\s*(?<empty>/)?>",
                                                regexOptions,
                                                "TagRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[1] = new RegexCompilationInfo(@"\G" + directiveRegexString,
                                                regexOptions,
                                                "DirectiveRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[2] = new RegexCompilationInfo(@"\G</(?<tagname>[\w:\.]+)\s*>",
                                                regexOptions,
                                                "EndTagRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[3] = new RegexCompilationInfo(@"\G<%(?!@)(?<code>.*?)%>",
                                                regexOptions,
                                                "AspCodeRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[4] = new RegexCompilationInfo(@"\G<%\s*?=(?<code>.*?)?%>",
                                                regexOptions,
                                                "AspExprRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[5] = new RegexCompilationInfo(@"\G<%#(?<encode>:)?(?<code>.*?)?%>",
                                                regexOptions,
                                                "DatabindExprRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[6] = new RegexCompilationInfo(@"\G<%--(([^-]*)-)*?-%>",
                                                regexOptions,
                                                "CommentRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[7] = new RegexCompilationInfo(@"\G<!--\s*#(?i:include)\s*(?<pathtype>[\w]+)\s*=\s*[""']?(?<filename>[^\""']*?)[""']?\s*-->",
                                                regexOptions,
                                                "IncludeRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[8] = new RegexCompilationInfo(@"\G[^<]+",
                                                regexOptions,
                                                "TextRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[9] = new RegexCompilationInfo("[^%]>",
                                                regexOptions,
                                                "GTRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[10] = new RegexCompilationInfo("<[^%]",
                                                regexOptions,
                                                "LTRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[11] = new RegexCompilationInfo("<%(?![#$])(([^%]*)%)*?>",
                                                regexOptions,
                                                "ServerTagsRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[12] = new RegexCompilationInfo(@"runat\W*server",
                                                regexOptions | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant,
                                                "RunatServerRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[13] = new RegexCompilationInfo(directiveRegexString,
                                                regexOptions,
                                                "SimpleDirectiveRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[14] = new RegexCompilationInfo(@"\G\s*<%\s*?#(?<encode>:)?(?<code>.*?)?%>\s*\z",
                                                regexOptions,
                                                "DataBindRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[15] = new RegexCompilationInfo(@"\G\s*<%\s*\$\s*(?<code>.*)?%>\s*\z",
                                                regexOptions,
                                                "ExpressionBuilderRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[16] = new RegexCompilationInfo(@"^\s*bind\s*\((?<params>.*)\)\s*\z",
                                                regexOptions | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant,
                                                "BindExpressionRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[17] = new RegexCompilationInfo(@"\s*((""(?<fieldName>(([\w\.]+)|(\[.+\])))"")|('(?<fieldName>(([\w\.]+)|(\[.+\])))'))\s*(,\s*((""(?<formatString>.*)"")|('(?<formatString>.*)'))\s*)?\s*\z",
                                                regexOptions,
                                                "BindParametersRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[18] = new RegexCompilationInfo(@"^(([^""]*("""")?)*)$",
                                                regexOptions,
                                                "FormatStringRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[19] = new RegexCompilationInfo(@"<%\s*=\s*WebResource\(""(?<resourceName>[^""]*)""\)\s*%>",
                                                 regexOptions,
                                                 "WebResourceRegex",
                                                 "System.Web.RegularExpressions",
                                                 false);

            regexes[20] = new RegexCompilationInfo(@"\W",
                                                 regexOptions,
                                                 "NonWordRegex",
                                                 "System.Web.RegularExpressions",
                                                 false);

            regexes[21] = new RegexCompilationInfo(@"^\s*eval\s*\((?<params>.*)\)\s*\z",
                                                regexOptions | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant,
                                                "EvalExpressionRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[22] = new RegexCompilationInfo(@"\$(?:\{(?<name>\w+)\})",
                                                regexOptions,
                                                "BrowserCapsRefRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[23] = new RegexCompilationInfo(@"\G<%:(?<code>.*?)?%>",
                                                regexOptions,
                                                "AspEncodedExprRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            // Note that this string is slightly different from TagRegex above
            // at the "=bar attribute" line. 
            // The 3.5 regex is used only when targeting 2.0/3.5 for 
            // backward compatibility (Dev10 bug 830783).
            regexes[24] = new RegexCompilationInfo(@"\G<(?<tagname>[\w:\.]+)" +
                                                  @"(" +
                                                  @"\s+(?<attrname>\w[-\w:]*)(" +       // Attribute name
                                                  @"\s*=\s*""(?<attrval>[^""]*)""|" +   // ="bar" attribute value
                                                  @"\s*=\s*'(?<attrval>[^']*)'|" +      // ='bar' attribute value
                                                  @"\s*=\s*(?<attrval><%#.*?%>)|" +     // =<%#expr%> attribute value
                                                  @"\s*=\s*(?<attrval>[^\s=/>]*)|" + // =bar attribute value
                                                  @"(?<attrval>\s*?)" +                 // no attrib value (with no '=')
                                                  @")" +
                                                  @")*" +
                                                  @"\s*(?<empty>/)?>",
                                                regexOptions,
                                                "TagRegex35",
                                                "System.Web.RegularExpressions",
                                                true);

            regexes[25] = new RegexCompilationInfo(@"^\s*BindItem\.(?<params>.*)\s*\z",
                                                regexOptions | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant,
                                                "BindItemExpressionRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            regexes[26] = new RegexCompilationInfo(@"(?<fieldName>([\w\.]+))\s*\z",
                                                regexOptions,
                                                "BindItemParametersRegex",
                                                "System.Web.RegularExpressions",
                                                false);

            return regexes;
        }
    }
}
