/**

\page doc_debug Debugging scripts

AngelScript offers a rich interface to support the debugging of scripts. It is easy to build an embedded debugger 
that can set break points, inspect/manipulate variables in functions, visualize the call stack, etc. 

Observe that the CDebugMgr class used in the examples below doesn't exist. It is only used an abstraction to
avoid having to write fictional debug routines.

\section doc_debug_1 Setting line breaks

In order to break at a specified line in the code the debugger can set the line callback function in the script context. The
VM will then invoke the callback for each statement executed, allowing the debugger to decide whether to proceed to the next
statement or not.

\code
// An example line callback
void DebugLineCallback(asIScriptContext *ctx, CDebugMgr *dbg)
{
  // Determine if we have reached a break point
  int funcId = ctx->GetCurrentFunction();
  int line   = ctx->GetCurrentLineNumber();

  // Determine the script section from the function implementation
  const asIScriptFunction *function = engine->GetFunctionDescriptorById(funcId);
  const char *scriptSection = function->GetScriptSectionName();

  // Now let the debugger check if a breakpoint has been set here
  if( dbg->IsBreakpoint(scriptSection, line, funcId) )
  {
    // A break point has been reached so the execution of the script should be suspended
    ctx->Suspend();
  }
}
\endcode

The line callback is set on the context with the following call:

\code
  // Set the line callback with the address of the debug manager as parameter
  ctx->SetLineCallback(asFUNCTION(DebugLineCallback), dbg, asCALL_CDECL);
\endcode

When the line callback suspends the execution the context's \ref asIScriptContext::Execute "Execute" function will return with
the code \ref asEXECUTION_SUSPENDED. The application can then go into a special message loop where the debug routines can be handled,
e.g. to \ref doc_debug_2 "view the call stack", \ref doc_debug_3 "examine variables", etc. Once the execution should continue, simply
call the Execute method again to resume it.

An alternative to suspending the script execution might be to start the message loop directly within the line callback, in which
case resuming the execution is done simply by returning from the line callback function. Which is the easiest to implement depends on
how you have implemented your application.











\section doc_debug_2 Viewing the call stack

\ref asIScriptContext::GetCallstackSize, \ref asIScriptContext::GetCallstackFunction, \ref asIScriptContext::GetCallstackLineNumber

\todo Complete this page













\section doc_debug_3 Inspecting variables

\ref asIScriptContext::GetVarCount, \ref asIScriptContext::GetVarTypeId, \ref asIScriptContext::GetAddressOfVar, 
\ref asIScriptContext::GetVarDeclaration, \ref asIScriptContext::GetThisPointer, \ref asIScriptContext::GetThisTypeId, 
\ref asIScriptObject





*/
