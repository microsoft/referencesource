Sys.NotifyCollectionChangedAction = function() {
    /// <summary>Describes how a collection has changed.</summary>
    /// <field name="add" type="Number" integer="true" static="true"/>
    /// <field name="remove" type="Number" integer="true" static="true"/>
    /// <field name="reset" type="Number" integer="true" static="true"/>
    throw Error.notImplemented();
}
Sys.NotifyCollectionChangedAction.prototype = {
    add: 0,
    remove: 1,
    reset: 2
}
Sys.NotifyCollectionChangedAction.registerEnum('Sys.NotifyCollectionChangedAction');
