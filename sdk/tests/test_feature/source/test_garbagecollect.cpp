

#include "utils.h"

namespace TestGarbageCollect
{

void PrintString_Generic(asIScriptGeneric *gen)
{
	std::string *str = (std::string*)gen->GetArgAddress(0);
	//printf("%s",str->c_str());
}

bool Test()
{
	bool fail = false;

    // Create the script engine
    asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
 
    // Register Function
    RegisterScriptString(engine);
    engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString_Generic), asCALL_GENERIC);
 
    // Compile
    asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
    mod->AddScriptSection("script", 
        "class Obj{};"
        "class Hoge"
        "{"
        "    Hoge(){ Print('ctor\\n'); }"
        "    ~Hoge(){ Print('dtor\\n'); }"
        "    Obj@ obj;"
        "};"
        "void main()"
        "{"
        "    Hoge hoge;"
        "};"
        , 0);
    mod->Build();
 
    // Context Create
    asIScriptContext *ctx = engine->CreateContext();
 
    // Loop
    for( asUINT n = 0; n < 3; n++ )
    {
        // Execute
        //printf("----- execute\n");
        ctx->Prepare(mod->GetFunctionIdByDecl("void main()"));
        ctx->Execute();
 
        // GC
        const int GC_STEP_COUNT_PER_FRAME = 100;
        for ( int i = 0; i < GC_STEP_COUNT_PER_FRAME; ++i )
        {
            engine->GarbageCollect(asGC_ONE_STEP);
        }
        
        // Check status
        {
            asUINT currentSize = asUINT();
            asUINT totalDestroyed = asUINT();
            asUINT totalDetected = asUINT();
            engine->GetGCStatistics(&currentSize , &totalDestroyed , &totalDetected );
			if( currentSize    != 8 ||
				totalDestroyed != n+1 ||
				totalDetected  != 0 )
				fail = true;
            //printf("(%lu,%lu,%lu)\n" , currentSize , totalDestroyed , totalDetected );
        }
    }

    // Release 
    ctx->Release();
    engine->Release();

	// Test 
	{	
		COutStream out;
		int r;

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, 
			"interface ITest\n"
			"{\n"
			"}\n"
			"class Test : ITest\n"
			"{\n"
			"	ITest@[] arr;\n"
			"	void Set(ITest@ e)\n"
			"	{\n"
			"		arr.resize(1);\n"
			"		@arr[0]=e;\n"
			"	}\n"
			"}\n"
			"void main()\n"
			"{\n"
			"	Test@ t=Test();\n"
			"	t.Set(t);\n"
			"}\n");
		r = mod->Build();
		if( r < 0 )
			fail = true;

		asUINT currentSize;
		engine->GetGCStatistics(&currentSize);
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;

		engine->GetGCStatistics(&currentSize);

		engine->Release();
	}

	return fail;
}


} // namespace

