//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation for location and line marker tables.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#define COMPARE_BEGIN true
#define COMPARE_END   false

void Location::SetStart(const Location * pNewStartLocation)
{
    AssertIfNull(pNewStartLocation);

    m_lBegLine = pNewStartLocation->m_lBegLine;
    m_lBegColumn = pNewStartLocation->m_lBegColumn;
}

void Location::SetStart(
    long iLine,
    long iColumn)
{
    m_lBegLine = iLine;
    m_lBegColumn = iColumn;
}

void Location::SetEnd(const Location * pNewEndLocation)
{
    AssertIfNull(pNewEndLocation);

    m_lEndLine = pNewEndLocation->m_lEndLine;
    m_lEndColumn = pNewEndLocation->m_lEndColumn;
}

void Location::SetEnd(
    long iLine,
    long iColumn)
{
    m_lEndLine = iLine;
    m_lEndColumn = iColumn;
}

bool Location::Expand(const Location * pLocation)
{
    AssertIfNull(pLocation);

    bool LocationChanged = false;

    if (pLocation->StartsBefore(this))
    {
        SetStart(pLocation);
        LocationChanged = true;
    }

    if (pLocation->EndsAfter(this))
    {
        SetEnd(pLocation);
        LocationChanged = true;
    }

    return LocationChanged;
}

//============================================================================
// Set this location to be the location of the tree.
//============================================================================

void Location::SetLocation(ILTree::ILNode * ptree)
{
    *this = ptree->Loc;
}

void Location::SetLocationToToken( const Token * pToken )
{
    AssertIfNull(pToken);

    if (pToken)
    {
        *this = pToken->GetLocation();
    }
}

//-------------------------------------------------------------------------------------------------
//
// Set the location to point to the start of the Token
//
//-------------------------------------------------------------------------------------------------
void
Location::SetLocationToTokenStart( _In_ const Token *pToken )
{
    ThrowIfNull(pToken);

    m_lBegLine = pToken->m_StartLine;
    m_lEndLine = pToken->m_StartLine;
    m_lBegColumn = pToken->m_StartColumn;
    m_lEndColumn = pToken->m_StartColumn;
}

//============================================================================
// Set this location to be a copy of another location
//============================================================================

void Location::SetLocation
(
    _In_ const Location * pNewLocation // the location to copy
)
{
    VSASSERT( pNewLocation, "You can't pass NULL to SetLocation( Location *)");
    *this = *pNewLocation;
}

//============================================================================
// Set the location to point to the beginning of a specific line.
//============================================================================

void Location::SetLocationToLine(long iLine)
{
    m_lBegLine = m_lEndLine = iLine;
    m_lBegColumn = m_lEndColumn = 0;
}

//============================================================================
// Set the location to the specified line/column.
//============================================================================

void Location::SetLocation(
    long iLine,
    long iColumn)
{
    m_lBegLine = m_lEndLine = iLine;
    m_lBegColumn = m_lEndColumn = iColumn;
}

void Location::SetLocation(
    long BegLine,
    const ParseTree::PunctuatorLocation * pPunctuator,
    long Width)
{
    AssertIfNull(pPunctuator);

    if (pPunctuator)
    {
        m_lBegLine = BegLine + pPunctuator->Line;
        m_lBegColumn = pPunctuator->Column;
        m_lEndLine = m_lBegLine;
        m_lEndColumn = m_lBegColumn + Width;
    }
}

//============================================================================
// Set the location to the specified line/column.
//============================================================================

void Location::SetLocationToHidden()
{
    m_lBegLine = m_lEndLine = HIDDEN_LINE_INDICATOR;
    m_lBegColumn = m_lEndColumn = 0;
}

Location Location::GetHiddenLocation()
{
    Location Loc;
    Loc.SetLocationToHidden();

#pragma warning (push)
#pragma warning (disable:6001) // Location is initialized above
    return Loc;
#pragma warning (pop)
}

