@echo off

ECHO This batch file needs to be run from a SUITE window whenever we make a change
ECHO to a regular expression.
ECHO Also, it should be run whenerver BCL makes a change that affects how
ECHO regex precompilation works.
ECHO. 
ECHO ** Important note when the Regex engine in System.dll was modified in the current release:
ECHO. 
ECHO When running RegexPreCompiler from the SUITE window it will run against the public runtime
ECHO installed on the machine.
ECHO.
ECHO The public runtime is typically a previously shipped RTM version. This means that any
ECHO changes made to the RegEx compiler version in the current release will *not* be effective.
ECHO There are several ways to avoid this, depending on your configuration (adjust
ECHO "v4.0.amd64chk" below to "v4.0.x86chk" or to whatever your current runtime build install
ECHO name is):
ECHO. 
ECHO - Add a RegexPreCompiler.exe.config file with
ECHO   "<configuration><startup>  <supportedRuntime version="v4.0.amd64chk" />  </startup></configuration>"
ECHO. 
ECHO - Do set COMPLUS_DEFAULTVERSION=v4.0.amd64chk
ECHO      set COMPLUS_VERSION=v4.0.amd64chk
ECHO   before running RegexPreCompiler and do
ECHO      set COMPLUS_DEFAULTVERSION=
ECHO      set COMPLUS_VERSION=
ECHO   afterwards.
ECHO. 
ECHO - Instead of running RebuildRegExILFile.cmd from a SUITE windows, run it from a ClrEnv window.
ECHO   This should have all the correct environment magic set up.

%IntermediateRootPath%\NDP_FX\regexprecompiler.csproj__1952437332\objc\x86\RegexPreCompiler.exe
ildasm.exe /NOBAR /out=System.Web.RegularExpressions.il System.Web.RegularExpressions.dll
rep ".assembly cil System.Web.RegularExpressions" ".assembly System.Web.RegularExpressions" System.Web.RegularExpressions.il
perl.exe ReplaceAssemblyVersionsInIL.pl System.Web.RegularExpressions.il System.Web.RegularExpressions.ilpp
del System.Web.RegularExpressions.il
del System.Web.RegularExpressions.dll
