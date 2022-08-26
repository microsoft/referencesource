//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to verify access exposure.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void Bindable::ResolvePartialTypesForContainer()
{
    if (GetStatusOfResolvePartialTypes() != NotStarted)
    {
        return;
    }

    SetStatusOfResolvePartialTypes(InProgress);

    BCSYM_Container *Container = CurrentContainer();

    VSASSERT( !Container->IsCCContainer(),
                    "Conditional compilation containers are not allowed here!!!");

    VSASSERT( Container->GetName(),
                    "How can a non-namespace container have no name ?");

    // Ensure that partial types have been bound for the enclosing container
    // Need to do this to make sure we can find all the partial components
    // of the current type correctly.
    //
    BCSYM_Container *Parent = Container->GetContainer();

    if (!Parent)
    {
        // Unnamed Namespace

        VSASSERT( Container->IsNamespace(),
                    "How can a non-namespace container not have a parent ?");

        SetStatusOfResolvePartialTypes(Done);
        return;
    }
    else
    {
        ResolvePartialTypesForContainer(Parent, m_CompilationCaches);

        // Get the parent again in case it has been updated when resolving the partial
        // types of its original parent
        Parent = Container->GetContainer();
    }

    DynamicArray<BCSYM_Container *> ListOfPreviouslyExistingTypes;
    DynamicArray<BCSYM_Container *> ListOfTypesInCurrentProject;

    // Size the dynamic arrays initially to 20 elements
    //
    ListOfPreviouslyExistingTypes.SizeArray(20);
    ListOfTypesInCurrentProject.SizeArray(20);

    if (Container->IsNamespace())
    {
        // Namespaces have a different partial type mechanism.
        // Namespaces are already formed into a ring by this
        // time.
        SetStatusOfResolvePartialTypes(Done);

        return;
    }

    FindAllPossibleTypeComponents(
        Parent,
        Container->GetName(),
        ListOfPreviouslyExistingTypes,
        ListOfTypesInCurrentProject);

    // This is to prevent this task from being started again through
    // one of the partial containers indirectly when GetHash triggers
    // the combining of the partial types
    //
    MarkStatusOfResolvePartialTypesForAllTypeComponents(
        InProgress,
        ListOfTypesInCurrentProject);

    FilterAllDuplicatesAndCombineTypes(
        Parent,
        ListOfPreviouslyExistingTypes,
        ListOfTypesInCurrentProject);

    MarkStatusOfResolvePartialTypesForAllTypeComponents(
        Done,
        ListOfTypesInCurrentProject);


}

void
Bindable::FindAllPossibleTypeComponents
(
    BCSYM_Container *EnclosingContainer,
    _In_z_ STRING *NameOfPartialType,
    DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
    // 


    FindAllPossibleTypeComponentsInContainer(
        EnclosingContainer,
        NameOfPartialType,
        ListOfPreviouslyExistingTypes,
        ListOfTypesInCurrentProject);

    if (EnclosingContainer->IsNamespace())
    {
        // Look up in the namespace ring in all namespaces available to the
        // current project.
        //
        CompilerProject *CurrentProject =
            EnclosingContainer->GetContainingProject();

        for(BCSYM_Namespace *NextNamespace =
                EnclosingContainer->PNamespace()->GetNextNamespace();
            NextNamespace != EnclosingContainer;
            NextNamespace = NextNamespace->GetNextNamespace())
        {
            if (NextNamespace->GetCompilerFile()->GetProject() != CurrentProject)
            {
                continue;
            }

            FindAllPossibleTypeComponentsInContainer(
                NextNamespace,
                NameOfPartialType,
                ListOfPreviouslyExistingTypes,
                ListOfTypesInCurrentProject);
        }
    }

    // Sort the type lists so that errors appear consistently against
    // the same symbols. Better user model.
    //
    // Perf.
    //
    //      ListOfTypesInCurrentProject would almost always contain a trivial
    //      number of entries, unless the user specifies about a 1000 partial
    //      classes for the same class, which is a pathological case.
    //
    // Note: ListOfPreviouslyExistingTypes should not be sorted because we
    //      use the order of discovery of symbols to report better duplicate errors
    //      against these.
    //
    SortListOfContainersByLocation(ListOfTypesInCurrentProject);
}

void
Bindable::FindAllPossibleTypeComponentsInContainer
(
    BCSYM_Container *EnclosingContainer,
    _In_z_ STRING *NameOfPartialType,
    DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
    VSASSERT(!DefinedInMetaData(EnclosingContainer),
                    "Why are type clashes across assemblies being checked ?");

    // We don't want to trigger resolving nested partial types
    // and just need the raw hash, else this task would be
    // reentrant.
    //
    BCSYM_Hash *BindableChildrenHash = EnclosingContainer->GetHashRaw();
    BCSYM_Hash *UnBindableChildrenHash = EnclosingContainer->GetUnBindableChildrenHashRaw();

    FindAllPossibleTypeComponentsInHash(
        BindableChildrenHash,
        NameOfPartialType,
        ListOfPreviouslyExistingTypes,
        ListOfTypesInCurrentProject);

    FindAllPossibleTypeComponentsInHash(
        UnBindableChildrenHash,
        NameOfPartialType,
        ListOfPreviouslyExistingTypes,
        ListOfTypesInCurrentProject);
}

