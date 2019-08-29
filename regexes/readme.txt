IMPORTANT: because the runtime that is used for building in a razzle window is often quite out of date, we can't reliably run RegexPreCompiler.exe from a razzle window.  So instead, we need to manually run it from suite window to get the correct IL file.

In order to do this, please follow these steps:
- Make your changes to ndp\fx\src\xsp\regcomp\RegexPrecompiler.cs
- build from ndp\fx\src\xsp\regcomp from a razzle window
- Open regexes\System.Web.RegularExpressions.ilpp for editing (using "tf edit")
- Open a suite window
- Go to ndp\fx\src\xsp\regexes
- Run the batch file RebuildRegExILFile.cmd. This will regenerate System.Web.RegularExpressions.ilpp. See below for info how to do this correctly.
- You then need to 'build -c' from ndp\fx\src\xsp\regexes in a RAZZLE window in order to actually build System.Web.RegularExpressions.dll.
- Check in the modified System.Web.RegularExpressions.ilpp.


** Important note when the Regex engine in System.dll was modified in the current release:

   When running RegexPreCompiler from the SUITE window it will run against the public runtime
   installed on the machine.

   The public runtime is typically a previously shipped RTM version. This means that any
   changes made to the RegEx compiler version in the current release will *not* be effective.
   There are several ways to avoid this, depending on your configuration (adjust
   "v4.0.amd64chk" below to "v4.0.x86chk" or to whatever your current runtime build install
   name is):

   - Add a RegexPreCompiler.exe.config file with
     "<configuration><startup>  <supportedRuntime version="v4.0.amd64chk" />  </startup></configuration>"

   - Do set COMPLUS_DEFAULTVERSION=v4.0.amd64chk
        set COMPLUS_VERSION=v4.0.amd64chk
     before running RegexPreCompiler and do
        set COMPLUS_DEFAULTVERSION=
        set COMPLUS_VERSION=
     afterwards.

   - Instead of running RebuildRegExILFile.cmd from a SUITE windows, run it from a ClrEnv window.
     This should have all the correct environment magic set up.


Please contact DavidEbb for questions on this.
