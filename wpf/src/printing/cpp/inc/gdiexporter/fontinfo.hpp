/**************************************************************************
*
* Copyright (c) 2006 Microsoft Corporation
*
* Module Name:
*
*   GDI font installation information and management.
*
* Created:
*
*   4/21/2006 bnguyen
*      Created it.
*
**************************************************************************/

ref class FontInfo;

//
// Wraps a font stream source (file Uri or GlyphTypeface) and allows comparing
// font streams to determine if two fonts are the same.
//
// Stream length is cached to avoid reopening the stream, and is only updated
// via explicit UpdateStreamLength() call.
//
// Close() must be called when context becomes unused to close the underlying stream
// if it was opened.
//
ref struct FontStreamContext
{
// Constants
public:
    // Maximum font size we'll process.
    static const int MaximumStreamLength = Int32::MaxValue;

private:
    // We compare font files CompareLength bytes at a time.
    static const int CompareLength = 65535;

// Constructors
public:
    FontStreamContext(GlyphTypeface^ source);

    /// <SecurityNote>
    /// Critical    - Stores font location and stream information
    /// </SecurityNote>
    [SecurityCritical]
    FontStreamContext(Uri^ source, int streamLength);

// Public Properties
public:
    // Length of font stream.
    property int StreamLength
    {
        /// <SecurityNote>
        /// Critical    - Returns information describing the font stream
        /// </SecurityNote>
        [SecurityCritical]
        int get() { return _streamLength; }
    }

// Public Methods
public:
    /// <SecurityNote>
    /// Critical    - Contains font stream and information describing the font stream
    /// TreatAsSafe - Simply closes the font stream; no information leak
    /// </SecurityNote>
    /// <Remarks>
    /// Closes the underlying stream if it's open.
    /// </Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    void Close();

    /// <SecurityNote>
    /// Critical    - Returns font stream
    /// </SecurityNote>
    /// <Remarks>
    /// Gets font stream, opening it if necessary.
    /// May return nullptr if unable to open stream.
    /// Stream position guaranteed to be at zero.
    /// </Remarks>
    [SecurityCritical]
    Stream^ GetStream();

    /// <SecurityNote>
    /// Critical    - Accesses font stream
    /// TreatAsSafe - Updates internal stream length field; no information leak
    /// </SecurityNote>
    /// <Remarks>
    /// Updates length information, opening the font stream if necessary.
    /// </Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    void UpdateStreamLength();

    /// <SecurityNote>
    /// Critical    - Accesses font stream
    /// TreatAsSafe - Returns only bool indicating if two streams are the same; no critical information leak
    /// </SecurityNote>
    /// <Remarks>
    /// Determines if two font streams are the same, comparing length and first CompareLength
    /// bytes if necessary.
    /// </Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    bool Equals(FontStreamContext %otherContext);

// Private Fields
private:
    GlyphTypeface^ _sourceTypeface;

    /// <SecurityNote>
    /// Critical    - Font location
    /// </SecurityNote>
    [SecurityCritical]
    Uri^ _sourceUri;

    /// <SecurityNote>
    /// Critical    - Font stream
    /// </SecurityNote>
    [SecurityCritical]
    Stream^ _stream;

    /// <SecurityNote>
    /// Critical    - Partially describes font stream
    /// </SecurityNote>
    [SecurityCritical]
    int _streamLength;
};