void
Bindable::FindAllPossibleTypeComponentsInHash
(
    BCSYM_Hash *Hash,
    _In_z_ STRING *NameOfPartialType,
    DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
    if (!Hash)
    {
        return;
    }

    for(BCSYM_NamedRoot *PossibleType =
            Hash->SimpleBind(NameOfPartialType);
        PossibleType;
        PossibleType = PossibleType->GetNextOfSameName())
    {
        if (!PossibleType->IsContainer())
        {
            continue;
        }

        BCSYM_Container *Type = PossibleType->PContainer();

        // Since namespaces are merged by the namespace ring mechanism beforehand,
        // treat the namespaces to be the same.
        //
        if (Type->IsNamespace() &&
            CurrentContainer()->IsNamespace())
        {
            continue;
        }

        if (Type->IsNamespace())
        {
            // Since namespaces merge with other namespaces, but clash with non-namespaces,
            // we treat it as an existing type against which all non-namespaces clash.
            //
            ListOfPreviouslyExistingTypes.AddElement(Type);
        }
        else
        {
            //VSASSERT( (Type == CurrentContainer()) ||
            //        (!Type->IsBindingDone() &&
            //        Type->GetBindableInstance()->GetStatusOfResolvePartialTypes() == TaskTriState::NotStarted),
            //                "How can this type/namespace already have been through partial type resolution ?")
            // Temporary fix for Bug VSWhidbey 253211
            //
            // The actual problem is due to a editclassify issue that has existed since everett. The fix for this
            // problem is risky at this point of time for beta1, so instead this temporary solution for beta1.
            // Permanent fix will be address by fix for Beta2 Bug VSWhidbey 282466.
            //
            if (Type != CurrentContainer() &&
                (Type->IsBindingDone() ||
                    Type->GetBindableInstance()->GetStatusOfResolvePartialTypes() != NotStarted))
            {
                ListOfPreviouslyExistingTypes.AddElement(Type);
            }
            else
            {
                ListOfTypesInCurrentProject.AddElement(Type);
            }
        }
    }
}

void
Bindable::FilterAllDuplicatesAndCombineTypes
(
    BCSYM_Container *EnclosingContainer,
    DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
    // The list is sorted by location and source file name, so for correctly reporting
    // errors on the later occuring entities we elect the first suitable one with the
    // partial keyword in the list as the MainType.
    //
    for(unsigned Index = 0;
        Index < ListOfTypesInCurrentProject.Count();
        Index++)
    {
        BCSYM_Container *PossibleMainType = ListOfTypesInCurrentProject.Element(Index);

        VSASSERT( PossibleMainType->GetSourceFile(),
                    "How can a non-source file type get here ?");

        // Currently, only classes and structures can be partial types. So need to look
        // for partial type components for any other kind of types.
        //
        if (!PossibleMainType->IsClass())
        {
            continue;
        }

        // If already has main type, then this cannot be elected to be the main type
        //
        if (PossibleMainType->GetMainType())
        {
            continue;
        }

        // Elect the first type in the partial type set to be the main type

        BCSYM_Container *ContainerOfPossibleMainType = PossibleMainType->GetContainer();

        bool PartialModifierSeen = PossibleMainType->IsPartialKeywordUsed();
        BCSYM_Container *FirstTypeWithPartialSpecifier = NULL;

        // If none of the matching types have a 'Partial' modifier, then they are treated as
        // different types and clashing errors are given. Current rule is that atleast one of
        // them should have the 'Partial' modifier.

        // Determine if any of the matching types have a 'Partial' modifier.
        //
        if (PartialModifierSeen)
        {
            FirstTypeWithPartialSpecifier = PossibleMainType;
        }
        else
        {
            for(unsigned PartialTypeIndex = Index+1;
                PartialTypeIndex < ListOfTypesInCurrentProject.Count();
                PartialTypeIndex++)
            {
                BCSYM_Container *PossiblePartialType = ListOfTypesInCurrentProject.Element(PartialTypeIndex);

                if (BothAreClassesOrBothAreStructures(PossiblePartialType, PossibleMainType) &&
                    PossiblePartialType->GetGenericParamCount() == PossibleMainType->GetGenericParamCount() &&
                    PossiblePartialType->GetContainer()->IsSameContainer(ContainerOfPossibleMainType))
                {
                    if (PossiblePartialType->IsPartialKeywordUsed())
                    {
                        PartialModifierSeen = true;
                        FirstTypeWithPartialSpecifier = PossiblePartialType;
                        break;
                    }
                }
            }
        }

        unsigned NumberOfTypesWithoutPartialModifier = 0;

        // If any of the matching types has a 'Partial' modifier, then merge the types.
        //
        if (PartialModifierSeen)
        {
            BCSYM_Container *PrevPartialType = PossibleMainType;

            if (!PossibleMainType->IsPartialKeywordUsed())
            {
                NumberOfTypesWithoutPartialModifier++;
            }

            for(unsigned PartialTypeIndex = Index+1;
                PartialTypeIndex < ListOfTypesInCurrentProject.Count();
                PartialTypeIndex++)
            {
                BCSYM_Container *PossiblePartialType = ListOfTypesInCurrentProject.Element(PartialTypeIndex);

                if (BothAreClassesOrBothAreStructures(PossiblePartialType, PossibleMainType) &&
                    PossiblePartialType->GetGenericParamCount() == PossibleMainType->GetGenericParamCount() &&
                    PossiblePartialType->GetContainer()->IsSameContainer(ContainerOfPossibleMainType))
                {
                    if (!PossiblePartialType->IsPartialKeywordUsed())
                    {
                        NumberOfTypesWithoutPartialModifier++;
                    }

                    PossiblePartialType->SetIsPartialType(true);

                    // Make the partial component of a type invisible to the rest of the compiler.
                    // The rest of the compiler only deals with the main component of the type.
                    //
                    Symbols::MakeUnbindable(PossiblePartialType);

                    PrevPartialType->PClass()->SetNextPartialType(PossiblePartialType->PClass());
                    PossiblePartialType->PClass()->SetMainType(PossibleMainType->PClass());

                    PossiblePartialType->GetSourceFile()->SetHasPartialTypes();

                    PrevPartialType = PossiblePartialType;
                }
            }
        }

        if (PossibleMainType->GetNextPartialType())
        {
            PossibleMainType->GetSourceFile()->SetHasPartialTypes();

            VSASSERT(FirstTypeWithPartialSpecifier,
                        "How can type be merged if none are marked partial ?");

            // Report warnings when 2 or more of the partial type components
            // do not have the partial keyword. Special rule in spec.
            //
            if (NumberOfTypesWithoutPartialModifier >= 2)
            {
                for(BCSYM_Container *PartialType = PossibleMainType;
                    PartialType;
                    PartialType = PartialType->GetNextPartialType())
                {
                    if (PartialType->IsPartialKeywordUsed())
                    {
                        continue;
                    }

                    VSASSERT(PartialType != FirstTypeWithPartialSpecifier,
                                "Type with partial modifier cannot clash with itself!!!");

                    ReportTypeClashError(
                        FirstTypeWithPartialSpecifier,
                        PartialType,
                        true);  // both the clashing types are being merged
                }
            }
        }

        // Note that the election needs to done after the partial types are linked together
        // in order to optimize for the non-partial class scenarios.
        //
        ElectPartialTypeAsMainType(PossibleMainType->PClass());


        VerifyXMLDocCommentsOnPartialType(PossibleMainType->PClass());
    }


#if DEBUG
    for(unsigned Index = 0;
        Index < ListOfTypesInCurrentProject.Count();
        Index++)
    {
        BCSYM_Container *Type = ListOfTypesInCurrentProject.Element(Index);

        VSASSERT(!Type->IsPartialType() || Type->GetMainType(), "Main type election failed!!!");
    }
#endif


    // Report Duplicate errors on Main Types if any
    //
    ReportDuplicateErrorsOnMainTypes(
        EnclosingContainer,
        ListOfPreviouslyExistingTypes,
        ListOfTypesInCurrentProject);

    // Combine the different partial type sets to form logical types.
    //
    CombineTypes(ListOfTypesInCurrentProject);
}


