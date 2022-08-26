
This Folder contains the source files we copied from Blend to remove our 
direct dependency on the Blend binaries

Original source locations:
	- \\authoring\sparkle\Source\1.0.1083.0\Common\Source
	
We only copied what we needed not the entire sources of the Expression assemblies

Changes from Expression:
	Changed namespace from Microsoft.Expression to MS.Internal.Designer.PropertyEditing.FromExpression
	Made all classes internal rather than public
	Removed some unused classes from files (see below)

*Note:* I did not reformat the files to follow our coding standards because we may want to do a diff against a future version
        of their sources and reformating would prevent this


Files copied:

	Diagnostics\AutomationId.cs

	Framework\ExceptionStringTable.cs (redirect to Resources.resx)
	Framework\IMessageLogger.cs
	Framework\MixedProperty.cs
	Framework\Tolerances.cs
	Framework\UIThreadDispatcher.cs

	Framework\Controls\WorkaroundPopup.cs

	Framework\Data\AppendSuffixConverter.cs
	Framework\Data\BoolToDoubleConverter.cs
	Framework\Data\BoolToVisibilityCollpasedConverter.cs
	Framework\Data\BoolToVisibilityHiddenConverter.cs
	Framework\Data\ComposingConverter.cs
	Framework\Data\DelegateCommand.cs
	Framework\Data\EqualsConverter.cs
	Framework\Data\IntegerToVisibilityConverter.cs
	Framework\Data\IsNullConverter.cs
	Framework\Data\NotConverter.cs
	Framework\Data\NullToBoolConverter.cs
	Framework\Data\ObservableCollection.cs
	Framework\Data\SwitchConverter.cs
	Framework\Data\ValueConverters.cs (not the complete file)
	Framework\Data\VisibilityOrConverter.cs (not the complete file)

	Framework\PropertyInspector\CategoryBase.cs
	Framework\PropertyInspector\CategoryContainer.xaml
	Framework\PropertyInspector\CategoryContainer.xaml.cs
	Framework\PropertyInspector\CategoryContainerCommands.cs
	Framework\PropertyInspector\CategoryLayoutContainer.cs
	Framework\PropertyInspector\ExtensibilityMetadataHelper.cs
	Framework\PropertyInspector\IPropertyInspector.cs
	Framework\PropertyInspector\NewItemFactoryTypeModel.cs
	Framework\PropertyInspector\PropertyContainerPopupHelper.cs
	Framework\PropertyInspector\StandardCategoryLayout.cs
	
	Framework\UserInterface\FocusScopeManager.cs
	
	Framework\ValueEditors\ChoiceEditor.cs
	Framework\ValueEditors\RenderUtils.cs
	Framework\ValueEditors\StringEditor.cs
	Framework\ValueEditors\ValueEditorUtils.cs (not the complete file)
	Framework\ValueEditors\ValueToIconConverter.cs
