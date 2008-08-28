/**

\page doc_adv_timeout Timeout long running scripts

The line callback feature is used to be able to some special treatment 
during execution of the scripts. The callback is called for every script 
statement, which for example makes it possible to verify if the script has  
executed for too long time and if so suspend the execution to be resumed at
a later time.

Before calling the context's \ref asIScriptContext::Execute "Execute" method, set the callback function 
like so:

\code
void ExecuteScript()
{
  DWORD timeOut;
  ctx->SetLineCallback(asFUNCTION(LineCallback), &timeOut, asCALL_CDECL);

  int status = asEXECUTION_SUSPENDED;
  while( status == asEXECUTION_SUSPENDED )
  {
    // Allow the long running script to execute for 10ms
    timeOut = timeGetTime() + 10;
    status = ctx->Execute();
  
    // The execution was suspended, now we can do something else before
    // continuing the execution with another call to Execute().
    ...
  }
}

void LineCallback(asIScriptContext *ctx, DWORD *timeOut)
{
  // If the time out is reached we suspend the script
  if( *timeOut < timeGetTime() )
    ctx->Suspend();
}
\endcode


Take a look at the sample \ref doc_samples_events to 
see this working.










*/
