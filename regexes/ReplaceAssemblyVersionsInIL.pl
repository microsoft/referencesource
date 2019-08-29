# Replace assembly versions and public key tokens in IL file.
# Creates .ilpp file from .il file.
#
# WARNING: Doesn't support .il files with custom attributes. They could contain assembly version
# Reports an error when custom attributes used.
# Custom attributes could contain assembly version (see ndp\fx\src\Designer\System\System.Design.asmmeta):
#    .custom instance void [System]System.ComponentModel.DefaultValueAttribute::.ctor(object) = 
#        (01 00 55 70 53 79 73 74 65 6D 2E 57 65 62 2E 55 49 2E 44 61 74 61 53 6F 75 72 63 65 4F 70 65 72 61 74 69 6F 6E 2C 20 53 79 73 74 65 6D 2E 57 65 62 2C 20 56 65 72 73 69 6F 6E 3D 32 2E 30 2E 30 2E 30 2C 20 43 75 6C 74 75 72 65 3D 6E 65 75 74 72 61 6C 2C 20 50 75 62 6C 69 63 4B 65 79 54 6F 6B 65 6E 3D 62 30 33 66 35 66 37 66 31 31 64 35 30 61 33 61 00 00 00 00 00 00)
# where sequence:
#        2C 20 56 65 72 73 69 6F 6E 3D 32 2E 30 2E 30 2E 30 2C
# represents:
#        ,     V  e  r  s  i  o  n  =  2  .  0  .  0  .  0  ,
#
# WARNING: .assembly must be well indented. See PrintUsage for an example.

my $parametersCount = @ARGV;
if ($parametersCount != 2) {
    PrintUsage();
    ReportError("Unexpected number of parameters.");
}

my ($sInputFileName, $sOutputFileName) = @ARGV;

open(InputFile, "<", $sInputFileName) or ReportError("Cannot open input file $sInputFileName");
open(OutputFile, ">", $sOutputFileName) or ReportError("Cannot open output file $sOutputFileName");

print(OutputFile "#include \"ndp40.versions.h\"\n");

my $iLineNumber = 0;
while(<InputFile>) {
    $iLineNumber++;
    if (/^\s*\.assembly/io) { # io = case insensitive, evaluate once
        if(/^\s*\.assembly\s+(extern\s+|)(\S+)(\s*(\/\/.*|))$/i) {
            my $fExternAssemblyReference = 0;
            my $sAssemblyName = $2;
            if ($1 =~ /extern/io) {
                $fExternAssemblyReference = 1;
            }
            if ($3 !~ /\s*/) {
                ReportError("Unexpected characters behind assembly name $2.\nSee line $iLineNumber: $_");
            }
            print(OutputFile);
            
            # Read assembly definition
            ReadAssemblyDefinition($sAssemblyName, \$iLineNumber, $fExternAssemblyReference);
            next;
        }
        else {
            ReportError("Unrecognized .assembly directive.\nSee line $iLineNumber: $_");
        }
    }
    if (/\.custom/io) {
        ReportError(".custom attributes are not supported.\nSee line $iLineNumber: $_");
    }
    if (/\.ver/io) {
        # We most probably finished .assembly directive parsing too early
        ReportError("Unexpected .ver directive. Input IL is most probably not well intended.\nSee line $iLineNumber: $_");
    }
    print(OutputFile);
}

exit 0;

