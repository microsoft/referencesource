//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File and directory path utilities.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//---------------------------------------------------------------------------
// Given a path, return one of the PathType constants.  A path is considered
// to be full if it starts with <any-char>:.  A path is a UNC path if it
// starts with \\.  A path is a File URL if it starts with file://.
// All other paths are considered to be relative paths.
//---------------------------------------------------------------------------
PathType PathGuessType(
    const WCHAR * wszPath,
    const WCHAR wchPathSeparator)
{
    if (wszPath == NULL)
    {
        return PathEmpty;
    }

    if (wszPath[0] == wchPathSeparator && wszPath[1] == wchPathSeparator)
    {
        return PathUNC;
    }
    else if (iswalpha(wszPath[0]) && wszPath[1] == L':')
    {
        return PathFull;
    }
    else if (_wcsnicmp(wszPath, L"file://", 7) == 0)
    {
        return PathFileURL;
    }
    else
    {
        return PathRelative;
    }
}

bool IsIllegalFileNameChar(const WCHAR wch)
{
    return (wch == L'\\' || wch == L'|' || wch == L'*' || wch == L'?' ||
        wch == L'<' || wch == L'>' || wch == L'/' || wch == L'[' || wch == L']' ||
        wch == L';' || wch == L'=' || wch == L',');
}

//---------------------------------------------------------------------------
// Return pointer to the name and extension portion of a path.
//---------------------------------------------------------------------------
WCHAR * PathFindName(
    const WCHAR * wszPath,
    const WCHAR wchPathSeparator)
{
    if (wszPath == NULL)
    {
        return NULL;
    }

    const WCHAR* wszFile = wszPath + wcslen(wszPath);

    while (wszFile > wszPath)
    {
        if (*wszFile == wchPathSeparator || IsIllegalFileNameChar(*wszFile))
        {
            wszFile++;
            break;
        }

        wszFile--;
    }

    return const_cast<WCHAR *>(wszFile);
}

//---------------------------------------------------------------------------
// Given a full or partial file / directory path, return a pointer to
// the extension portion of the filename.  This includes the period.
// If there is no extension, then the pointer is to the end of the
// string.
//   "c:\bar\foo.txt"      returns ".txt"
//   "c:\"          returns ""
//   "c:\foo"          returns ""
//   "c:\bar\foo.bar.txt" returns ".txt"
//   "c:\bar.foo\bar"      returns ""
//---------------------------------------------------------------------------
WCHAR * PathFindExt(
    const WCHAR * wszPath,
    const WCHAR wchPathSeparator,
    const WCHAR wchExtSeparator)
{
    if (wszPath == NULL)
    {
        return NULL;
    }

    const WCHAR *wszExt;
    const WCHAR* wszEnd = wszPath + wcslen(wszPath);

    for (wszExt = wszEnd; wszExt > wszPath; wszExt--)
    {
        if (*wszExt == wchExtSeparator)
        {
            break;
        }
        if (*wszExt == wchPathSeparator || IsIllegalFileNameChar(*wszExt))
        {
            wszExt = wszEnd;
            break;
        }
    }

    return const_cast<WCHAR *>(wszExt);
}

//---------------------------------------------------------------------------
// Given a directory or filename, add a directory or filename to the path.
// This is like WszCat, only it makes sure that a wchPathSeparator separator is added
// if necessary.  The return value is wszPath passed in.  For the general
// case, the wszPath buffer should be MAX_PATH * 2 characters.
//---------------------------------------------------------------------------
WCHAR * PathCatName(
    _Inout_opt_cap_(cchPathBuffer)_Prepost_count_(cchPathBuffer)WCHAR * wszPath,
    size_t cchPathBuffer,
    _In_z_ const WCHAR * wszName,
    const WCHAR wchPathSeparator)
{
    if (wszPath == NULL || wszName == NULL)
    {
        return NULL;
    }

    // Find end of path
    size_t cchPath = wcslen(wszPath);
    if (cchPath > 0 && wszPath[cchPath - 1] != wchPathSeparator)
    {
        wszPath[cchPath] = wchPathSeparator;
        wcscpy_s(wszPath + cchPath + 1, cchPathBuffer - (cchPath + 1), wszName);
    }
    else
    {
        wcscpy_s(wszPath + cchPath, cchPathBuffer - cchPath, wszName);
    }

    return wszPath;
}