Location Location::GetInvalidLocation()
{
    Location Loc;
    Loc.Invalidate();
#pragma warning (push)
#pragma warning (disable:6001) // Location is initialized above
    return Loc;
#pragma warning (pop)
}


//============================================================================
// Constructor
//============================================================================

LineMarkerTable::LineMarkerTable(SourceFile * pSourceFile) 
{
#if IDE
    m_fPreNotified = false;
    m_pSourceFile = pSourceFile;
#endif
}

#if IDE

void LineMarkerTable::AddOutOfProcBuildErrorLocation
(
    ErrorLocation* pErrorLocation
)
{
#if DEBUG
    bool inserted = m_outOfProcBuildErrorLocations.insert(pErrorLocation).second;
    ThrowIfFalse(inserted);
#else
    m_outOfProcBuildErrorLocations.push_back(pErrorLocation);
#endif

    pErrorLocation->m_locType = OutOfProcBuildErrorLoc;
    pErrorLocation->m_state = CS_NoState;     // Set to a default value (not used for this error type)
    pErrorLocation->m_fForceRemove = false;   // Set to a default value (not used for this error type)
}

void LineMarkerTable::RemoveInvalidOutOfProcBuildErrorLocations()
{
    CompilerIdeLock lock(m_lock);
    RemoveInvalidErrorLocations(&m_outOfProcBuildErrorLocations);
}

void LineMarkerTable::RemoveOutOfProcBuildErrorLocations()
{
    CompilerIdeLock lock(m_lock);
    m_outOfProcBuildErrorLocations.clear();
}

//============================================================================
// Add an error location to the list.
//============================================================================

void LineMarkerTable::AddErrorLocation(
    _Out_ ErrorLocation * pErrorLocation,
    CompilationState state)
{
    ThrowIfNull(pErrorLocation);
    CompilerIdeLock lock(m_lock);

#if DEBUG
    bool inserted = m_errorLocations.insert(pErrorLocation).second;
    ThrowIfFalse(inserted);
#else
    m_errorLocations.push_back(pErrorLocation);
#endif

    pErrorLocation->m_locType = ErrorLoc;
    pErrorLocation->m_state = (state > CS_Bound) ? CS_Bound : CS_NoState;
    pErrorLocation->m_fForceRemove = false;
}

//============================================================================
// Remove all of the invalid locations from the error location list.
//============================================================================

void LineMarkerTable::RemoveInvalidErrorLocations()
{
    CompilerIdeLock lock(m_lock);
    RemoveInvalidErrorLocations(&m_errorLocations);
}

//============================================================================
// Add a decl location to the list.
//============================================================================

void LineMarkerTable::AddDeclLocation(
    _Out_ TrackedLocation *ploc,
    BCSYM *pSymbolForLocation
)
{
    ThrowIfNull(ploc);
    CompilerIdeLock lock(m_lock);
    ploc->m_pSymbolForLocation = pSymbolForLocation;

#if DEBUG
    bool inserted = m_declLocations.insert(ploc).second;
    ThrowIfFalse(inserted);
#else
    m_declLocations.push_back(ploc);
#endif

    ploc->m_locType = TrackedLoc;
}

//============================================================================
// Add a code block definition to the list.
//============================================================================

void LineMarkerTable::AddCodeBlock(
    TrackedCodeBlock *pblock,
    BCSYM *pSymbolForLocation
)
{
    ThrowIfNull(pblock);
    CompilerIdeLock lock(m_lock);
    pblock->m_pSymbolForLocation = pSymbolForLocation;
    
#if DEBUG
    bool inserted = m_trackedCodeBlockLocations.insert(pblock).second;
    ThrowIfFalse(inserted);
#else
    m_trackedCodeBlockLocations.push_back(pblock);
#endif
    pblock->m_locType = CodeBlockLoc;
}

//============================================================================
// Remove all locations from the decl list.
//============================================================================

void LineMarkerTable::RemoveAllDeclLocations()
{
    CompilerIdeLock lock(m_lock);
    m_declLocations.clear();
    m_trackedCodeBlockLocations.clear();
}