void Bindable::VerifyXMLDocCommentsOnPartialType(BCSYM_Class *PartialClass)
{
    //Check XMLDoc comments to make sure that only one of the partial definitions
    //has a comment
    BCSYM_Class * pCurrentClass = PartialClass;
    BCSYM_Class * pFirstClassWithXMLDocNode = NULL;
    bool bFoundMultipleUserSpecifiedCommentBlocks = false;
    while (pCurrentClass)
    {
        if (pCurrentClass->GetXMLDocNode() && pFirstClassWithXMLDocNode)
        {
            VSASSERT(pFirstClassWithXMLDocNode->GetXMLDocNode(), "Where did our XMLDoc node go?");
            
            // Oh dear we've found more than 1 XMLDoc comment. Need to remove them from the XMLDoc file.
            bool isFirstClassDesignerGenerated = Semantics::IsDesignerControlClass(pFirstClassWithXMLDocNode, /* SearchThisSymbolOnly */ true);
            bool isCurrentClassDesignerGenerated = Semantics::IsDesignerControlClass(pCurrentClass, /* SearchThisSymbolOnly */ true);
            
            BCSYM_Class * pClassWithRedundantXMLDocNode = NULL;
            bool generateMultipleXMLDocWarning = false;

            if (isFirstClassDesignerGenerated)
            {
                if (!isCurrentClassDesignerGenerated)
                {
                    // Ignore the XML doc comment on the designer generated partial definition.
                    pClassWithRedundantXMLDocNode = pFirstClassWithXMLDocNode;
                    pFirstClassWithXMLDocNode = pCurrentClass;
                }
                else
                {
                    // Multiple designer generated partial definitions.
                    // Ignore the XML doc comment on the current designer generated definition.
                    pClassWithRedundantXMLDocNode = pCurrentClass;
                }
            }
            else
            {
                if (!isCurrentClassDesignerGenerated)
                {
                    // Neither of the partial definitions are designer generated.
                    // Ignore the XML doc comment on the current definition and also generate WRNID_XMLDocOnAPartialType.
                    pClassWithRedundantXMLDocNode = pCurrentClass;
                    generateMultipleXMLDocWarning = true;
                }
                else
                {
                    // Ignore the XML doc comment on the designer generated partial definition.
                    pClassWithRedundantXMLDocNode = pCurrentClass;
                }
            }

            SourceFile *pSourceFile = pClassWithRedundantXMLDocNode->GetSourceFile();
            pSourceFile->GetXMLDocFile()->DisableXMLDocNode(pClassWithRedundantXMLDocNode->GetXMLDocNode());
            Location *loc = pClassWithRedundantXMLDocNode->GetXMLDocNode()->GetCommentLocation();
            pClassWithRedundantXMLDocNode->SetXMLDocNode(NULL);

            if (generateMultipleXMLDocWarning)
            {
                bFoundMultipleUserSpecifiedCommentBlocks = true;
                ReportErrorAtLocation(
                    WRNID_XMLDocOnAPartialType,
                    pClassWithRedundantXMLDocNode->GetBindableInstance()->CurrentErrorLog(pClassWithRedundantXMLDocNode),
                    loc,
                    StringOfSymbol(m_SourceFile->GetCompiler(),
                    pClassWithRedundantXMLDocNode));
            }
        }
        else if (pCurrentClass->GetXMLDocNode())
        {
            //This is the first comment we have found so far
            pFirstClassWithXMLDocNode = pCurrentClass;
        }

        pCurrentClass = pCurrentClass->GetNextPartialType();
    }

    if (bFoundMultipleUserSpecifiedCommentBlocks)
    {
        //Now we know the first comment we found is invalid becuase we found more than one so we disable it too.
        SourceFile *pSourceFile = pFirstClassWithXMLDocNode->GetSourceFile();
        pSourceFile->GetXMLDocFile()->DisableXMLDocNode(pFirstClassWithXMLDocNode->GetXMLDocNode());
        Location *loc = pFirstClassWithXMLDocNode->GetXMLDocNode()->GetCommentLocation();
        pFirstClassWithXMLDocNode->SetXMLDocNode(NULL);
        ReportErrorAtLocation(
            WRNID_XMLDocOnAPartialType,
            pFirstClassWithXMLDocNode->GetBindableInstance()->CurrentErrorLog(pFirstClassWithXMLDocNode),
            loc,
            StringOfSymbol(m_SourceFile->GetCompiler(), pFirstClassWithXMLDocNode));

    }
}

