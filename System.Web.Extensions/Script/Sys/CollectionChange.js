Sys.CollectionChange = function(action, newItems, newStartingIndex, oldItems, oldStartingIndex) {
    /// <summary>Describes a change in a collection.</summary>
    /// <param name="action" type="Sys.NotifyCollectionChangedAction"></param>
    /// <param name="newItems" optional="true" mayBeNull="true">The items that were added when action is add or replace.</param>
    /// <param name="newStartingIndex" type="Number" integer="true" optional="true" mayBeNull="true">The index where new items have been inserted.</param>
    /// <param name="oldItems" optional="true" mayBeNull="true">The items that were removed when action is remove or replace.</param>
    /// <param name="oldStartingIndex" type="Number" integer="true" optional="true" mayBeNull="true">The index where old items have been removed.</param>
    /// <field name="action" type="Sys.NotifyCollectionChangedAction"></field>
    /// <field name="newItems" type="Array" mayBeNull="true" elementMayBeNull="true">The items that were added when action is add.</field>
    /// <field name="newStartingIndex" type="Number" integer="true">The index where new items have been inserted.</field>
    /// <field name="oldItems" type="Array" mayBeNull="true" elementMayBeNull="true">The items that were removed when action is remove.</field>
    /// <field name="oldStartingIndex" type="Number" integer="true">The index where old items have been removed.</field>
    this.action = action;
    if (newItems) {
        if (!(newItems instanceof Array)) {
            newItems = [newItems];
        }
    }
    this.newItems = newItems || null;
    if (typeof newStartingIndex !== "number") {
        newStartingIndex = -1;
    }
    this.newStartingIndex = newStartingIndex;
    if (oldItems) {
        if (!(oldItems instanceof Array)) {
            oldItems = [oldItems];
        }
    }
    this.oldItems = oldItems || null;
    if (typeof oldStartingIndex !== "number") {
        oldStartingIndex = -1;
    }
    this.oldStartingIndex = oldStartingIndex;
}
Sys.CollectionChange.registerClass("Sys.CollectionChange");
