#ifndef ASWRAPPEDCALL_H
#define ASWRAPPEDCALL_H

// Generate the wrappers by calling the macro asDECLARE_WRAPPER in global scope. 
// Then register the wrapper function with the script engine using the asCALL_GENERIC 
// calling convention. The wrapper can handle both global functions and class methods.
//
// Example:
//
// asDECLARE_WRAPPER(MyGenericWrapper, MyRealFunction);
// asDECLARE_WRAPPER(MyGenericClassWrapper, MyClass::Method);
//
// This file was generated to accept functions with a maximum of 10 parameters.

#include <new> // placement new
#include <angelscript.h>

union asNativeFuncPtr { void (*func)(); void (asNativeFuncPtr::*method)(); };

#define asDECLARE_WRAPPER(wrapper_name,func) \
    static void wrapper_name(asIScriptGeneric *gen)\
    { \
        asCallWrappedFunc(&func,gen);\
    }

typedef void (*asNativeCallFunc)(asNativeFuncPtr,asIScriptGeneric *gen);

// Cast a function pointer
template<typename F>
inline asNativeFuncPtr as_funcptr_cast(F x)
{
    asNativeFuncPtr ptr;
    ptr.method=0;
    *((F*)&ptr)=x;
    return ptr;
}

// A helper class to accept reference parameters
template<typename X>
class as_wrapNative_helper
{
public:
    X d;
    as_wrapNative_helper(X d_) : d(d_) {}
};

// Workaround to keep GCC happy
template<typename F>
asNativeCallFunc as_wrapNative_cast(F f)
{
    asNativeCallFunc d;
    *((F*)&d) = f;
    return d;
}

// 0 parameter(s)

static void asWrapNative_p0_void(asNativeFuncPtr func,asIScriptGeneric *)
{
    typedef void (*FuncType)();

     ((FuncType)(func.func))( ) ;
}

inline void asCallWrappedFunc(void (*func)(),asIScriptGeneric *gen)
{
    asWrapNative_p0_void(as_funcptr_cast(func),gen);
}

template<typename R>
static void asWrapNative_p0(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)();

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ) );
}

template<typename R>
inline void asCallWrappedFunc(R (*func)(),asIScriptGeneric *gen)
{
    asWrapNative_p0<R>(as_funcptr_cast(func),gen);
}

template<typename C>
static void asWrapNative_p0_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)();

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ) ;
}

template<typename C>
inline void asCallWrappedFunc(void (C::*func)(),asIScriptGeneric *gen)
{
    asWrapNative_p0_void_this<C>(as_funcptr_cast(func),gen);
}

template<typename C,typename R>
static void asWrapNative_p0_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)();

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ) );
}

template<typename C,typename R>
inline void asCallWrappedFunc(R (C::*func)(),asIScriptGeneric *gen)
{
    asWrapNative_p0_this<C,R>(as_funcptr_cast(func),gen);
}

// 1 parameter(s)

template<typename T1>
static void asWrapNative_p1_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d ) ;
}

template<typename T1>
inline void asCallWrappedFunc(void (*func)(T1),asIScriptGeneric *gen)
{
    asWrapNative_p1_void<T1>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1>
static void asWrapNative_p1(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d ) );
}

template<typename R,typename T1>
inline void asCallWrappedFunc(R (*func)(T1),asIScriptGeneric *gen)
{
    asWrapNative_p1<R,T1>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1>
static void asWrapNative_p1_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d ) ;
}

template<typename C,typename T1>
inline void asCallWrappedFunc(void (C::*func)(T1),asIScriptGeneric *gen)
{
    asWrapNative_p1_void_this<C,T1>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1>
static void asWrapNative_p1_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d ) );
}

template<typename C,typename R,typename T1>
inline void asCallWrappedFunc(R (C::*func)(T1),asIScriptGeneric *gen)
{
    asWrapNative_p1_this<C,R,T1>(as_funcptr_cast(func),gen);
}

// 2 parameter(s)

template<typename T1,typename T2>
static void asWrapNative_p2_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d ) ;
}

template<typename T1,typename T2>
inline void asCallWrappedFunc(void (*func)(T1,T2),asIScriptGeneric *gen)
{
    asWrapNative_p2_void<T1,T2>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2>
static void asWrapNative_p2(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d ) );
}

template<typename R,typename T1,typename T2>
inline void asCallWrappedFunc(R (*func)(T1,T2),asIScriptGeneric *gen)
{
    asWrapNative_p2<R,T1,T2>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2>
static void asWrapNative_p2_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d ) ;
}