//============================================================================
// Remove invalid error locations from the provided list
//============================================================================
void LineMarkerTable::RemoveInvalidErrorLocations
(
    TrackedLocations<ErrorLocation *>::Type * errorLocations
)
{
    CompilerIdeLock lock(m_lock);
    TrackedLocations<ErrorLocation *>::Type validLocations;
    for (auto it = errorLocations->begin(); it != errorLocations->end(); ++it)
    {
        if ((*it)->IsInvalid() || (*it)->m_fForceRemove)
        {
            continue;
        }
#if DEBUG
        validLocations.insert(*it);
#else
        validLocations.push_back(*it);
#endif
    }

    *errorLocations = validLocations;
}


//============================================================================
// Before the edit, get the offset for the tracked locations
//============================================================================
void LineMarkerTable::PreNotifyOfEdit(_In_ VBTextBuffer * pVBTextBuffer, _In_ long lStartLine)
{
    CompilerIdeLock lock(m_lock);
    // VSW#147887
    // Make sure we don't get the offsets again if we already have them
    if (pVBTextBuffer && !m_fPreNotified)
    {
        CacheTrackedLocationBufferOffsets<ErrorLocation>(m_outOfProcBuildErrorLocations, pVBTextBuffer, lStartLine);
        CacheTrackedLocationBufferOffsets<ErrorLocation>(m_errorLocations, pVBTextBuffer, lStartLine);
        CacheTrackedLocationBufferOffsets<TrackedLocation>(m_trackedCodeBlockLocations, pVBTextBuffer, lStartLine);
        CacheTrackedLocationBufferOffsets<TrackedLocation>(m_declLocations, pVBTextBuffer, lStartLine);

        m_fPreNotified = true;
    }
}

template <typename T>
void LineMarkerTable::CacheTrackedLocationBufferOffsets(
    _In_ const typename TrackedLocations<T*>::Type & trackedLocations, 
    _In_ VBTextBuffer* pVBTextBuffer, 
    _In_ long lStartLine)
{
    TemplateUtil::CompileAssertIsChildOf<TrackedLocation,T>();
    CompilerIdeLock lock(m_lock);

    TrackedLocation *ploc;
    long oBegin,oEnd;

    for (auto it = trackedLocations.begin(); it != trackedLocations.end(); ++it)
    {
        ploc = *it;
        oBegin = oEnd = 0;
        if (ploc->IsInvalid())
        {
            continue;
        }
        // ignore any locations before the edit
        if (ploc->m_lEndLine < lStartLine && ploc->IsValidOffsets())
        {
            continue;
        }
        if (SUCCEEDED(pVBTextBuffer->GetPositionOfLineIndex(ploc->m_lBegLine, ploc->m_lBegColumn, &oBegin)) &&
            SUCCEEDED(pVBTextBuffer->GetPositionOfLineIndex(ploc->m_lEndLine, ploc->m_lEndColumn, &oEnd)))
        {
            ploc->m_oBegin = oBegin;
            ploc->m_oEnd = oEnd;
        }
        else
        {
           ploc->Invalidate();
        }
    }
}

