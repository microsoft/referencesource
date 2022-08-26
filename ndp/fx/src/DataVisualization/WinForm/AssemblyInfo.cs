using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using System.Security;
using System.Runtime.InteropServices;
using System.Resources;

//
// General Information about an assembly is controlled through the following 
// set of properties. Change these attribute values to modify the information
// associated with an assembly.
//
[assembly: InternalsVisibleTo("System.Windows.Forms.DataVisualization.Design, PublicKey=" + AssemblyRef.SharedLibPublicKeyFull)]

// Permissions required
[assembly: SecurityCritical]

#if VS_BUILD
[assembly: AssemblyVersion(ThisAssembly.Version)]
[assembly: AllowPartiallyTrustedCallers]
[assembly: ComVisible(false)]
[assembly: CLSCompliant(true)]
[assembly: NeutralResourcesLanguageAttribute("")]
[assembly: SecurityRules(SecurityRuleSet.Level1, SkipVerificationInFullTrust = true)]
#endif //VS_BUILD

[module: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA900:AptcaAssembliesShouldBeReviewed", 
    Justification = "We have APTCA signoff, for details please refer to SWI Track, Project ID 7972")]