void Bindable::ElectPartialTypeAsMainType
(
    BCSYM_Class *TypeToMorphToMainType
)
{
    // Set this partial type component to be the main type.
    //
    TypeToMorphToMainType->SetIsPartialType(false);

    // If only one type component is present, then no need to backup any info.
    //
    if (!TypeToMorphToMainType->GetNextPartialType())
    {
        return;
    }


#if IDE

    // Back up the main class info to aid with decompilation.
    //
    // Alloc and back up the info for the partial class that will be changed.
    //
    MainClassBackupInfo *BackedupClassInfo =
        (MainClassBackupInfo *)TypeToMorphToMainType->GetBindableInstance()->CurrentAllocator()->Alloc(sizeof MainClassBackupInfo);

    BackedupClassInfo->m_Access = TypeToMorphToMainType->GetRawAccess();
    BackedupClassInfo->m_IsShadowsKeywordUsed = TypeToMorphToMainType->IsShadowsKeywordUsed();
    BackedupClassInfo->m_IsNotInheritable = TypeToMorphToMainType->IsNotInheritable();
    BackedupClassInfo->m_IsMustInherit = TypeToMorphToMainType->IsMustInherit();
    BackedupClassInfo->m_RawBase = TypeToMorphToMainType->GetRawBase();

    TypeToMorphToMainType->SetMainClassBackupInfo(BackedupClassInfo);

#endif IDE
}

void Bindable::ValidateModifiersAcrossPartialTypes
(
    BCSYM_Class *MainType
)
{
    VSASSERT(!MainType->IsPartialType(), "Partial type unexpected!!!");

    // Get the logical access modifiers and report any clashes across
    // partial types.

    bool ExplicitlySpecifiedAccessFound = false;
    ACCESS Access = ACCESS_Public;

    bool IsShadowingKeywordUsed = false;
    bool IsNotInheritable = false;
    bool IsMustInherit = false;

    for(BCSYM_Class *PartialType = MainType;
        PartialType;
        PartialType = PartialType->GetNextPartialType()->PClass())
    {
        // Detect the first explicitly specified access and that is the access of the whole type.
        // If any explicitly specified accesses mismatch, then reported partial type access mismatch
        // error. Note that when an explicitly specified access is present, the implicit access on
        // any of the partial types even if different from the explicit one is ignored.
        //
        if (!PartialType->IsTypeAccessExplictlySpecified())
        {
            if (!ExplicitlySpecifiedAccessFound)
            {
                Access = PartialType->GetRawAccess();
            }
        }
        else if (!ExplicitlySpecifiedAccessFound)
        {
            ExplicitlySpecifiedAccessFound = true;
            Access = PartialType->GetRawAccess();
        }
        else if (PartialType->GetRawAccess() != Access)
        {
            // Specified access '|1' for '|2' does not match the access '|3' specified on one of its other partial types.

            ReportErrorOnSymbol(
                ERRID_PartialTypeAccessMismatch3,
                PartialType->GetBindableInstance()->CurrentErrorLog(PartialType),
                PartialType,
                StringOfAccess(CurrentCompilerInstance(), PartialType->GetRawAccess()),
                GetQualifiedErrorName(PartialType),
                StringOfAccess(CurrentCompilerInstance(), Access));
        }

        // Detect if any of the partial types are marked as shadows.
        //
        if (PartialType->IsShadowsKeywordUsed())
        {
            IsShadowingKeywordUsed = true;
        }

        // Detect if any of the partial types are marked as NotInheritable
        //
        if (PartialType->IsNotInheritable())
        {
            IsNotInheritable = true;
        }

        // Detect if any of the partial types are marked as MustInherit
        //
        if (PartialType->IsMustInherit())
        {
            IsMustInherit = true;
        }
    }

    // If any of the partial types are marked as NotInheritable, then for the partial type
    // that are not marked NotInheritable, we need to invalidate the use of MustInherit and
    // Mustoverride. Note that for the partial types explicitly marked as NotInheritable, no
    // validation to do here because declared would have already done this validation.
    //
    if (IsNotInheritable)
    {
        for(BCSYM_Class *PartialType = MainType;
            PartialType;
            PartialType = PartialType->GetNextPartialType()->PClass())
        {
            // In this case, declared would already have done the appropriate validation.
            //
            if (PartialType->IsNotInheritable())
            {
                // Declared should already have handled this clash on the same physical container
                //
                VSASSERT(!PartialType->IsMustInherit(), "Unexpected modifier clash!!!");

                continue;
            }

            if (PartialType->IsMustInherit())
            {
                // Declared should already have handled this clash on the same physical container
                //
                VSASSERT(!PartialType->IsNotInheritable(), "Unexpected modifier clash!!!");

                // 'MustInherit' cannot be specified for partial type '|1' because it cannot be combined with 'NotInheritable'
                // specified for one of its other partial types.

                ReportErrorOnSymbol(
                    ERRID_PartialTypeBadMustInherit1,
                    PartialType->GetBindableInstance()->CurrentErrorLog(PartialType),
                    PartialType,
                    GetQualifiedErrorName(PartialType));
            }

            const bool LookOnlyInCurrentPartialType = true;
            BCITER_CHILD_ALL PartialTypeMemberIter(PartialType, LookOnlyInCurrentPartialType);

            while (BCSYM_NamedRoot *Member = PartialTypeMemberIter.GetNext())
            {
                if (Member->IsProc() &&
                    Member->PProc()->IsMustOverrideKeywordUsed())
                {
                    if (!Member->IsSyntheticMethod() &&
                        !IsSynthetic(Member))
                    {
                        // 'MustOverride' cannot be specified on '|1' because it is in a partial type one of whose other partial types
                        // is declared 'NotInheritable' thus making the whole class 'NotInheritable'.

                        // 'NotInheritable' classes cannot have members declared 'MustOverride'.

                        StringBuffer ErrorNameOfMember;

                        ReportErrorOnSymbol(
                            ERRID_MustOverOnNotInheritPartClsMem1,
                            PartialType->GetBindableInstance()->CurrentErrorLog(Member),
                            Member,
                            PartialType->GetBindableInstance()->CurrentErrorLog(Member)->
                                ExtractErrorName(
                                    Member,
                                    PartialType,
                                    ErrorNameOfMember));
                    }

                    Member->PProc()->SetIgnoreMustOverrideKeyword(true);
                }
            }
        }
    }

    MainType->SetAccess(Access);
    MainType->SetShadowsKeywordUsed(IsShadowingKeywordUsed);
    MainType->SetNotInheritable(IsNotInheritable);
    MainType->SetMustInherit(IsNotInheritable ? false : IsMustInherit);
}

