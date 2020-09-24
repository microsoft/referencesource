Sys.NotifyCollectionChangedEventArgs = function(changes) {
    /// <summary>Describes how the collection was changed.</summary>
    /// <param name="changes" type="Array" elementType="Sys.CollectionChange">A list of changes that were performed on the collection since the last event.</param>
    this._changes = changes;
    Sys.NotifyCollectionChangedEventArgs.initializeBase(this);
}
Sys.NotifyCollectionChangedEventArgs.prototype = {
    get_changes: function() {
        /// <value type="Array" elementType="Sys.CollectionChange"></value>
        return this._changes || [];
    }
}
Sys.NotifyCollectionChangedEventArgs.registerClass("Sys.NotifyCollectionChangedEventArgs", Sys.EventArgs);