template<typename C,typename T1,typename T2>
inline void asCallWrappedFunc(void (C::*func)(T1,T2),asIScriptGeneric *gen)
{
    asWrapNative_p2_void_this<C,T1,T2>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2>
static void asWrapNative_p2_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d ) );
}

template<typename C,typename R,typename T1,typename T2>
inline void asCallWrappedFunc(R (C::*func)(T1,T2),asIScriptGeneric *gen)
{
    asWrapNative_p2_this<C,R,T1,T2>(as_funcptr_cast(func),gen);
}

// 3 parameter(s)

template<typename T1,typename T2,typename T3>
static void asWrapNative_p3_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d ) ;
}

template<typename T1,typename T2,typename T3>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3),asIScriptGeneric *gen)
{
    asWrapNative_p3_void<T1,T2,T3>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3>
static void asWrapNative_p3(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d ) );
}

template<typename R,typename T1,typename T2,typename T3>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3),asIScriptGeneric *gen)
{
    asWrapNative_p3<R,T1,T2,T3>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3>
static void asWrapNative_p3_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3),asIScriptGeneric *gen)
{
    asWrapNative_p3_void_this<C,T1,T2,T3>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3>
static void asWrapNative_p3_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3),asIScriptGeneric *gen)
{
    asWrapNative_p3_this<C,R,T1,T2,T3>(as_funcptr_cast(func),gen);
}

// 4 parameter(s)

template<typename T1,typename T2,typename T3,typename T4>
static void asWrapNative_p4_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4),asIScriptGeneric *gen)
{
    asWrapNative_p4_void<T1,T2,T3,T4>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4>
static void asWrapNative_p4(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4),asIScriptGeneric *gen)
{
    asWrapNative_p4<R,T1,T2,T3,T4>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4>
static void asWrapNative_p4_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4),asIScriptGeneric *gen)
{
    asWrapNative_p4_void_this<C,T1,T2,T3,T4>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4>
static void asWrapNative_p4_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4),asIScriptGeneric *gen)
{
    asWrapNative_p4_this<C,R,T1,T2,T3,T4>(as_funcptr_cast(func),gen);
}

// 5 parameter(s)

template<typename T1,typename T2,typename T3,typename T4,typename T5>
static void asWrapNative_p5_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4,T5);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4,typename T5>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4,T5),asIScriptGeneric *gen)
{
    asWrapNative_p5_void<T1,T2,T3,T4,T5>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5>
static void asWrapNative_p5(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4,T5);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4,T5),asIScriptGeneric *gen)
{
    asWrapNative_p5<R,T1,T2,T3,T4,T5>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5>
static void asWrapNative_p5_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4,T5);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4,T5),asIScriptGeneric *gen)
{
    asWrapNative_p5_void_this<C,T1,T2,T3,T4,T5>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5>
static void asWrapNative_p5_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4,T5);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4,T5),asIScriptGeneric *gen)
{
    asWrapNative_p5_this<C,R,T1,T2,T3,T4,T5>(as_funcptr_cast(func),gen);
}

// 6 parameter(s)

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static void asWrapNative_p6_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4,T5,T6);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4,T5,T6),asIScriptGeneric *gen)
{
    asWrapNative_p6_void<T1,T2,T3,T4,T5,T6>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static void asWrapNative_p6(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4,T5,T6);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4,T5,T6),asIScriptGeneric *gen)
{
    asWrapNative_p6<R,T1,T2,T3,T4,T5,T6>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static void asWrapNative_p6_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4,T5,T6);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4,T5,T6),asIScriptGeneric *gen)
{
    asWrapNative_p6_void_this<C,T1,T2,T3,T4,T5,T6>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static void asWrapNative_p6_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4,T5,T6);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4,T5,T6),asIScriptGeneric *gen)
{
    asWrapNative_p6_this<C,R,T1,T2,T3,T4,T5,T6>(as_funcptr_cast(func),gen);
}

// 7 parameter(s)

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
static void asWrapNative_p7_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4,T5,T6,T7);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4,T5,T6,T7),asIScriptGeneric *gen)
{
    asWrapNative_p7_void<T1,T2,T3,T4,T5,T6,T7>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
static void asWrapNative_p7(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4,T5,T6,T7);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4,T5,T6,T7),asIScriptGeneric *gen)
{
    asWrapNative_p7<R,T1,T2,T3,T4,T5,T6,T7>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
static void asWrapNative_p7_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4,T5,T6,T7),asIScriptGeneric *gen)
{
    asWrapNative_p7_void_this<C,T1,T2,T3,T4,T5,T6,T7>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
static void asWrapNative_p7_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4,T5,T6,T7),asIScriptGeneric *gen)
{
    asWrapNative_p7_this<C,R,T1,T2,T3,T4,T5,T6,T7>(as_funcptr_cast(func),gen);
}

