namespace System.Windows
{
    /// <summary>
    ///     Property value freeze callback
    /// </summary>
    internal delegate bool FreezeValueCallback(DependencyObject d, DependencyProperty dp, EntryIndex entryIndex, PropertyMetadata metadata, bool isChecking);
}

