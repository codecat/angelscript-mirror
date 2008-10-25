/**

\page doc_gc Garbage collection

Though AngelScript uses reference counting for memory management, there is still need for a garbage collector to 
take care of the few cases where circular referencing between objects prevents the reference counter from reaching zero.
As the application wants to guarantee responsiveness of the application, AngelScript doesn't invoke the garbage collector
automatically. For that reason it is important that the application do this manually at convenient times.

The garbage collector implemented in AngelScript is incremental, so it can be executed for short periods of time without
requiring the entire application to halt. For this reason, it is recommended that a call to \ref 
asIScriptEngine::GarbageCollect "GarbageCollect(false)" is made at least once during the normal event processing. This will
execute one step in the incremental process, eventually detecting and destroying objects being kept alive due to circular 
references.

This is not enough however, as executing the garbage collector like this will require many iterations to complete a full cycle.
Thus if the scripts behave extremely badly and leave a lot of dead objects with circular references, the objects will accumulate
faster than the garbage collector can detect and free them. For this reason it is necessary to monitor the number of objects
known to the garbage collector, and increase the frequency of the GarbageCollect calls if it increases a lot. Call the method
\ref asIScriptEngine::GetObjectsInGarbageCollectorCount "GetObjectsInGarbageCollectorCount" to determine the number of objects.

Finally, if the application goes into a state where responsiveness is not so critical, it might be a good idea to do a full
cycle on the garbage collector, thus cleaning up all garbage at once. To do this, call GarbageCollect(true).

\see \ref doc_memory



*/