//============================================================================
// Update the tracked locations
//============================================================================
void LineMarkerTable::PostNotifyOfEdit(
    VBTextBuffer * pVBTextBuffer,
    EditInfoLists * pEditLists,
    EditCache SingleEdit,
    bool LastEdit)
{
    VSASSERT(SingleEdit.oAdjust != 0, L"oAdjust should never be 0 inside LineMarkerTable::PostNotifyOfEdit()");

    CompilerIdeLock lock(m_lock);

    // We need to refresh the task list if an error location changes
    if (m_pSourceFile->GetProject() &&
        m_pSourceFile->GetProject()->GetCompilerTaskProvider() &&
        GetCompilerPackage())
    {
        bool fUpdatedErrors = UpdateTrackedOffsetsAndLocations<ErrorLocation>(m_errorLocations, pVBTextBuffer, pEditLists, SingleEdit, LastEdit);
        bool fUpdatedOutOfProcBuildErrors = UpdateTrackedOffsetsAndLocations<ErrorLocation>(m_outOfProcBuildErrorLocations, pVBTextBuffer, pEditLists, SingleEdit, LastEdit);

        if (fUpdatedOutOfProcBuildErrors)
        {
            m_pSourceFile->SignalOutOfProcBuildErrorLocationsUpdated();
        }

        if (fUpdatedErrors || fUpdatedOutOfProcBuildErrors)
        {
            m_pSourceFile->GetProject()->GetCompilerTaskProvider()->TaskListNeedsRefresh();
            GetCompilerSharedState()->RefreshTaskList();
        }
    }

    // Stop tracking errors we no longer care about (IsInvalid or the force remove flag is set)
    if (LastEdit)
    {
        RemoveInvalidErrorLocations();
    }

    UpdateTrackedOffsetsAndLocations<TrackedLocation>(m_declLocations, pVBTextBuffer, pEditLists, SingleEdit, LastEdit);
    UpdateTrackedOffsetsAndLocations<TrackedLocation>(m_trackedCodeBlockLocations, pVBTextBuffer, pEditLists, SingleEdit, LastEdit);

    m_fPreNotified = false;
}

//
// Determine if the edit position given lies before the tracked location
// This is used to determine if a tracked location is affected by an edit
//
bool LineMarkerTable::IsEditBeforeLocation(
    long lEditPos,
    TrackedLocation * ploc,
    bool fCompareBegin)
{
    bool fIsEditBeforeLocation = false;

    // Grab the portion of the TrackedLocation we want to compare against
    long lLoc = fCompareBegin ? ploc->m_oBegin : ploc->m_oEnd;

    if (ploc->m_locType == CodeBlockLoc)
    {
        // Non-zero length code blocks get special treatment
        if (static_cast<TrackedCodeBlock *>(ploc)->fProcBlock)
        {
                // The tracked location for a proc block looks like:
                // |Sub foo()
                //  .....
                //  End Sub|
                //
                // The end of the tracked location is one character AFTER the 'b'
                // in 'End Sub'.  Typing there should NOT move the location.
                fIsEditBeforeLocation = fCompareBegin ? lEditPos <= lLoc : lEditPos < lLoc;
        }
        else
        {
            // The tracked location for this case (method body) looks like:
            // Sub foo()|
            // .....
            // |End Sub
            //
            // The start of the tracked location is one character AFTER the ')'
            // in 'Sub foo'.  Typing there should NOT move the location.
            fIsEditBeforeLocation = fCompareBegin ? lEditPos < lLoc : lEditPos <= lLoc;
        }
    }
    else
    {
        fIsEditBeforeLocation = (lEditPos <= lLoc);
    }

    return fIsEditBeforeLocation;
}

