/**

\page doc_adv_jit JIT compilation

\todo Complete this page

AngelScript doesn't provide a built-in JIT compiler, instead it permits an external JIT compiler to be implemented. 

To use JIT compilation, the scripts must be compiled with a few extra instructions that provide hints to the JIT compiler
and also entry points so that the VM will know when to pass control to the JIT compiled function. By default this is 
turned off, and must thus be turned on by setting the engine property \ref asEP_INCLUDE_JIT_INSTRUCTIONS.

 - \ref asIJITCompiler
 - \ref asJITFunction
 - \ref asSVMRegisters
 - \ref asEBCInstr
 - \ref asSBCInfo

 - \ref asBC_DWORDARG
 - \ref asBC_INTARG
 - \ref asBC_QWORDARG
 - \ref asBC_FLOATARG
 - \ref asBC_PTRARG
 - \ref asBC_WORDARG0
 - \ref asBC_WORDARG1
 - \ref asBC_SWORDARG0
 - \ref asBC_SWORDARG1
 - \ref asBC_SWORDARG2

*/