//
// Describes a font installation instance, either an existing system installation of the font or
// a private install during the course of printing glyphs.
//
// Allows comparing two font installations to determine if the fonts are the same.
// Can install/uninstall the font from GDI.
//
ref class FontInstallInfo
{
public:
    /// <SecurityNote>
    /// Critical    - Stores critical font path
    /// </SecurityNote>
    [SecurityCritical]
    FontInstallInfo(Uri^ uri);

// Public Methods
public:
    /// <SecurityNote>
    /// Critical    - Uses critical stream context, touches critical stream information (length, hash)
    /// TreatAsSafe - No leak of critical information
    /// </SecurityNote>
    /// <Remarks>
    /// Determines if two font installations refer to the same font, given stream context of this installation.
    /// </Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    bool Equals(FontStreamContext% context, FontInstallInfo^ otherFont);

    /// <SecurityNote>
    /// Critical    - Calls native methods, uses critical stream context, returns critical installation data
    /// </SecurityNote>
    /// <Remarks>
    /// Installs GDI font via AddFont*ResourceEx.
    ///
    /// Returns either null (installation failed), string (font filename; installed from file),
    /// or GDI install handle (installed from memory). Returns new font family name via newFamilyName
    /// </Remarks>
    [SecurityCritical]
    Object^ Install(FontStreamContext% context, String^ % newFamilyName, unsigned faceIndex);

    /// <SecurityNote>
    /// Critical    - Calls native methods, takes in critical data
    /// </SecurityNote>
    /// <Remarks>
    /// Uninstalls GDI font via RemoveFont*ResourceEx.
    ///
    /// installHandle is either a string (file to uninstall) or GdiFontResourceSafeHandle (handle to
    /// uninstall font from memory).
    /// </Remarks>
    [SecurityCritical]
    void Uninstall(Object^ installHandle);

// Private Fields
private:
    /// <SecurityNote>
    /// Critical    - Critical font path
    /// </SecurityNote>
    [SecurityCritical]
    Uri^ _uri;

    /// <SecurityNote>
    /// Critical    - Describes font stream
    /// </SecurityNote>
    [SecurityCritical]
    int _streamLength;

// Private Methods
private:
    /// <SecurityNote>
    /// Critical    - Touches critical font stream data
    /// TreatAsSafe - Stores into critical fields, no critical information leak
    /// </SecurityNote>
    /// <Remarks>
    /// Caches font stream information to speed up future font stream comparisons.
    /// </Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    void UpdateFromContext(FontStreamContext% context);
};

//
// Stores information to track the status of a font used to print a document. Each FontInfo
// corresponds to a font with a particular name.
//
// The system may have a font installed with this name, in which case _systemInstall != nullptr.
// The font used can be overridden by installing a private font; _privateInstall != nullptr in
// that case.
//
ref class FontInfo
{
public:
    // Constructs FontInfo that describes no installed font with this name.
    FontInfo();

    /// <SecurityNote>
    /// Critical    - Stores critical information
    /// </SecurityNote>
    /// <Remarks>Constructs FontInfo that describes a system-installed font.</Remarks>
    [SecurityCritical]
    FontInfo(Uri^ systemUri);

// Public Methods
public:
    /// <SecurityNote>
    /// Critical    - Calls critical GetFontUri, performs critical font installation
    /// TreatAsSafe - GDI font installation considered trusted, no critical information leak
    /// </SecurityNote>
    /// <Remarks>
    /// Prepares GDI to render glyphs using GlyphTypeface by installing GDI fonts or
    /// verifying that the currently installed GDI font matches GlyphTypeface.
    ///
    /// Returns false if GDI could not be prepared. In such an event, should fallback
    /// to filling glyph geometry.
    /// </Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    bool UsePrivate(GlyphTypeface^ typeface);

    /// <SecurityNote>
    /// Critical    - Touches critical install handle
    /// TreatAsSafe - No critical information leaked
    /// </SecurityNote>
    /// <Remarks>Uninstalls the private font if one was installed.</Remarks>
    [SecurityCritical, SecurityTreatAsSafe]
    void UninstallPrivate();

    property String^ NewFamilyName
    {
        String^ get()
        {
            return _newFamilyName;
        }
    }

// Private Fields
private:
    FontInstallInfo^ _systemInstall;
    FontInstallInfo^ _privateInstall;
    String^          _newFamilyName;    // New 'unique' font family name to avoid name conflict. 
                                        // Should be valid when _privateInstall has value

    /// <SecurityNote>
    /// Critical    - Contains critical font path or font handle
    /// </SecurityNote>
    [SecurityCritical]
    Object^ _privateInstallHandle;
};
