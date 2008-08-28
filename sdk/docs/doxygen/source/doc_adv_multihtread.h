/**

\page doc_adv_multithread Multithreading

AngelScript supports multithreading, though not yet on all platforms. You can determine if multithreading 
is supported on your platform by calling the \ref asGetLibraryOptions function and checking the returned 
string for <code>"AS_NO_THREADS"</code> or <code>"!AS_NO_THREADS"</code>.

Even if you don't want or can't use multithreading, you can still write applications that execute 
\ref doc_adv_concurrent "multiple scripts simultaneously". 

\section doc_adv_multithread_1 Things to think about with a multithreaded environment

 - Always call \ref asThreadCleanup before terminating a thread that accesses the script engine. If this
   is not done, the memory allocated specifically for that thread will be lost until the script engine
   is freed.

 - Multiple threads may execute scripts in separate contexts. The contexts may execute scripts from the 
   same module, but if the module has global variables you need to make sure the scripts perform proper
   access control so that they do not get corrupted, as multiple threads try to update them simultaneously.

 - Only one thread at a time must be allowed to compile scripts. You may however have threads that 
   execute scripts while another thread is compiling new modules.

 - Resources that are shared by script modules such as registered properties and objects must be protected 
   by the application for simultaneous access, as the script engine doesn't do this.
   
*/