template <typename T>
bool LineMarkerTable::UpdateTrackedOffsetsAndLocations(
    _In_ const typename TrackedLocations<T*>::Type & trackedLocations, 
    VBTextBuffer * pVBTextBuffer,
    EditInfoLists * pEditLists,
    EditCache SingleEdit,
    bool LastEdit)
{
    TemplateUtil::CompileAssertIsChildOf<TrackedLocation,T>();
    CompilerIdeLock lock(m_lock);
    TrackedLocation *ploc;

    // Tells us whether any of the tracked locations changed as part of this update
    bool fLocationsChanged = false;

#if DEBUG
    TrackedLocation locCache;
#endif

    // Here is a list of bugs that we've had in previous code, we should regress against these
    // bugs when changes are made here.
    // VS#302006, VS#305056, VS#305847, VS#243392, VS#180674, VS#186022, VS#238616, VS#192462,
    // VS#262018, VS#544586, VS#543103, VS#545446, VS#545457, VSW#47521

    // Walk all edits and apply each one at a time.
    for (auto it = trackedLocations.begin(); it != trackedLocations.end(); ++it)
    {
        ploc = *it;
#if DEBUG
        locCache = *ploc;
#endif

        // Cache the length of the location
        long lLocLength = ploc->m_oEnd - ploc->m_oBegin;

        if (!ploc->IsValidOffsets())
        {
            goto CommitChange;
        }

        long oEnd = SingleEdit.iStartPos + SingleEdit.oAdjust;

        // CASE1: if this is a replace and if the location is inside where the edit happened,
        // then just go ahead and move the location to the new replace place.
        if (SingleEdit.IsReplace)
        {
            if (ploc->IsThisLocationWithinEdit(SingleEdit))
            {
                ploc->MoveLocation(SingleEdit);
                ploc->fIsChanged = true;
                goto CommitChange;
            }
        }

        // CASE2: We are deleting the begin, end, or both locations, in that case, we need to invalidate the location(s)
        // Note that this can only be called after we've checked CASE1 above, otherwise, we may be invalidating locations
        // that should be moved instead.
        if (SingleEdit.oAdjust < 0)
        {
            VSASSERT(oEnd < SingleEdit.iStartPos, "How can oEnd not be before iStartPos for a deletion?!");

            // Note: For deletions, oEnd is before SingleEdit.iStartPos
            //
            // Check to see if the edit deleted the begin and/or end of the location ploc
            if ((IsEditBeforeLocation(oEnd, ploc, COMPARE_BEGIN) && !IsEditBeforeLocation(SingleEdit.iStartPos, ploc, COMPARE_BEGIN)) ||
                (IsEditBeforeLocation(oEnd, ploc, COMPARE_END) && !IsEditBeforeLocation(SingleEdit.iStartPos, ploc, COMPARE_END)))
            {
                ploc->InvalidateOffsets();
                ploc->fIsChanged = true;

                // Out of proc build error location changes cannot trigger decompilation
                if (pEditLists && ploc->m_locType != OutOfProcBuildErrorLoc)
                {
                    pEditLists->SetInvalidateFile(
                        ploc->m_locType == ErrorLoc ?
                        static_cast<ErrorLocation *>(ploc)->m_state :
                        CS_NoState);
                }

                goto CommitChange;
            }
        }

        // CASE3: Edit before begin location, just update begin location.
        if (IsEditBeforeLocation(SingleEdit.iStartPos, ploc, COMPARE_BEGIN))
        {
            ploc->m_oBegin += SingleEdit.oAdjust;
            ploc->fIsChanged = true;

            VSASSERT(ploc->m_oBegin >= 0, L"Invalid Begin offset");
        }

        // CASE4: Edit before end location, just update end location
        if (IsEditBeforeLocation(SingleEdit.iStartPos, ploc, COMPARE_END))
        {
            ploc->m_oEnd += SingleEdit.oAdjust;
            ploc->fIsChanged = true;

            VSASSERT(ploc->m_oEnd >= 0, L"Invalid End offset");
        }

        // CASE5: We are in a repalce and we are repalcing before the begin or the end locations,
        // in these cases, we need to adjust the locations as needed.
        if (SingleEdit.IsReplace)
        {
            if (IsEditBeforeLocation(SingleEdit.iReplacePos, ploc, COMPARE_BEGIN))
            {
                ploc->m_oBegin -= SingleEdit.oAdjust;
                ploc->fIsChanged = true;

                VSASSERT(ploc->m_oBegin >= 0, L"Invalid Begin offset");
            }

            if (IsEditBeforeLocation(SingleEdit.iReplacePos, ploc, COMPARE_END))
            {
                ploc->m_oEnd -= SingleEdit.oAdjust;
                ploc->fIsChanged = true;

                VSASSERT(ploc->m_oEnd >= 0, L"Invalid End offset");
            }
        }

        // VSW#56950, VSW#58017, VSW#60451
        // If an edit modifies the size of an error location, the error needs to be
        // reported again.  This scenario is hit when the user edits squiggled text.
        if (ploc->m_locType == ErrorLoc && (ploc->m_oEnd - ploc->m_oBegin != lLocLength))
        {
            if (pEditLists)
            {
                pEditLists->SetInvalidateFile(static_cast<ErrorLocation *>(ploc)->m_state);
            }

            ploc->InvalidateOffsets();
        }

CommitChange:

        if (LastEdit)
        {
            if (ploc->fIsChanged == true)
            {
                // VSW#51592
                // For non-error locations, we always update the location.
                // For error locations, we stick to the last valid location by
                // preventing the line/col members from being updated.
                if (ploc->m_locType != ErrorLoc || ploc->IsValidOffsets())
                {
                    fLocationsChanged = true;
                    UpdateTrackedLocation(pVBTextBuffer, ploc);
                }
                else
                {
                    // Make sure the error location will be removed from the list
                    static_cast<ErrorLocation *>(ploc)->m_fForceRemove = true;
                }

                ploc->fIsChanged = false;
            }
#if DEBUG
            else
            {
                if (VBFTESTSWITCH(fDebugLineMark))
                {
                    DebPrintf("Location was not moved  ([%3d, %3d] (%3d, %3d),(%3d, %3d))\n",
                        ploc->m_oBegin, ploc->m_oEnd,  ploc->m_lBegLine, ploc->m_lBegColumn, ploc->m_lEndLine, ploc->m_lEndColumn);
                }
            }
#endif DEBUG
        }
    }

    return fLocationsChanged;
}