sub ReadAssemblyDefinition
{
    my ($sAssemblyName, $iLineNumber, $fExternAssemblyReference) = @_;
    my $iStartLineNumber = ${$iLineNumber};
    
    my $sAssemblyMacro = uc($sAssemblyName) . "_ASSEMBLY_";
    $sAssemblyMacro =~ s/\./_/g;
    
    $_ = <InputFile>;
    ${$iLineNumber}++;
    
    # First line must start with { (at the beginning of the line) and may be followed by IL comment
    if ($_ !~ /^{\s*(|\/\/.*)$/o) {
        # Wrong line format
        if (/^\s+{\s*(|\/\/.*)$/o) {
            # { is preceeded by white spaces - unsupported .assembly indentation
            ReportError("'{' character must begin at the beginning of the line.\nSee line $iLineNumber: $_");
        }
        else {
            ReportError("Expected '{' character after .assembly directive start.\nSee line $iLineNumber: $_");
        }
    }
    print(OutputFile);
    
    my $fVersionParsed = 0;
    my $fPublicKeyTokenParsed = 0;
    
    while(<InputFile>) {
        ${$iLineNumber}++;

        # End of assembly definition
        if (/^}\s*(|\/\/.*)$/o) {
            if (!$fVersionParsed) {
                ReportAssemblyDefinitionWarning("Missing .ver directive", $sAssemblyName, $iStartLineNumber);
            }
            if (!$fPublicKeyTokenParsed && $fExternAssemblyReference) {
                ReportAssemblyDefinitionWarning("Missing .publickeytoken directive", $sAssemblyName, $iStartLineNumber);
            }
            print(OutputFile);
            return;
        }
        
        # Check end of assembly definition '}' not starting at the line start
        if (/^\s+}/io) {
            ReportAssemblyDefinitionWarning("End brace '}' not recognized as .assembly directive end at line ${$iLineNumber}", $sAssemblyName, $iStartLineNumber);
        }
        
        # .ver directive
        if (/^(\s*\.ver\s+)\S+(\s*(|\/\/.*))$/io) {
            if ($fVersionParsed) {
                ReportAssemblyDefinitionError("Duplicate .ver directive", $sAssemblyName, $iStartLineNumber);
            }
            $fVersionParsed = 1;
            print(OutputFile $1 . $sAssemblyMacro . "VERSION_IL" . $3 . "\n");
            next;
        }
        
        # .publickeytoken directive
        if (/^(\s*\.publickeytoken\s+=\s+)\(.*?\)(.*)$/io) {
            if ($fPublicKeyTokenParsed) {
                ReportAssemblyDefinitionError("Duplicate .publickeytoken directive", $sAssemblyName, $iStartLineNumber);
            }
            $fPublicKeyTokenParsed = 1;
            print(OutputFile $1 . $sAssemblyMacro . "PUBLIC_KEY_TOKEN_IL" . $2 . "\n");
            next;
        }
        
        # Double check that we didn't miss end of assembly definition
        if (/\.assembly/io) {
            ReportAssemblyDefinitionError("Unexpected .assembly directive at line ${$iLineNumber}", $sAssemblyName, $iStartLineNumber);
        }
        
        print(OutputFile);
    }
    
    ReportAssemblyDefinitionError("Unexpected end of file", $sAssemblyName, $iStartLineNumber);
}

sub ReportAssemblyDefinitionError
{
    my ($errorMessage, $sAssemblyName, $iStartLineNumber) = @_;
    ReportError("$errorMessage in '$sAssemblyName' definition starting at line $iStartLineNumber");
}
sub ReportAssemblyDefinitionWarning
{
    my ($warningMessage, $sAssemblyName, $iStartLineNumber) = @_;
    ReportWarning("$warningMessage in '$sAssemblyName' definition starting at line $iStartLineNumber");
}

sub ReportWarning
{
    my ($warningMessage) = @_;
    print(STDERR "WARNING: $warningMessage\n");
}

sub ReportError
{
    my ($errorMessage) = @_;
    print(STDERR "ERROR: $errorMessage\n");
    
    exit -1;
}

sub PrintUsage
{
    print("Replace assembly versions and public key tokens in IL file.\n");
    print("Usage:\n");
    print("       ReplaceAssemblyVersionsInIL.pl input.il output.ilpp\n");
    print("\n");
    print("Replaces .ver and .publickkeytoken directives inside .assembly directive with assembly name macros defined in public\\internal\\VSCommon\\inc\\ndp40.versions.h\n");
    print("Example:\n");
    print("   .assembly extern System\n");
    print("   {\n");
    print("     .publickeytoken = (B7 7A 5C 56 19 34 E0 89 )\n");
    print("     .ver 4:0:0:0\n");
    print("   }\n");
    print("replaces to:\n");
    print("   .assembly extern System\n");
    print("   {\n");
    print("     .publickeytoken = SYSTEM_ASSEMBLY_PUBLIC_KEY_TOKEN_IL\n");
    print("     .ver SYSTEM_ASSEMBLY_VERSION_IL\n");
    print("   }\n");
    print("\n");
    print("WARNING: Doesn't support custom attributes. See the script for more details.\n");
    print("WARNING: .assembly must be well indented.\n");
    print("\n");
}
