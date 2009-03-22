//
// This generator creates a header file that implements automatic
// wrapper functions for the generic calling convention.
//
// Originally implmented by George Yohng from 4Front Technologies in 2009-03-11
//

#include <stdio.h>

// Generate templates for up to this number of function parameters
const int MAXPARAM = 10;


int main()
{
    printf("#ifndef ASWRAPPEDCALL_H\n"
           "#define ASWRAPPEDCALL_H\n\n");

	// Add some instructions on how to use this 
	printf("// Generate the wrappers by calling the macro asDECLARE_WRAPPER in global scope. \n");
	printf("// Then register the wrapper function with the script engine using the asCALL_GENERIC \n");
	printf("// calling convention. The wrapper can handle both global functions and class methods.\n");
	printf("//\n");
	printf("// Example:\n");
	printf("//\n");
	printf("// asDECLARE_WRAPPER(MyGenericWrapper, MyRealFunction);\n");
	printf("// asDECLARE_WRAPPER(MyGenericClassWrapper, MyClass::Method);\n");
	printf("//\n");
    printf("// This file was generated to accept functions with a maximum of %d parameters.\n\n", MAXPARAM);

	// Include files
	printf("#include <new> // placement new\n");
    printf("#include <angelscript.h>\n\n");

    printf("union asNativeFuncPtr { void (*func)(); void (asNativeFuncPtr::*method)(); };\n\n");

	// This is the macro that should be used to implement the wrappers
    printf("#define asDECLARE_WRAPPER(wrapper_name,func) \\\n"
           "    static void wrapper_name(asIScriptGeneric *gen)\\\n"
           "    { \\\n"
           "        asCallWrappedFunc(&func,gen);\\\n"
           "    }\n\n"
           );

    printf("typedef void (*asNativeCallFunc)(asNativeFuncPtr,asIScriptGeneric *gen);\n\n");

    printf("// Cast a function pointer\n"
           "template<typename F>\n"
           "inline asNativeFuncPtr as_funcptr_cast(F x)\n"
           "{\n"
           "    asNativeFuncPtr ptr;\n"
           "    ptr.method=0;\n"
           "    *((F*)&ptr)=x;\n"
           "    return ptr;\n"
           "}\n\n");

    printf("// A helper class to accept reference parameters\n");

    printf("template<typename X>\n"
           "class as_wrapNative_helper\n"
           "{\n"
           "public:\n"
           "    X d;\n"
           "    as_wrapNative_helper(X d_) : d(d_) {}\n"
           "};\n\n");

    printf("// Workaround to keep GCC happy\n"
           "template<typename F>\n"
           "asNativeCallFunc as_wrapNative_cast(F f)\n"
           "{\n"
           "    asNativeCallFunc d;\n"
           "    *((F*)&d) = f;\n"
           "    return d;\n"
           "}\n\n");

	// Iterate over the number of parameters 
    for(int t = 0; t <= MAXPARAM; t++)
    {
		printf("// %d parameter(s)\n\n", t);

		// Iterate over the different function forms
        for(int d = 0; d < 4; d++)
        {
			int k;

			// Different forms of the function
            static const char *start[]=
            {"template<",                      // global function with no return type
             "template<typename R",            // global function with return type
             "template<typename C",            // class method with no return type
             "template<typename C,typename R"  // class method with return type
            };

            static const char *start2[]=
            {"<",
             "<R",
             "<C",
             "<C,R"
            };

            static const char *signature[]=
            {"_void",
             "",
             "_void_this",
             "_this"
            };

			//----------
			// Generate the function that extracts the parameters from 
			// the asIScriptGeneric interface and calls the native function

			// Build the template declaration
            if( (t>0) || (d>0) )
            {
                printf("%s",start[d]);

                for(int k=0;k<t;k++)
                    printf("%stypename T%d",(k||(d>0))?",":"",k+1);

                printf(">\n");
            }           

            printf("static void asWrapNative_p%d%s(asNativeFuncPtr func,asIScriptGeneric *%s)\n",
                   t,signature[d],((t>0)||(d>0))?"gen":"");

            printf("{\n"
                   "    typedef %s (%s*FuncType)(" ,(d&1)?"R":"void", (d&2)?"C::":"");

            for(k=0;k<t;k++)
                printf("%sT%d",k?",":"",k+1);

            printf(");\n\n");

            printf("     %s%s(",(d&1)?"new(gen->GetAddressOfReturnLocation()) as_wrapNative_helper<R>( ":"",

                   (d&2)?"((*((C*)gen->GetObject())).*((FuncType)(func.method)))":"((FuncType)(func.func))");

            for( k = 0; k < t; k++ )
                printf("%s ((as_wrapNative_helper<T%d> *)gen->GetAddressOfArg(%d))->d",k?",":"",k+1,k);

            printf(" ) %s;\n"
                   "}\n\n",(d&1)?")":"");

			//----------
			// Generate the function that calls the templated wrapper function.
			// This is overloads for the asCallWrappedFunc functions

			// Build the template declaration
            if( (t>0) || (d>0) )
            {
                printf("%s",start[d]);

                for(int k=0;k<t;k++)
                    printf("%stypename T%d", (k||(d>0))?",":"", k+1);

                printf(">\n");
            }

            printf("inline void asCallWrappedFunc(%s (%s*func)(", (d&1)?"R":"void", (d&2)?"C::":"");

            for( k =0; k < t; k++ )
                printf("%sT%d",k?",":"",k+1);

            printf("),asIScriptGeneric *gen)\n"
                   "{\n"
                   "    asWrapNative_p%d%s",t,signature[d]);

            if( (t>0) || (d>0) )
            {
                printf("%s",start2[d]);

                for( int k = 0; k < t; k++ )
                    printf("%sT%d",(k||(d>0))?",":"",k+1);

                printf(">");
            }
            printf("(as_funcptr_cast(func),gen);\n"
                   "}\n\n");
        }

    }    

    printf("#endif // ASWRAPPEDCALL_H\n\n");

    return 0;
}