//---------------------------------------------------------------------------
// Given a full or partial file / directory path, return a pointer to the
// the first character in the path that does not correspond to the device:
//   "c:\bar\foo.txt"   returns "\bar\foo.txt"
//   "c:\"        returns    "\"
//   "c:foo"        returns "foo"
//   "c:..\foo"        returns "..\foo"
//   "c:.."        returns ".."
//   "\\bar\foo"        returns ""
//   "\\bar\foo\----" returns "\----"
//   "bar\foo.txt"    returns "bar\foo.txt"
//   "foo.txt"        returns "foo.txt"
//   "file:///\\Server\Share\foo" returns "foo"
//   "file://c:\foo\" returns "c:\foo\"
//   "file://Server" returns ""
//   "file://Server/Share" returns ""
//   "file://Server/Share/foo/" returns "foo/"
//---------------------------------------------------------------------------
WCHAR * PathFindDevice(
    const WCHAR * wszPath,
    const WCHAR wchPathSeparator)
{
    if (wszPath == NULL)
    {
        return NULL;
    }

    while (iswspace(wszPath[0]))
    {
        ++wszPath;
    }

    PathType type = PathGuessType(wszPath, wchPathSeparator);

    switch (type)
    {
        case PathEmpty:
        {
            return NULL;
        }
        case PathUNC:
        {
            const WCHAR *wszServer = wcschr(wszPath + 2, wchPathSeparator);
            if (wszServer == NULL)
            {
                return const_cast<WCHAR *>(wszPath + wcslen(wszPath));    // returns ""
            }
            const WCHAR *wszShare = wcschr(wszServer + 1, wchPathSeparator);
            if (wszShare == NULL)
            {
                return const_cast<WCHAR *>(wszPath + wcslen(wszPath));    // returns ""
            }
            return const_cast<WCHAR*>(wszShare + 1);    // return first character after \\server\share
        }
        case PathFull:
        {
            return(WCHAR*)wszPath + 2;  // return first character after drive and colon
        }
        case PathFileURL:
        {
            wszPath = wszPath[7] == L'/' ? wszPath + 8 : wszPath + 7;
            if (wcschr(wszPath, L'/') == NULL)
            {
                return PathFindDevice(wszPath, wchPathSeparator);    // return the first character after stripping protocol and other possible device.  sample input file:///\\Server\Share\foo\... or file://c:\foo\... returns foo\... or c:\foo\... respectively
            }
            else
            {
                if (iswalpha(*wszPath) && wszPath[1] == L':' && (wszPath[2]==L'/' || wszPath[2]==wchPathSeparator))
                {
                    return const_cast<WCHAR *>(wszPath);    // return the first character after stripping protocol.  sample input file://c:/foo/... returns c:/foo/...
                }

                const WCHAR *wszServer = wcschr(wszPath, L'/');
                if (wszServer == NULL)
                {
                    return const_cast<WCHAR *>(wszPath + wcslen(wszPath));    // returns ""
                }
                const WCHAR *wszShare = wcschr(wszServer + 1, L'/');
                if (wszShare == NULL)
                {
                    return const_cast<WCHAR *>(wszPath + wcslen(wszPath));    // returns ""
                }
                return const_cast<WCHAR *>(wszShare + 1);    // return the first character after stripping protocol and computer share device.  sample input file://Server/Share/foo/... returns foo/...
            }
        }
        default:
        {
            return(WCHAR*)wszPath;   // return existing path since it's relative
        }
    }
}