void LineMarkerTable::UpdateTrackedLocation(
    VBTextBuffer * pVBTextBuffer,
    TrackedLocation * ploc)
{
    VSASSERT(pVBTextBuffer && ploc, L"Bad input");
    VSASSERT(ploc->fIsChanged, L"LineMarkerTable::UpdateTrackedLocation() only takes locations that have been changed!");

    TrackedLocation locCache = *ploc;

    // Here is a list of bugs that we've had in previous code, we should regress against these
    // bugs when changes are made here.
    // VS#302006, VS#305056, VS#305847, VS#243392, VS#180674, VS#186022, VS#238616, VS#192462, VS#262018, VS#544586, VS#543103, VS#545446, VS#545457

    VSASSERT(ploc->m_oBegin >= 0, L"Invalid Begin offset");
    VSASSERT(ploc->m_oEnd >= ploc->m_oBegin, L"Invalid End offset");

    if (ploc->m_oBegin == 0)
    {
        ploc->m_lBegLine   = 0;
        ploc->m_lBegColumn = 0;
    }
    else
    {
        ploc->UpdateStartLineAndColumn(pVBTextBuffer);
    }

    if (ploc->m_oEnd == 0)
    {
        ploc->m_lEndLine   = 0;
        ploc->m_lEndColumn = 0;
    }
    else
    {
        ploc->UpdateEndLineAndColumn(pVBTextBuffer);
    }

    // And Update the symbols hash table as needed too.
#if DEBUG
    ploc->DebCheck();   // Make sure that the lines and columns are all good.

    if (VBFTESTSWITCH(fDebugLineMark))
    {
        DebPrintf("Location was moved From ((%3d, %3d),(%3d, %3d)) ", locCache.m_lBegLine, locCache.m_lBegColumn, locCache.m_lEndLine, locCache.m_lEndColumn);
        DebPrintf("To ((%3d, %3d),(%3d, %3d))", ploc->m_lBegLine, ploc->m_lBegColumn, ploc->m_lEndLine, ploc->m_lEndColumn);
    }
#endif DEBUG

    if (ploc->fIsInSourceFileHashTable == true)
    {
        // The sourcefile has a hash table of symbols that are hashed based on their
        // location. We need to update the hash table accroding to the new location
        // info for the symbols.

        // Remove the symbol from the hash table and hold on to the symbol
        BCSYM_NamedRoot *SymbolToUpdate = m_pSourceFile->RemoveSymbolFromHashTable(&locCache);
        VSASSERT(ploc->m_pSymbolForLocation == SymbolToUpdate, L"Two different symbols?");

        // Re-add the symbol back to the hash table with the new location.
        // This should add the symbol to the tail of the list of symbols
        // in one hash bucket. See VS213488
        if (SymbolToUpdate && ploc->IsValid())
        {
            m_pSourceFile->AddSymbolToHashTable((Location *) ploc, SymbolToUpdate);
        }

#if DEBUG
        if (VBFTESTSWITCH(fDebugLineMark))
        {
            if (ploc->m_locType == CodeBlockLoc)
            {
                DebPrintf(" (%S) ", ((TrackedCodeBlock *)ploc)->fProcBlock == true ? L"pTrackedProcBlock" : L"pTrackedCodeBlock");
            }

            DebPrintf(" \"%S\" (In HashTable)\n", (SymbolToUpdate && SymbolToUpdate->IsNamedRoot())? SymbolToUpdate->PNamedRoot()->GetName() : L"");
        }
#endif DEBUG
    }
    else
    {
#if DEBUG
        if (VBFTESTSWITCH(fDebugLineMark))
        {
            if (ploc->m_locType == CodeBlockLoc)
            {
                DebPrintf(" (%S) ", ((TrackedCodeBlock *)ploc)->fProcBlock == true ? L"pTrackedProcBlock" : L"pTrackedCodeBlock");
            }

            DebPrintf(" \"%S\" (Not In HashTable)\n", (ploc->m_pSymbolForLocation && ploc->m_pSymbolForLocation->IsNamedRoot())?  ploc->m_pSymbolForLocation->PNamedRoot()->GetName() : L"");
        }
#endif DEBUG
    }
}
#if DEBUG
void LineMarkerTable::CheckCodeBlockLocations()
{
    CompilerIdeLock lock(m_lock);
    for (auto it= m_trackedCodeBlockLocations.begin(); it != m_trackedCodeBlockLocations.end(); ++it)
    {
        TrackedLocation *ploc = *it;
        if (ploc->m_locType != CodeBlockLoc)
        {
            continue;
        }

        TrackedCodeBlock* pblock = (TrackedCodeBlock*) ploc;

        BCSYM_MethodImpl* pMethodImpl;
        CodeBlockLocation BlockFromTree;

        if (pblock->fProcBlock)
        {
            pMethodImpl = (BCSYM_MethodImpl*) (((char*) pblock) - BCSYM_MethodImpl::GetProcBlockOffset());
        }
        else
        {
            pMethodImpl = (BCSYM_MethodImpl*) (((char*) pblock) - BCSYM_MethodImpl::GetCodeBlockOffset());
        }

        ParseTree::StatementList  *pStatementsForLine = NULL;
        ParseTree::BlockStatement *pBlockForLine = NULL;

        if (pblock->m_lBegLine >= 0)
        {
            m_pSourceFile->GetParseTreeService()->GetStatementsOnLine(pblock->m_lBegLine, &pStatementsForLine, &pBlockForLine);
        }

        if (pStatementsForLine != NULL && pStatementsForLine->Element->IsMethodDefinition())
        {
            if (pblock->fProcBlock)
            {
                BlockFromTree = pStatementsForLine->Element->AsMethodDefinition()->EntireMethodLocation;
            }
            else
            {
                BlockFromTree = pStatementsForLine->Element->AsMethodDefinition()->BodyLocation;
            }

            if (pblock->m_oBegin != BlockFromTree.m_oBegin || pblock->m_oEnd != BlockFromTree.m_oEnd)
            {
                if (VBFTESTSWITCH(fDebugLineMark))
                {
                    DebPrintf("DumpCodeBlock %S : %S, m_oBegin = %d, m_oEnd = %d, TreeOBegin = %d, TreeOEnd = %d\n",
                        pblock->fProcBlock ? L"ProcBloc" : L"CodeBlock", pMethodImpl->GetName(),
                        pblock->m_oBegin, pblock->m_oEnd, BlockFromTree.m_oBegin, BlockFromTree.m_oEnd);
                }

                 //VSFAIL("Location info mismatch!");
            }
        }
    }
}
#endif DEBUG

#endif IDE