void
Bindable::SortListOfContainersByLocation
(
    DynamicArray<BCSYM_Container *> &ListOfContainers
)
{
    qsort(
        ListOfContainers.Array(),
        ListOfContainers.Count(),
        sizeof(BCSYM_Container *),
        SortSymbolsByLocation);
}

bool
Bindable::BothAreClassesOrBothAreStructures
(
    BCSYM_Container *Container1,
    BCSYM_Container *Container2
)
{
    return
        BothAreClasses(Container1, Container2) ||
        BothAreStructures(Container1, Container2);
}

bool
Bindable::BothAreClasses
(
    BCSYM_Container *Container1,
    BCSYM_Container *Container2
)
{
    return
        IsClass(Container1) && IsClass(Container2);
}

bool
Bindable::BothAreStructures
(
    BCSYM_Container *Container1,
    BCSYM_Container *Container2
)
{
    return
        IsStructure(Container1) && IsStructure(Container2);
}

bool
Bindable::BothAreInterfaces
(
    BCSYM_Container *Container1,
    BCSYM_Container *Container2
)
{
    return
        IsInterface(Container1) && IsInterface(Container2);
}

void
Bindable::ReportDuplicateErrorsOnMainTypes
(
    BCSYM_Container *EnclosingContainer,
    DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
   // Report Duplicate errors on Main Types if any

    BCSYM_Container *FirstMainType = NULL;

    if (ListOfPreviouslyExistingTypes.Count() > 0)
    {
        // 




        FirstMainType =
            ListOfPreviouslyExistingTypes.Element(
                ListOfPreviouslyExistingTypes.Count() - 1);

        ReportDuplicateErrorOnMainType(
            EnclosingContainer,
            FirstMainType,
            0,      // Index in ListOfTypesInCurrentProject from which to start reporting errors on
            ListOfTypesInCurrentProject);

    }
    else if (ListOfTypesInCurrentProject.Count() > 0)
    {
        for(unsigned Index = 0;
            Index < ListOfTypesInCurrentProject.Count();
            Index++)
        {
            if (ListOfTypesInCurrentProject.Element(Index)->IsPartialType())
            {
                continue;
            }

            FirstMainType = ListOfTypesInCurrentProject.Element(Index);

            ReportDuplicateErrorOnMainType(
                EnclosingContainer,
                FirstMainType,
                Index + 1,      // Index in ListOfTypesInCurrentProject from which to start reporting errors on
                ListOfTypesInCurrentProject);

            break;
        }
    }
}

