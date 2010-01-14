/**

\page doc_debug Debugging scripts

\todo Complete this page

AngelScript offers a rich interface to support the debugging of scripts. It is easy to build an embedded debugger 
that can set break points, inspect/manipulate variables in functions, visualize the call stack, etc. 

\section doc_debug_1 Setting line breaks

\ref asIScriptContext::SetLineCallback, \ref asIScriptContext::GetCurrentFunction, \ref asIScriptContext::GetCurrentLineNumber, 
\ref asIScriptFunction::GetScriptSectionName, \ref asIScriptEngine::GetFunctionDescriptorById, \ref asIScriptContext::Suspend



\section doc_debug_2 Viewing the call stack

\ref asIScriptContext::GetCallstackSize, \ref asIScriptContext::GetCallstackFunction, \ref asIScriptContext::GetCallstackLineNumber



\section doc_debug_3 Inspecting variables

\ref asIScriptContext::GetVarCount, \ref asIScriptContext::GetVarTypeId, \ref asIScriptContext::GetAddressOfVar, 
\ref asIScriptContext::GetVarDeclaration, \ref asIScriptContext::GetThisPointer, \ref asIScriptContext::GetThisTypeId, 
\ref asIScriptObject





*/