// 8 parameter(s)

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
static void asWrapNative_p8_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4,T5,T6,T7,T8),asIScriptGeneric *gen)
{
    asWrapNative_p8_void<T1,T2,T3,T4,T5,T6,T7,T8>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
static void asWrapNative_p8(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4,T5,T6,T7,T8),asIScriptGeneric *gen)
{
    asWrapNative_p8<R,T1,T2,T3,T4,T5,T6,T7,T8>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
static void asWrapNative_p8_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4,T5,T6,T7,T8),asIScriptGeneric *gen)
{
    asWrapNative_p8_void_this<C,T1,T2,T3,T4,T5,T6,T7,T8>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
static void asWrapNative_p8_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4,T5,T6,T7,T8),asIScriptGeneric *gen)
{
    asWrapNative_p8_this<C,R,T1,T2,T3,T4,T5,T6,T7,T8>(as_funcptr_cast(func),gen);
}

// 9 parameter(s)

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
static void asWrapNative_p9_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9),asIScriptGeneric *gen)
{
    asWrapNative_p9_void<T1,T2,T3,T4,T5,T6,T7,T8,T9>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
static void asWrapNative_p9(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9),asIScriptGeneric *gen)
{
    asWrapNative_p9<R,T1,T2,T3,T4,T5,T6,T7,T8,T9>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
static void asWrapNative_p9_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9),asIScriptGeneric *gen)
{
    asWrapNative_p9_void_this<C,T1,T2,T3,T4,T5,T6,T7,T8,T9>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
static void asWrapNative_p9_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9),asIScriptGeneric *gen)
{
    asWrapNative_p9_this<C,R,T1,T2,T3,T4,T5,T6,T7,T8,T9>(as_funcptr_cast(func),gen);
}

// 10 parameter(s)

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
static void asWrapNative_p10_void(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10);

     ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d, ((as_wrapNative_helper<T10> *)gen->GetAddressOfArg(9))->d ) ;
}

template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
inline void asCallWrappedFunc(void (*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10),asIScriptGeneric *gen)
{
    asWrapNative_p10_void<T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>(as_funcptr_cast(func),gen);
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
static void asWrapNative_p10(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((FuncType)(func.func))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d, ((as_wrapNative_helper<T10> *)gen->GetAddressOfArg(9))->d ) );
}

template<typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
inline void asCallWrappedFunc(R (*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10),asIScriptGeneric *gen)
{
    asWrapNative_p10<R,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>(as_funcptr_cast(func),gen);
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
static void asWrapNative_p10_void_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef void (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10);

     ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d, ((as_wrapNative_helper<T10> *)gen->GetAddressOfArg(9))->d ) ;
}

template<typename C,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
inline void asCallWrappedFunc(void (C::*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10),asIScriptGeneric *gen)
{
    asWrapNative_p10_void_this<C,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>(as_funcptr_cast(func),gen);
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
static void asWrapNative_p10_this(asNativeFuncPtr func,asIScriptGeneric *gen)
{
    typedef R (C::*FuncType)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10);

     new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ((*((C*)gen->GetObject())).*((FuncType)(func.method)))( ((as_wrapNative_helper<T1> *)gen->GetAddressOfArg(0))->d, ((as_wrapNative_helper<T2> *)gen->GetAddressOfArg(1))->d, ((as_wrapNative_helper<T3> *)gen->GetAddressOfArg(2))->d, ((as_wrapNative_helper<T4> *)gen->GetAddressOfArg(3))->d, ((as_wrapNative_helper<T5> *)gen->GetAddressOfArg(4))->d, ((as_wrapNative_helper<T6> *)gen->GetAddressOfArg(5))->d, ((as_wrapNative_helper<T7> *)gen->GetAddressOfArg(6))->d, ((as_wrapNative_helper<T8> *)gen->GetAddressOfArg(7))->d, ((as_wrapNative_helper<T9> *)gen->GetAddressOfArg(8))->d, ((as_wrapNative_helper<T10> *)gen->GetAddressOfArg(9))->d ) );
}

template<typename C,typename R,typename T1,typename T2,typename T3,typename T4,typename T5,typename T6,typename T7,typename T8,typename T9,typename T10>
inline void asCallWrappedFunc(R (C::*func)(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10),asIScriptGeneric *gen)
{
    asWrapNative_p10_this<C,R,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>(as_funcptr_cast(func),gen);
}

#endif // ASWRAPPEDCALL_H

