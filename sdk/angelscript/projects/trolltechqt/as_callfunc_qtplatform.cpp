// Add-on to load proper platform file

#if defined(__x86__)||defined(__i386__)||defined(_M_IX86)
#include "../../source/as_callfunc_x86.cpp"
#elif defined(__powerpc64__)
#include "../../source/as_callfunc_ppc_64.cpp"
#elif defined(__ppc__)||defined(__powerpc__)
#include "../../source/as_callfunc_ppc.cpp"
#else
#error Unknown Platform!
#endif