void
Bindable::ReportDuplicateErrorOnMainType
(
    BCSYM_Container *EnclosingContainer,
    BCSYM_Container *FirstMainType,
    unsigned IndexOfFirstPossibleDuplicate,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
    // Don't report duplicate errors among metadata containers, because these errors
    // will be reported only if somebody tries to bind to one of them.

    do
    {
        CompilerFile* FileOfFirstMainType = FirstMainType->GetCompilerFile();
        BindableMemberInfo FirstMainTypeInfo(FirstMainType);

        BCSYM_Container *NextMainTypeToCheckDuplicatesAgainst = NULL;
        unsigned IndexToStartDuplicateCheckingFrom = 0;

        for(unsigned Index = IndexOfFirstPossibleDuplicate;
            Index < ListOfTypesInCurrentProject.Count();
            Index++)
        {
            BCSYM_Container *DuplicateMainType = ListOfTypesInCurrentProject.Element(Index);

            // It is okay to have multiple Namespaces with the same name
            //
            if (FirstMainType->IsNamespace() && DuplicateMainType->IsNamespace())
            {
                continue;
            }

            if (DuplicateMainType->IsPartialType())
            {
                continue;
            }

            if (DuplicateMainType->IsDuplicateType())
            {
                continue;
            }

            SourceFile *SourceFileOfDuplicateMainType = DuplicateMainType->GetSourceFile();
            BindableMemberInfo DuplicateMainTypeInfo(DuplicateMainType);

            // No overloading on arity or partial types permitted among synthetic types.
            //
            if (FirstMainTypeInfo.IsSynthetic() ||
                DuplicateMainTypeInfo.IsSynthetic())
            {
                ReportSyntheticMemberClashInSameClass(
                    &DuplicateMainTypeInfo,
                    &FirstMainTypeInfo);

                MakeDuplicateContainerBad(DuplicateMainType);
                continue;
            }

            {
                // Overloading of types on arity allowed.
                //
                if (FirstMainType->GetGenericParamCount() != DuplicateMainType->GetGenericParamCount())
                {
                    if (!NextMainTypeToCheckDuplicatesAgainst)
                    {
                        NextMainTypeToCheckDuplicatesAgainst = DuplicateMainType;
                        IndexToStartDuplicateCheckingFrom = Index + 1;
                    }
                    continue;
                }
            }

            ReportTypeClashError(
                FirstMainType,
                DuplicateMainType,
                false);     // false indicates that the conflict types cannot be merged together

            MakeDuplicateContainerBad(DuplicateMainType);
        }

        FirstMainType = NextMainTypeToCheckDuplicatesAgainst;
        IndexOfFirstPossibleDuplicate = IndexToStartDuplicateCheckingFrom;

    } while (FirstMainType);
}

void
Bindable::ReportTypeClashError
(
    BCSYM_Container *Type,
    BCSYM_Container *ClashingType,
    bool AreTypesMerged
)
{
    Container *EnclosingContainer = ClashingType->GetContainer();
    Compiler *CompilerInstance = CurrentCompilerInstance();

    CompilerFile *FileOfType = Type->GetCompilerFile();
    SourceFile *SourceFileOfClashingType = ClashingType->GetSourceFile();

    VSASSERT(SourceFileOfClashingType, "Non-Source clashing type unexpected!!!");

    VSASSERT(!AreTypesMerged || Type->IsPartialKeywordUsed(),
                "Merging warnings unexpected between types both of which are not makred partial!!!");

    if (FileOfType != SourceFileOfClashingType)
    {
        // Found dupe in separate file ?
        // Give the source file to help them know where to find the thing we dup'd against
        //
        // 



        STRING *FileNameToReport =
            FileOfType->IsSourceFile() ?
                (FileOfType->IsSolutionExtension() ?
                    SourceFileOfClashingType->GetFileName() :
                    FileOfType->GetFileName()):
                FileOfType->PMetaDataFile()->GetName();


        // "|1 '|2' and |3 '|4' declared in '|5' conflict in |6 '|7', but are being merged because one of them is declared partial."
        // or
        // "|1 '|2' and |3 '|4', declared in '|5' conflict in |6 '|7'."

        ReportErrorOnSymbol(
            AreTypesMerged ?
                WRNID_TypeConflictButMerged7 :
                ERRID_TypeConflict7,
            ClashingType->GetBindableInstance()->CurrentErrorLog(ClashingType),
            ClashingType,
            StringOfSymbol(CompilerInstance, ClashingType),
            GetQualifiedErrorName(ClashingType),
            StringOfSymbol(CompilerInstance, Type),
            GetQualifiedErrorName(Type),
            FileNameToReport,
            StringOfSymbol(CompilerInstance, EnclosingContainer),
            StringPool::IsEqual(EnclosingContainer->GetName(), STRING_CONST(CompilerInstance, EmptyString)) ?
                STRING_CONST(CurrentCompilerInstance(), UnnamedNamespaceErrName) :
                EnclosingContainer->GetName());
    }
    else
    {
        // these are in the same file so no need to give source file info

        // "|1 '|2' and |3 '|4' conflict in |5 '|6', but are being merged because one of them is declared partial."
        // or
        // "|1 '|2' and |3 '|4' conflict in |5 '|6'."

        ReportErrorOnSymbol(
            AreTypesMerged ?
                WRNID_TypeConflictButMerged6 :
                ERRID_TypeConflict6,
            ClashingType->GetBindableInstance()->CurrentErrorLog(ClashingType),
            ClashingType,
            StringOfSymbol(CompilerInstance, ClashingType),
            GetQualifiedErrorName(ClashingType),
            StringOfSymbol(CompilerInstance, Type),
            GetQualifiedErrorName(Type),
            StringOfSymbol(CompilerInstance, EnclosingContainer),
            StringPool::IsEqual(EnclosingContainer->GetName(), STRING_CONST(CompilerInstance, EmptyString)) ?
                STRING_CONST(CurrentCompilerInstance(), UnnamedNamespaceErrName) :
                EnclosingContainer->GetName());
    }
}

void
Bindable::MakeDuplicateContainerBad
(
    BCSYM_Container *Container
)
{
    Container->SetIsBad();
    Container->SetIsDuplicateType(true);
    Symbols::MakeUnbindable(Container);
}

void
Bindable::CombineTypes
(
    DynamicArray<BCSYM_Container *> &ListOfTypes
)
{
    for(unsigned Index = 0;
        Index < ListOfTypes.Count();
        Index++)
    {
        BCSYM_Container *PossibleMainType = ListOfTypes.Element(Index);

        if (PossibleMainType->IsPartialType())
        {
            continue;
        }

        CombineMainAndPartialTypes(PossibleMainType);
    }
}