//---------------------------------------------------------------------------
// Given a path pointing to a file or directory, shorten the path string to
// to be just the path of the parent directory.  If there is no parent
// directory, then it has no effect on the buffer.  The pointer passed
// in is returned.  In no cases will the truncated path have a wchPathSeparator at
// the end of it. For example:
//
//    "c:\a\b\c"  returns "c:\a\b"
//    "\\a\\b"      returns "\\a\\b"
//    "\\a\\b\c"  returns "\\a\\b"
//    "c:\a\b"      returns "c:\a"
//    "c:\"      returns "c:
//    "c:\a"      returns "c:
//
// This returns whether or not the buffer was modified.
//---------------------------------------------------------------------------
BOOL PathMakeDir(
    _Inout_opt_z_ WCHAR * wszPath,
    const WCHAR wchPathSeparator)
{
    if (wszPath == NULL)
    {
        return FALSE;
    }

    // Find the trailing file or dir name, backup to backslash preceeding it
    WCHAR* wszFileName = PathFindName(wszPath, wchPathSeparator);
    if (wszFileName == NULL)
    {
        return FALSE;
    }

    // Find the start of the leading dir (after the device)
    WCHAR* wszDir = PathFindDevice(wszPath, wchPathSeparator);
    if (wszDir == NULL)
    {
        return FALSE;
    }

    // If the name is not part of the device name
    if (wszFileName > wszDir)
    {
        --wszFileName;

        // Then truncate the path to be only the directory
        if (*wszFileName == wchPathSeparator)
        {
            *wszFileName = L'\0';
            // We modified the buffer
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    // We did not modify the buffer.
    return FALSE;
}

//---------------------------------------------------------------------------
// Takes the (possibly-qualified) file name in pwszFileName and puts a canonical version
// of the file name (no path, all lowercase) into pwszCanonicalBuf, which must be at
// least cch Unicode characters in size.  The function modifies and returns pwszCanonicalBuf.
//---------------------------------------------------------------------------
WCHAR * GetCanonicalFileName(
    _In_opt_z_ const WCHAR * pwszFileName,
    _Out_opt_cap_(cch)WCHAR * pwszCanonicalBuf,
    unsigned cch,
    const WCHAR wchPathSeparator)
{
    if (pwszFileName == NULL || pwszCanonicalBuf == NULL)
    {
        return NULL;
    }

    WCHAR *pwszShortName = PathFindName(pwszFileName, wchPathSeparator);

    if (wcslen(pwszShortName) >= cch)
    {
        VbThrow(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    }

    wcscpy_s(pwszCanonicalBuf, cch, pwszShortName);
    FilenameToLowerCaseInPlace(pwszCanonicalBuf);
    return pwszCanonicalBuf;
}

unsigned VB_W_GetCommandLine(
    _Out_opt_cap_(SizeOfBuffer)WCHAR * pwszBufferForCommandLine,
    unsigned SizeOfBuffer)
{
    unsigned SizeInWideChars = 0;

    LPWSTR pwszCommandLine;
    LPSTR pszCommandLine;

    if (W_IsUnicodeSystem())
    {
        pwszCommandLine = GetCommandLineW();
        SizeInWideChars = (unsigned)(pwszCommandLine ? wcslen(pwszCommandLine) + 1 : 0);
    }
    else
    {
        pszCommandLine = GetCommandLineA();

        SizeInWideChars = pszCommandLine ?
#ifndef FEATURE_CORESYSTEM
                            MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, pszCommandLine, -1, NULL, 0):
#else
                            MultiByteToWideChar(CP_ACP, 0, pszCommandLine, -1, NULL, 0):
#endif
                            0;
    }

    if (SizeOfBuffer == 0)
    {
        return SizeInWideChars;
    }
    else
    {
        unsigned SizeToCopy = SizeInWideChars > SizeOfBuffer ?
                                SizeOfBuffer :
                                SizeInWideChars;

#pragma warning (push)
#pragma warning (disable:6001 6054) // pszCommandLine is set before its use
        if (W_IsUnicodeSystem())
        {
            if (SizeToCopy == 0)
            {
                pwszBufferForCommandLine[0] = 0;
            }
            else
            {
#pragma warning (push)
#pragma warning (disable:6387) // pwszBufferForCommandLine would only be NULL if SizeToCopy is 0
                wcsncpy_s(pwszBufferForCommandLine, SizeOfBuffer, pwszCommandLine, SizeToCopy - 1);
#pragma warning (pop)
            }
            return SizeToCopy;
        }
        else
        {
#ifndef FEATURE_CORESYSTEM
            return MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, pszCommandLine, -1, pwszBufferForCommandLine, SizeToCopy);
#else
            return MultiByteToWideChar(CP_ACP, 0, pszCommandLine, -1, pwszBufferForCommandLine, SizeToCopy);
#endif
        }
#pragma warning (pop)
    }

    return SizeInWideChars;
}