void
Bindable::CombineMainAndPartialTypes
(
    BCSYM_Container *MainType
)
{
    VSASSERT( !DefinedInMetaData(MainType),
                    "Why are we trying to combine metadata container ?");

    VSASSERT( !MainType->IsPartialType(),
                    "How can the main type be partial ?");

    if (!MainType->GetNextPartialType())
    {
        return;
    }

    VSASSERT( MainType->IsClass(),
                "Only classes and structures can have partial types!!!");

    BCSYM_Class *MainClass = MainType->PClass();

    ValidateModifiersAcrossPartialTypes(MainClass);

    BCSYM_Hash *PrevHash, *FirstHash;
    BCSYM_Hash *PrevUnBindableChildrenHash, *FirstUnBindableChildrenHash;
    BCSYM_Hash *PrevBackingFieldsHash, *FirstBackingFieldsHash;

    FirstHash = PrevHash = MainClass->GetHashRaw();
    FirstUnBindableChildrenHash = PrevUnBindableChildrenHash = MainClass->GetUnBindableChildrenHashRaw();
    FirstBackingFieldsHash = PrevBackingFieldsHash = MainClass->GetBackingFieldsForStaticsRaw();

    bool PartialTypesHaveAttributes = false;
    WellKnownAttrVals *CommonAttrVals = NULL;

    MainType->GetBindableInstance()->SetContainerHasPartialTypeComponents(true);

    for(BCSYM_Class *PartialClass = MainType->GetNextPartialType()->PClass();
        PartialClass;
        PartialClass = PartialClass->GetNextPartialType()->PClass())
    {
        VSASSERT( PartialClass->IsPartialType(),
                        "Partial type not marked as partial ?");

        PartialClass->GetBindableInstance()->SetContainerHasPartialTypeComponents(true);

        CombineHashes(
            MainClass,
            FirstHash,
            PrevHash,
            PartialClass->GetHashRaw());

        CombineHashes(
            MainClass,
            FirstUnBindableChildrenHash,
            PrevUnBindableChildrenHash,
            PartialClass->GetUnBindableChildrenHashRaw());

        CombineHashes(
            MainClass,
            FirstBackingFieldsHash,
            PrevBackingFieldsHash,
            PartialClass->GetBackingFieldsForStaticsRaw());

        // Set a common shared well known attribute block to all the partial types of a type
        //
        if (PartialClass->GetPAttrVals())
        {
            if (!CommonAttrVals)
            {
                CommonAttrVals = MainClass->GetBindableInstance()->CurrentAllocator()->Alloc<WellKnownAttrVals>();
                CommonAttrVals = new (CommonAttrVals) WellKnownAttrVals(MainClass->GetBindableInstance()->CurrentAllocator());
            }

            VSASSERT( !PartialClass->GetPAttrVals()->GetPWellKnownAttrVals(),
                            "How can this be set already ? Bad decompilation!!!");

            PartialClass->GetPAttrVals()->SetPWellKnownAttrVals(CommonAttrVals);
        }
    }

    MainClass->SetHashes(FirstHash, FirstUnBindableChildrenHash);
    MainClass->SetBackingFieldsForStatics(FirstBackingFieldsHash);

    if (CommonAttrVals)
    {
        if (!MainClass->GetPAttrVals())
        {
            MainClass->AllocAttrVals(
                MainClass->GetBindableInstance()->CurrentAllocator(),
                CS_Bound,
                MainClass->GetSourceFile(),
                MainClass);
        }

        VSASSERT( !MainClass->GetPAttrVals()->GetPWellKnownAttrVals(),
                        "How can this be set already ? Bad decompilation!!!");

        MainClass->GetPAttrVals()->SetPWellKnownAttrVals(CommonAttrVals);
    }

    RemoveDuplicateSynthesizedSymbols(MainType);
}

void
Bindable::CombineHashes
(
    BCSYM_Class *NewParent,
    BCSYM_Hash *&FirstHash,
    BCSYM_Hash *&PrevHash,
    BCSYM_Hash *NextHash
)
{
    if (!NextHash)
    {
        return;
    }

    VSASSERT( !NextHash->GetPrevHash(),
                    "Incorrect partial type decompilation ?");

    if (!PrevHash)
    {
        FirstHash = NextHash;
        PrevHash = NextHash;
    }
    else if (NextHash)
    {
        PrevHash->SetNextHash(NextHash);
        NextHash->SetPrevHash(PrevHash);

        PrevHash = NextHash;
    }

    VSASSERT( NextHash->GetLogicalContainer() == NextHash->GetPhysicalContainer(),
                    "How did these change already ?");

    NextHash->SetPhysicalContainer(NextHash->GetLogicalContainer());
    NextHash->SetLogicalContainer(NewParent);

}

void
Bindable::RemoveDuplicateSynthesizedSymbols
(
    BCSYM_Container *MainType
)
{
    VSASSERT( !MainType->IsPartialType(),
                    "Partial type unexpected!!!");

    VSASSERT( MainType->GetNextPartialType(),
                    "Why is this being invoked on a type with no partial components ?");

    STRING *NameOfSharedConstructor = STRING_CONST(CurrentCompilerInstance(), SharedConstructor);
    STRING *NameOfInstanceConstructor = STRING_CONST(CurrentCompilerInstance(), Constructor);
    STRING *NameOfFormSubMain = STRING_CONST(CurrentCompilerInstance(), Main);

    RemoveDuplicateSyntheticMethods(
        NameOfSharedConstructor,
        MainType);

    RemoveDuplicateSyntheticMethods(
        NameOfInstanceConstructor,
        MainType);

    RemoveDuplicateSyntheticMethods(
        NameOfFormSubMain,
        MainType);
}

void
Bindable::RemoveDuplicateSyntheticMethods
(
    _In_z_ STRING *NameOfSyntheticMethod,
    BCSYM_Container *MainType
)
{
    bool SeenNonSyntheticMemberByThisName = false;

    DynamicArray<BCSYM_SyntheticMethod *> SyntheticMethods;

    for(BCSYM_NamedRoot *PossibleSyntheticMethod = MainType->GetHash()->SimpleBind(NameOfSyntheticMethod);
        PossibleSyntheticMethod;
        PossibleSyntheticMethod = PossibleSyntheticMethod->GetNextOfSameName())
    {
        if (!PossibleSyntheticMethod->IsSyntheticMethod())
        {
            SeenNonSyntheticMemberByThisName = true;
            continue;
        }

        BCSYM_SyntheticMethod *SyntheticMethod = PossibleSyntheticMethod->PSyntheticMethod();
        SyntheticMethods.AddElement(SyntheticMethod);
    }

    unsigned StartIndexForInvalidatingSyntheticMethods;
    if (SeenNonSyntheticMemberByThisName)
    {
        StartIndexForInvalidatingSyntheticMethods = 0;
    }
    else
    {
        // Bug VSWhidbey 494456.
        if (SyntheticMethods.Count() > 0 &&
            MainType->IsClass() &&
            MainType->PClass()->IsMustInherit() &&
            SyntheticMethods.Element(0)->PSyntheticMethod()->GetKind() == SYNTH_New)
        {
            AssertIfFalse(SyntheticMethods.Element(0)->GetAccess() == ACCESS_Public);
            SyntheticMethods.Element(0)->SetAccess(ACCESS_Protected);
        }

        StartIndexForInvalidatingSyntheticMethods = 1;
    }

    for(unsigned Index = StartIndexForInvalidatingSyntheticMethods;
        Index < SyntheticMethods.Count();
        Index++)
    {
        // Invalidate this member because it has probably already been
        // generated in another partial type component or one of the
        // other type components has a user defined member by this name.
        //
        DeleteSymbolInBindable(SyntheticMethods.Element(Index));
    }
}

void
Bindable::DeleteSymbolInBindable
(
    BCSYM_NamedRoot *Member
)
{
    VSASSERT( Member->GetImmediateParent(),
                    "Deleting unnamed namespace ?");

    VSASSERT( Member->GetImmediateParent()->IsHash(),
                    "Unexpected Member!!! Member not in hash ?");

    AssertIfFalse(Member->GetSourceFile()->GetCompState() == CS_Declared);

    Symbols::RemoveSymbolFromHash(
        Member->GetImmediateParent()->PHash(),
        Member);

    VSASSERT( Member->GetPhysicalContainer(),
                    "Deleting unnamed namespace ?");

#if IDE
    // Remember having deleted this symbol in bindable.
    // Needed for decompilation.
    //
    BCSYM_Container *PhysicalContainerOfMember = Member->GetPhysicalContainer();

    SymbolList *BindableDeletedSymbolList =
        PhysicalContainerOfMember->GetBindableDeletedSymbolList();

    SourceFile *SourceFileOfMember =
        PhysicalContainerOfMember->GetSourceFile();

    Symbols SymbolAllocator(
        SourceFileOfMember->GetCompiler(),
        SourceFileOfMember->SymbolStorage(),
        NULL);

    SymbolAllocator.GetAliasOfSymbol(
        Member,
        ACCESS_Private,
        BindableDeletedSymbolList);

#endif
}


void
Bindable::MarkStatusOfResolvePartialTypesForAllTypeComponents
(
    TaskTriState Status,
    DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
)
{
    for(unsigned Index = 0;
        Index < ListOfTypesInCurrentProject.Count();
        Index++)
    {
        BCSYM_Container *Container = ListOfTypesInCurrentProject.Element(Index);

        Container->GetBindableInstance()->
            SetStatusOfResolvePartialTypes(Status);
    }
}


void
Bindable::ResolvePartialTypesForNestedContainers()
{
    if (GetStatusOfResolvePartialTypesForNestedContainers() != NotStarted)
    {
        return;
    }

    BCSYM_Container *Container = CurrentContainer();

    VSASSERT( !DefinedInMetaData(Container),
                    "Metadata container unexpected!!!");

    if (Container->IsPartialTypeAndHasMainType())
    {
        Container = Container->GetMainType();
    }

    MarkStatusOfResolvePartialTypesForNestedContainers(
        InProgress,
        Container);

    // Resolve partial types for the nested types in this container
    // and also in its partial container siblings or children.
    //
    BCSYM_Container *CurrentContainer = Container;
    do
    {
        for(BCSYM_Container *NestedType = CurrentContainer->GetNestedTypes();
            NestedType;
            NestedType = NestedType->GetNextNestedType())
        {
            ResolvePartialTypesForContainer(NestedType, m_CompilationCaches);
        }

    } while(CurrentContainer = CurrentContainer->GetNextPartialType());


    MarkStatusOfResolvePartialTypesForNestedContainers(
        Done,
        Container);

}

void
Bindable::MarkStatusOfResolvePartialTypesForNestedContainers
(
    TaskTriState Status,
    BCSYM_Container *Container
)
{
    VSASSERT( !Container->IsPartialTypeAndHasMainType(),
                    "Partial type with main type unexpected!!!");

    do
    {
        Container->GetBindableInstance()->
            SetStatusOfResolvePartialTypesForNestedContainers(Status);
    }
    while (Container = Container->GetNextPartialType());
}

bool
Bindable::HavePartialTypesBeenResolvedForGivenType
(
    BCSYM_Container *Container
)
{
    if (DefinedInMetaData(Container))
    {
        return true;
    }

    return Container->IsBindingDone() ||
           Container->GetBindableInstance()->GetStatusOfResolvePartialTypes() == Done;

}

