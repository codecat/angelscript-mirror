#include "utils.h"

namespace TestContext
{

std::string g_buf;
void print(asIScriptGeneric* gen)
{
	std::string* s = (std::string*)gen->GetArgAddress(0);
	g_buf += *s;
}

void pause(asIScriptGeneric*)
{
	asIScriptContext* ctx = asGetActiveContext();
	ctx->Suspend();
}

void abort(asIScriptGeneric*)
{
	asIScriptContext* ctx = asGetActiveContext();
	ctx->Abort();
}

class CContextSerializer
{
public:
	CContextSerializer() : storage("test"), m_ctx(0) {}
	
	int Serialize(asIScriptContext* ctx)
	{
		m_ctx = ctx;
		bool fail = false;
		int r;

		asUINT callstackSize = ctx->GetCallstackSize();
		storage.Write(&callstackSize, sizeof(callstackSize));

		// First store the entire callstack state
		for (int n = ctx->GetCallstackSize() - 1; n >= 0; n--)
		{
			// Get the stack frame content
			asDWORD stackFramePointer; // TODO: Shouldn't this be a pointer?
			asIScriptFunction* currentFunction;
			asDWORD programPointer; // TODO: Shouldn't this be a pointer?
			asDWORD stackPointer; // TODO: Shouldn't this be a pointer?
			asDWORD stackIndex;  // TODO: This shouldn't be needed. It should be transparent to the application how the stack memory is laid out
			r = ctx->GetCallStateRegisters(n, &stackFramePointer, &currentFunction, &programPointer, &stackPointer, &stackIndex);
			if (r == asSUCCESS)
			{
				bool stackFrame = true;
				storage.Write(&stackFrame, sizeof(stackFrame));

				storage.Write(&stackFramePointer, sizeof(stackFramePointer));
				storage.Write(&currentFunction, sizeof(currentFunction)); // TODO: serialize the function declaration
				storage.Write(&programPointer, sizeof(programPointer));
				storage.Write(&stackPointer, sizeof(stackPointer));
				storage.Write(&stackIndex, sizeof(stackIndex)); // TODO: This shouldn't be needed
			}
			else if (r == asNO_FUNCTION)
			{
				// This is a nested call, so get the state registers instead
				asIScriptFunction* callingSystemFunction;
				asIScriptFunction* initialFunction;
				asDWORD originalStackPointer;
				asDWORD argumentSize;
				asQWORD valueRegister;
				void* objectRegister;
				asITypeInfo* objectType;
				r = ctx->GetStateRegisters(n, &callingSystemFunction, &initialFunction, &originalStackPointer, &argumentSize, &valueRegister, &objectRegister, &objectType);
				if (r < 0)
					TEST_FAILED;

				bool stackFrame = false;
				storage.Write(&stackFrame, sizeof(stackFrame));

				storage.Write(&callingSystemFunction, sizeof(callingSystemFunction)); // TODO: serialize the function declaration
				storage.Write(&initialFunction, sizeof(initialFunction)); // TODO: serialize the function declaration
				storage.Write(&originalStackPointer, sizeof(originalStackPointer));
				storage.Write(&argumentSize, sizeof(argumentSize));
				storage.Write(&valueRegister, sizeof(valueRegister));
				storage.Write(&objectRegister, sizeof(objectRegister)); // TODO: serialize the object pointer
				storage.Write(&objectType, sizeof(objectType)); // TODO: serialize the type info
			}
			else
				TEST_FAILED;
		}

		// Store the context state registers
		// TODO: This should be in a function as it is repeated
		asIScriptFunction* callingSystemFunction;
		asIScriptFunction* initialFunction;
		asDWORD originalStackPointer; // TODO: Shouldn't this be a pointer?
		asDWORD argumentSize;
		asQWORD valueRegister;
		void* objectRegister;
		asITypeInfo* objectType;
		r = ctx->GetStateRegisters(0, &callingSystemFunction, &initialFunction, &originalStackPointer, &argumentSize, &valueRegister, &objectRegister, &objectType);
		if (r < 0)
			TEST_FAILED;

		bool stackFrame = false;
		storage.Write(&stackFrame, sizeof(stackFrame));

		storage.Write(&callingSystemFunction, sizeof(callingSystemFunction)); // TODO: serialize the function declaration
		storage.Write(&initialFunction, sizeof(initialFunction)); // TODO: serialize the function declaration
		storage.Write(&originalStackPointer, sizeof(originalStackPointer));
		storage.Write(&argumentSize, sizeof(argumentSize));
		storage.Write(&valueRegister, sizeof(valueRegister));
		storage.Write(&objectRegister, sizeof(objectRegister)); // TODO: serialize the object pointer
		storage.Write(&objectType, sizeof(objectType)); // TODO: serialize the type info

		// Then store the stack values
		for (int n = ctx->GetCallstackSize() - 1; n >= 0; n--)
		{
			r = ctx->GetCallStateRegisters(n, 0, 0, 0, 0, 0);
			if (r == asSUCCESS)
			{
				// Get variables on the stack frame
				int thisTypeId = ctx->GetThisTypeId(n);
				storage.Write(&thisTypeId, sizeof(thisTypeId)); // TODO: Serialize the type id
				int varCount = ctx->GetVarCount(n); 
				storage.Write(&varCount, sizeof(varCount));
				for (int v = 0; v < varCount; v++)
				{
					if (!ctx->IsVarInScope(v, n))
						continue;

					int typeId;
					asETypeModifiers typeModifiers;
					ctx->GetVar(v, n, 0, &typeId, &typeModifiers);
					void* var = ctx->GetAddressOfVar(v, n);
					if (!(typeModifiers & asTM_INOUTREF))
					{
						bool isValue = true;
						storage.Write(&isValue, sizeof(isValue));
						storage.Write(&typeId, sizeof(typeId)); // TODO: serialize the type id
						if (typeId == asTYPEID_INT32)
							storage.Write(var, sizeof(asINT32));
						else if (typeId == asTYPEID_DOUBLE)
							storage.Write(var, sizeof(double));
						else if (typeId == ctx->GetEngine()->GetTypeIdByDecl("string"))
						{
							// If object is not initialized yet, the pointer will be null
							bool isNull = var == 0;
							storage.Write(&isNull, sizeof(bool));
							if (!isNull)
							{
								asUINT len = asUINT(reinterpret_cast<std::string*>(var)->length());
								storage.Write(&len, sizeof(len));
								storage.Write(reinterpret_cast<std::string*>(var)->data(), len);
							}
						}
						else
							TEST_FAILED; // TODO: Add support for more types
					}
					else
					{
						bool isValue = false;
						storage.Write(&isValue, sizeof(isValue));

						// Serialize the reference
						// Reference variables must not serialize to themselves. 
						r = SerializeAddress(var, v, n, false);
						if (r < 0)
							TEST_FAILED;
					}
				}

				// Get values pushed on the stack
				int stackValuesCount = ctx->GetArgsOnStackCount(n);
				storage.Write(&stackValuesCount, sizeof(stackValuesCount));
				for (int v = 0; v < stackValuesCount; v++)
				{
					int typeId;
					asUINT flags;
					void* address;
					ctx->GetArgOnStack(n, v, &typeId, &flags, &address);

					if ((flags & asTM_INOUTREF) || (typeId & asTYPEID_MASK_OBJECT))
					{
						r = SerializeAddress(*(void**)address, v, n, true);
						if (r < 0)
							TEST_FAILED;
					}
					else if (typeId == asTYPEID_UINT64 || typeId == asTYPEID_INT64 || typeId == asTYPEID_DOUBLE)
						storage.Write(address, 8);
					else
						storage.Write(address, 4);
				}
			}
		}

		return fail ? -1 : 0;
	}

	int Deserialize(asIScriptContext* ctx)
	{
		m_ctx = ctx;
		bool fail = false;
		int r;

		storage.Restart();
		r = ctx->StartDeserialization();
		if (r < 0)
			TEST_FAILED;
		asUINT callstackSize;
		storage.Read(&callstackSize, sizeof(callstackSize));

		// First restore the callstack
		for (asUINT n = 0; n < callstackSize; n++)
		{
			bool stackFrame;
			storage.Read(&stackFrame, sizeof(stackFrame));

			if (stackFrame)
			{
				// Get the stack frame content
				asDWORD stackFramePointer;
				asIScriptFunction* currentFunction;
				asDWORD programPointer;
				asDWORD stackPointer;
				asDWORD stackIndex;
				storage.Read(&stackFramePointer, sizeof(stackFramePointer));
				storage.Read(&currentFunction, sizeof(currentFunction));
				storage.Read(&programPointer, sizeof(programPointer));
				storage.Read(&stackPointer, sizeof(stackPointer));
				storage.Read(&stackIndex, sizeof(stackIndex));

				// Set the stack frame
				r = ctx->PushFunction(currentFunction, 0); // TODO: Set object if method call (not for delegate)
				r = ctx->SetCallStateRegisters(0, stackFramePointer, currentFunction, programPointer, stackPointer, stackIndex);
				if (r < 0)
					TEST_FAILED;
			}
			else
			{
				// Nested function call. Get the pushed stack register
				asIScriptFunction* callingSystemFunction;
				asIScriptFunction* initialFunction;
				asDWORD originalStackPointer;
				asDWORD argumentSize;
				asQWORD valueRegister;
				void* objectRegister;
				asITypeInfo* objectType;

				storage.Read(&callingSystemFunction, sizeof(callingSystemFunction)); // TODO: serialize the function declaration
				storage.Read(&initialFunction, sizeof(initialFunction)); // TODO: serialize the function declaration
				storage.Read(&originalStackPointer, sizeof(originalStackPointer));
				storage.Read(&argumentSize, sizeof(argumentSize));
				storage.Read(&valueRegister, sizeof(valueRegister));
				storage.Read(&objectRegister, sizeof(objectRegister)); // TODO: serialize the object pointer
				storage.Read(&objectType, sizeof(objectType)); // TODO: serialize the type info

				// Push the state registers
				r = ctx->PushState();
				if (r < 0)
					TEST_FAILED;
				r = ctx->SetStateRegisters(0, callingSystemFunction, initialFunction, originalStackPointer, argumentSize, valueRegister, objectRegister, objectType);
				if (r < 0)
					TEST_FAILED;
			}
		}

		// Read the context state registers
		asIScriptFunction* callingSystemFunction;
		asIScriptFunction* initialFunction;
		asDWORD originalStackPointer; // TODO: Shouldn't this be a pointer?
		asDWORD argumentSize;
		asQWORD valueRegister;
		void* objectRegister;
		asITypeInfo* objectType;

		bool stackFrame;
		storage.Read(&stackFrame, sizeof(stackFrame));

		storage.Read(&callingSystemFunction, sizeof(callingSystemFunction)); // TODO: serialize the function declaration
		storage.Read(&initialFunction, sizeof(initialFunction)); // TODO: serialize the function declaration
		storage.Read(&originalStackPointer, sizeof(originalStackPointer));
		storage.Read(&argumentSize, sizeof(argumentSize));
		storage.Read(&valueRegister, sizeof(valueRegister));
		storage.Read(&objectRegister, sizeof(objectRegister)); // TODO: serialize the object pointer
		storage.Read(&objectType, sizeof(objectType)); // TODO: serialize the type info

		// Set the state registers
		r = ctx->SetStateRegisters(0, callingSystemFunction, initialFunction, originalStackPointer, argumentSize, valueRegister, objectRegister, objectType);
		if (r < 0)
			TEST_FAILED;

		// Then restore the values on the stack
		for (int n = ctx->GetCallstackSize() - 1; n >= 0; n--)
		{
			r = ctx->GetCallStateRegisters(n, 0, 0, 0, 0, 0);
			if (r == asSUCCESS)
			{
				// Deserialize the local variables
				int thisTypeId;
				storage.Read(&thisTypeId, sizeof(thisTypeId)); // TODO: Serialize the type id
				// TODO: set this type id in the context? Do we really need to serialize this?
				int varCount;
				storage.Read(&varCount, sizeof(varCount)); // TODO: This doesn't have to be serialized. We can get it from the context
				for (int v = 0; v < varCount; v++)
				{
					if (!ctx->IsVarInScope(v, n))
						continue;

					bool isValue;
					storage.Read(&isValue, sizeof(isValue));
					if (isValue)
					{
						int typeId;
						void* var = ctx->GetAddressOfVar(v, n);
						storage.Read(&typeId, sizeof(typeId)); // TODO: serialize the type id
						if (typeId == asTYPEID_INT32)
							storage.Read(var, sizeof(asINT32));
						else if (typeId == asTYPEID_DOUBLE)
							storage.Read(var, sizeof(double));
						else if (typeId == ctx->GetEngine()->GetTypeIdByDecl("string"))
						{
							bool isNull;
							storage.Read(&isNull, sizeof(bool));
							if (isNull)
							{
								// set the var as null
								// TODO: Is this needed?
							}
							else
							{
								std::string value;
								asUINT len;
								storage.Read(&len, sizeof(asUINT));
								value.resize(len);
								storage.Read(&value[0], len);

								// Check if the variable is stored on the heap
								bool isOnHeap = false;
								ctx->GetVar(v, n, 0, 0, 0, &isOnHeap);
								if( isOnHeap )
								{
									// When variable is stored on the heap, GetAddressOfVar should be called with doNotDerefence = true, 
									// so the actual reference is obtained and can be set to the newly allocated
									// If stored on the heap the memory needs to be allocated
									var = ctx->GetAddressOfVar(v, n, true, true);

									// Allocate memory for the string on the heap and set the variable to refer to it
									*reinterpret_cast<std::string**>(var) = reinterpret_cast<std::string*>(asAllocMem(sizeof(std::string)));

									var = *reinterpret_cast<void**>(var);
								}
								else
									var = ctx->GetAddressOfVar(v, n, false, true);

								// Initialize the string
								new (var) std::string(value);
							}
						}
						else
							TEST_FAILED; // TODO: Add support for more types
					}
					else
					{
						// Deserialize the reference
						void** var = (void**)ctx->GetAddressOfVar(v, n, true);
						r = DeserializeAddress(var);
						if (r < 0)
							TEST_FAILED;
					}
				}

				// Deserialize values pushed on the stack
				int stackValuesCount = ctx->GetArgsOnStackCount(n);
				int storedCount;
				storage.Read(&storedCount, sizeof(storedCount));
				if (storedCount != stackValuesCount)
					TEST_FAILED;
				for (int v = 0; v < stackValuesCount; v++)
				{
					int typeId;
					asUINT flags;
					void* address;
					ctx->GetArgOnStack(n, v, &typeId, &flags, &address);

					if ((flags & asTM_INOUTREF) || (typeId & asTYPEID_MASK_OBJECT))
					{
						r = DeserializeAddress((void**)address);
						if (r < 0)
							TEST_FAILED;
					}
					else if (typeId == asTYPEID_UINT64 || typeId == asTYPEID_INT64 || typeId == asTYPEID_DOUBLE)
						storage.Read(address, 8);
					else
						storage.Read(address, 4);
				}
			}
		}

		r = ctx->FinishDeserialization();
		if (r < 0)
			TEST_FAILED;

		return fail ? -1 : 0;
	}

	int SerializeAddress(void *addr, int varIndex, int stackLevel, bool isArgOnStack)
	{
		bool fail = false;

		// Iterate through the variables in the context to see if the address refers to any of them
		for (int n = m_ctx->GetCallstackSize() - 1; n >= 0; n--)
		{
			for (int v = 0; v < m_ctx->GetVarCount(n); v++)
			{
				// Don't match the variable itself
				void* addr2 = m_ctx->GetAddressOfVar(v, n, false, true);
				if (addr == addr2 && !(v == varIndex && n == stackLevel) && !isArgOnStack)
				{
					int refType = 0; // local variable
					storage.Write(&refType, sizeof(refType));
					storage.Write(&n, sizeof(n)); // TODO: invert order of stackLevel so that 0 is the first function (this is important because when deserializing the function stack is built up as it is read, and the stack level will not match)
					storage.Write(&v, sizeof(v));
					return 0;
				}
			}
		}

		// For args on the stack, check if it is a placeholder for a local variable rather than an addres
		if (isArgOnStack)
		{
			for (int v = 0; v < m_ctx->GetVarCount(stackLevel) ; v++)
			{
				int stackOffset = 0;
				m_ctx->GetVar(v, stackLevel, 0, 0, 0, 0, &stackOffset);
				void* addr2 = m_ctx->GetAddressOfVar(v, stackLevel);
				if (addr2 && (asPWORD)stackOffset == (asPWORD)addr)
				{
					int refType = 2; // variable placeholder
					storage.Write(&refType, sizeof(refType));
					// TODO: Instead of storing the stack offset, store the variable index. This way it will be 32bit/64bit agnostic
					storage.Write(&stackOffset, sizeof(stackOffset));
					return 0;
				}
			}
		}

		// It's not a local variable, save the address as is for now
		int refType = 1; // absolute address
		storage.Write(&refType, sizeof(refType));
		storage.Write(&addr, sizeof(addr));

		// TODO: The address might reference a global variable or literal string constant

		// TODO: Iterate through previously saved objects to see if the address refers to any of them

		// TODO: If the address refers to a member of an object or an element of a container it may be tricky to find

		return fail ? -1 : 0;
	}

	int DeserializeAddress(void** addr)
	{
		bool fail = false;

		int refType;
		storage.Read(&refType, sizeof(refType));
		if (refType == 0) // local variable
		{
			int stackLevel, varIndex;
			storage.Read(&stackLevel, sizeof(stackLevel));
			storage.Read(&varIndex, sizeof(varIndex));
			*addr = m_ctx->GetAddressOfVar(varIndex, stackLevel, false, true);
		}
		else if (refType == 1) // absolute address
		{
			void* storedAddress;
			storage.Read(&storedAddress, sizeof(storedAddress));
			*addr = storedAddress;
		}
		else if (refType == 2) // variable placeholder
		{
			int stackOffset;
			storage.Read(&stackOffset, sizeof(stackOffset));
			*(asPWORD*)addr = (asPWORD)stackOffset;
		}
		else
			TEST_FAILED;

		return fail ? -1 : 0;
	}

	CBytecodeStream storage;
	asIScriptContext* m_ctx;
};

asIScriptObject* CreateScriptClassObject(asIScriptEngine* pEngine, asIScriptContext* pContext, const std::string& ClassName)
{
	asIScriptFunction* factory = nullptr;
	{
		asIScriptModule* module = pEngine->GetModule("test");
		asITypeInfo* type = module->GetTypeInfoByDecl((const char*)ClassName.c_str());
		if (!type)
		{
			assert(false);
			return nullptr;
		}

		auto FactoryDecl = ClassName;
		FactoryDecl += " @";
		FactoryDecl += ClassName;
		FactoryDecl += "()";
		factory = type->GetFactoryByDecl((const char*)FactoryDecl.c_str());
		if (!factory)
		{
			assert(false);
			return nullptr;
		}
	}

	pContext->Prepare(factory);

	pContext->Execute();

	asIScriptObject* obj = *(asIScriptObject**)pContext->GetAddressOfReturnValue();

	obj->AddRef();

	return obj;
}


class GameComponentBase {
public:
	GameComponentBase() = default;
	GameComponentBase(asIScriptObject* pScriptObj) : m_pScriptObj(pScriptObj) {};
	virtual ~GameComponentBase()
	{
		m_pScriptObj->Release();
	}

	virtual int Update(asIScriptContext* pContext, float fDeltaT)
	{
		auto pTypeInfo = m_pScriptObj->GetObjectType();
		asIScriptFunction* pFunc = pTypeInfo->GetMethodByDecl("void Update( float )");

		auto r = pContext->Prepare(pFunc); assert(r >= 0);
		r = pContext->SetObject(m_pScriptObj); assert(r >= 0);
		r = pContext->SetArgFloat(0, fDeltaT); assert(r >= 0);
		r = pContext->Execute(); assert(r == 0);

		return r;
	}

protected:
	asIScriptObject* m_pScriptObj;
};

// A class to increase the stack size used by scripts.
class TestData
{
public:

private:
	float TEST[1024 * 4];
};

template< typename T > static void ConstructorFunc(void* memory)
{
	// Initialize the pre-allocated memory by calling the
	// object constructor with the placement-new operator
	new(memory) T();
}
template< typename T > static void DestructorFunc(void* memory)
{
	// Uninitialize the memory by calling the object destructor
	((T*)memory)->~T();
}

template< typename T > bool RegisterClassConstructAndDestruct(asIScriptEngine* pEngine, const char* pClassName)
{
	auto r = pEngine->RegisterObjectBehaviour(pClassName, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructorFunc<T>), asCALL_CDECL_OBJLAST);
	assert(r >= 0);
	r = pEngine->RegisterObjectBehaviour(pClassName, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructorFunc<T>), asCALL_CDECL_OBJLAST);
	assert(r >= 0);

	return true;
}

template< typename T > bool RegisterClassOpAssign(asIScriptEngine* pEngine, const char* pClassName)
{
	std::string opFunc = pClassName;
	opFunc += "& opAssign(const ";
	opFunc += pClassName;
	opFunc += " &in )";
	auto r = pEngine->RegisterObjectMethod(pClassName, opFunc.c_str(), asMETHODPR(T, operator =, (const T&), T&), asCALL_THISCALL); assert(r >= 0);

	return true;
}

bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	int r;

	// Test reuse of context after the stack has grown
	// Reported by Doi Hiroshi
	{
		auto engine = asCreateScriptEngine();

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);

		// regist game use type and function.
		{
			r = engine->RegisterInterface("IGameComponentScript"); assert(r >= 0);

			r = engine->RegisterObjectType("TestData", sizeof(TestData), asOBJ_VALUE | asGetTypeTraits<TestData>()); assert(r >= 0);
			RegisterClassConstructAndDestruct<TestData>(engine, "TestData");
			RegisterClassOpAssign<TestData>(engine, "TestData");
		}

		// build script.
		{
			auto mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

			mod->AddScriptSection("char",
				R"(
class Character : GameComponentScriptBase
{
	private TestData m_TEST;

	bool Init() override	
	{
		return true;
	}
	void Term() override {}

	void Update( float fDeltaT ) override
	{
		TestData TEST;// = m_TEST;
	}

	void Draw() override {}

}

class Enemy : GameComponentScriptBase
{
	private TestData m_TEST;

	bool Init() override	
	{
		return true;
	}
	void Term() override {}

	void Update( float fDeltaT ) override
	{
		TestData TEST = m_TEST;
	}

	void Draw() override {}

})");
			mod->AddScriptSection("game",
				R"(
class GameComponentScriptBase : IGameComponentScript
{
	bool Init() { return true; }
	void Term() {}

	void Update( float fDeltaT ){}
	void Draw(){}
}
)");
			r = mod->Build();
			if (r < 0)
				TEST_FAILED;
		}

		// create context.
		asIScriptContext* m_pContext = engine->CreateContext();

		// run.
		{
			// create game object.
			GameComponentBase Chara(CreateScriptClassObject(engine, m_pContext, "Character"));
			GameComponentBase Enemy(CreateScriptClassObject(engine, m_pContext, "Enemy"));

			float fDeltaT = 0.166666f;

			// When you run this script, the StackIndex will increase to 3.
			r = Chara.Update(m_pContext, fDeltaT);
			if (r != asEXECUTION_FINISHED)
				TEST_FAILED;

			asDWORD stackIndex = 0;
			m_pContext->GetCallStateRegisters(0, 0, 0, 0, 0, &stackIndex);
			if (stackIndex != 3)
				TEST_FAILED;

			// If you run this script,
			// the StackIndex will start at 3 and memory will be corrupted.
			r = Enemy.Update(m_pContext, fDeltaT);
			if (r != asEXECUTION_FINISHED)
				TEST_FAILED;
		}

		// shutdown.
		{
			m_pContext->Release();
			engine->ShutDownAndRelease();

			if (bout.buffer != "")
			{
				PRINTF("%s", bout.buffer.c_str());
				TEST_FAILED;
			}
		}
	}

	// TODO: The serialized context must be 32bit/64bit agnostic, both for program pointer and stack pointer
	// TODO: Test serializing contexts with nested calls. Although technically possible, it would be a very odd situation since a nested call means the lower execution is actually active, and it isn't possible to deserialize a context in active state since when deserializing it will finish in suspended state
	// TODO: Test serializing references to ref types and handles to ref types
	// TODO: Test serializing class method calls
	// TODO: Test serializing a delegate call
	// TODO: Test serializing context and saving bytecode, both with and without debug info (it must be possible to serialize even without debug info)
	// TODO: Test serializing context within template factory stub (function parameters should be properly serialized)
	// TODO: Test serializing when multiple stack memory blocks are used, and also when deserializing on a contex with different initContextStackSize
	// ref: https://github.com/SpehleonLP/Zodiac
	//      https://github.com/SpehleonLP/Zodiac/blob/main/src/z_zodiaccontext.cpp

	// Test with asEP_ALLOW_UNSAFE_REFERENCES on
	// https://www.gamedev.net/forums/topic/714777-asep_allow_unsafe_references-caused-ccontextserializer-to-crash/5458480/
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);
		g_buf = "";
		engine->RegisterGlobalFunction("void pause()", asFUNCTION(pause), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void main() \n"
			"{ \n"
			// when level2 is called, a reference to 'd' is already pushed on the stack, an index to the variable holding 'c' is pushed on the stack, and the value 3.14
			// the string 'c' is also stored in a local variable where the object is actually allocated on the heap
			// when level3 is called the value 42 is already pushed on the stack
			"  string c = level1(level2(level3('test'), 42), 3.14, 'c', 'd'); \n"
			"  print('result: '+c+'\\n'); \n"
			"} \n"
			"string level1(const string &in a, double b, string c, const string &in d) \n"
			"{ \n"
			"  return a+b+c+d; \n"
			"} \n"
			"string level2(const string &in a, double b) \n"
			"{ \n"
			"  return a+b; \n"
			"} \n"
			"string level3(const string &in a) \n"
			"{ \n"
			"  pause(); \n" // this is where the script will be serialized
			"  return a; \n"
			"} \n"
		);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		bout.buffer = "";

		// Start execution
		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("main"));
		r = ctx->Execute();
		if (r != asEXECUTION_SUSPENDED)
			TEST_FAILED;

		// Serialize the context
		CContextSerializer storage;
		r = storage.Serialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Destroy the context
		ctx->Release();

		// Create a new context
		ctx = engine->CreateContext();

		// Deserialize the context
		r = storage.Deserialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r != asEXECUTION_FINISHED)
		{
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("Exception: %s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if (g_buf != "result: test423.14cd\n")
		{
			PRINTF("%s", g_buf.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test serializing context in the middle of setting up arguments for a function call
	// Arguments are pushed on the stack from last to first.
	// For object types first a placeholder is pushed on the stack to give the position of the temporary variable holding the object and only right before the call is the placeholder replaced with the object pointer.
	// When counting values on the stack first the code can compare the stack pointer with the highest variable offset, and if higher then some value is pushed on the stack. 
	// If the count is done at a lower stack level then the count must substract the arguments of the called function before counting, then the code should iterate through the bytecode to find the 
	// function that will be called to determine the type of the arguments on the stack, this must be done iteratively until all the stack values have been identified as there can potentially be 
	// arguments for different functions prepared.
	// Test with value types passed by value and by reference
	// TODO: The arguments on the stack must be 32bit/64bit agnostic, especifically placeholders need to adjusted
	// TODO: test complex expressions with ternary operators to guarantee that branches are also properly handled
	// TODO: Test with object methods that return value types by value (i.e. on stack)
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);
		g_buf = "";
		engine->RegisterGlobalFunction("void pause()", asFUNCTION(pause), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void main() \n"
			"{ \n"
			// when level2 is called, a reference to 'd' is already pushed on the stack, an index to the variable holding 'c' is pushed on the stack, and the value 3.14
			// the string 'c' is also stored in a local variable where the object is actually allocated on the heap
			// when level3 is called the value 42 is already pushed on the stack
			"  string c = level1(level2(level3('test'), 42), 3.14, 'c', 'd'); \n"
			"  print('result: '+c+'\\n'); \n"
			"} \n"
			"string level1(const string &in a, double b, string c, const string &in d) \n"
			"{ \n"
			"  return a+b+c+d; \n"
			"} \n"
			"string level2(const string &in a, double b) \n"
			"{ \n"
			"  return a+b; \n"
			"} \n"
			"string level3(const string &in a) \n"
			"{ \n"
			"  pause(); \n" // this is where the script will be serialized
			"  return a; \n"
			"} \n"
		);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		bout.buffer = "";

		// Start execution
		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("main"));
		r = ctx->Execute();
		if (r != asEXECUTION_SUSPENDED)
			TEST_FAILED;

		// Serialize the context
		CContextSerializer storage;
		r = storage.Serialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Destroy the context
		ctx->Release();

		// Create a new context
		ctx = engine->CreateContext();

		// Deserialize the context
		r = storage.Deserialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r != asEXECUTION_FINISHED)
		{
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("Exception: %s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if (g_buf != "result: test423.14cd\n")
		{
			PRINTF("%s", g_buf.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test serializing a context with a value type objects
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);
		g_buf = "";
		engine->RegisterGlobalFunction("void pause()", asFUNCTION(pause), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
						"void main() \n"
						"{ \n"
						"  int a = 3; \n"
						"  double b = .14; \n"
						"  string c = doSomething(a, b, 'test'); \n" // 'test' is a global address
						"  print('result: '+c+'\\n'); \n"
						"} \n"
						"string doSomething(int a, double b, const string &in s) \n" // s is referencing the global address to the string literal 'test'
						"{ \n"
						"  string result = a + b + ''; \n"
						"  pause(); \n" // this is where the script will be serialized
						"  return result + s; \n"
						"} \n"
		);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		bout.buffer = "";

		// Start execution
		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("main"));
		r = ctx->Execute();
		if (r != asEXECUTION_SUSPENDED)
			TEST_FAILED;

		// Serialize the context
		CContextSerializer storage;
		r = storage.Serialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Destroy the context
		ctx->Release();

		// Create a new context
		ctx = engine->CreateContext();

		// Deserialize the context
		r = storage.Deserialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r != asEXECUTION_FINISHED)
		{
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("Exception: %s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if (g_buf != "result: 3.14test\n")
		{
			PRINTF("%s", g_buf.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test serializing context with temporary variables
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);
		g_buf = "";
		engine->RegisterGlobalFunction("void pause()", asFUNCTION(pause), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void main() \n"
			"{ \n"
			"  int a = 3; \n"
			"  double b = .14; \n"
			"  double c = a + b + doSomething(a, b); \n"
			"  print('result: '+c+'\\n'); \n"
			"} \n"
			"double doSomething(int a, double b) \n"
			"{ \n"
			"  pause(); \n" // this is where the script will be serialized
			"  return a+b; \n"
			"} \n"
		);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		bout.buffer = "";

		// Start execution
		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("main"));
		r = ctx->Execute();
		if (r != asEXECUTION_SUSPENDED)
			TEST_FAILED;

		// Serialize the context
		CContextSerializer storage;
		r = storage.Serialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Destroy the context
		ctx->Release();

		// Create a new context
		ctx = engine->CreateContext();

		// Deserialize the context
		r = storage.Deserialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r != asEXECUTION_FINISHED)
		{
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("Exception: %s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if (g_buf != "result: 6.28\n")
		{
			PRINTF("%s", g_buf.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test invalid deserialization (must not crash)
	// TODO: Interrupt deserialization in the middle, and then clean up context. Can the context handle this?
	// TODO: It may not always be possible to avoid a crash depending on why and at what point the deserialization failed. Perhaps it is best just to report a critical error and not do any cleanup at all?
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		// Create a new context
		asIScriptContext *ctx = engine->CreateContext();

		// Deserialize the context with no content
		r = ctx->StartDeserialization();
		if (r < 0)
			TEST_FAILED;

		r = ctx->FinishDeserialization();
		if (r >= 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r == asEXECUTION_FINISHED)
			TEST_FAILED;
		ctx->Release();

		if (bout.buffer != " (0, 0) : Error   : Failed in call to function 'FinishDeserialization' with 'No function set' (Code: asCONTEXT_NOT_PREPARED, -4)\n"
			               " (0, 0) : Error   : Failed in call to function 'Execute' (Code: asCONTEXT_NOT_PREPARED, -4)\n")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test serializing a context with references in parameters
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);
		g_buf = "";
		engine->RegisterGlobalFunction("void pause()", asFUNCTION(pause), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void main() \n"
			"{ \n"
			"  int a = 3; \n"
			"  double b = .14; \n"
			"  int c = doSomething(a, b); \n"
			"  print('result: '+c+'\\n'); \n"
			"} \n"
			"int doSomething(const int &in a, const double &in b) \n"
			"{ \n"
			"  pause(); \n" // this is where the script will be serialized
			"  return int(a + b); \n"
			"} \n"
		);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		bout.buffer = "";

		// Start execution
		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("main"));
		r = ctx->Execute();
		if (r != asEXECUTION_SUSPENDED)
			TEST_FAILED;

		// Serialize the context
		CContextSerializer storage;
		r = storage.Serialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Destroy the context
		ctx->Release();

		// Create a new context
		ctx = engine->CreateContext();

		// Deserialize the context
		r = storage.Deserialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r != asEXECUTION_FINISHED)
		{
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("Exception: %s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if (g_buf != "result: 3\n")
		{
			PRINTF("%s", g_buf.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test serializing a context with a trivial script (no temporary variables, no objects)
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);
		g_buf = "";
		engine->RegisterGlobalFunction("void pause()", asFUNCTION(pause), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void main() \n"
			"{ \n"
			"  int a = 3; \n"
			"  double b = .14; \n"
			"  int c = doSomething(a, b); \n"
			"  print('result: '+c+'\\n'); \n"
			"} \n"
			"int doSomething(int a, double b) \n"
			"{ \n"
			"  pause(); \n" // this is where the script will be serialized
			"  return int(a + b); \n"
			"} \n"
		);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		bout.buffer = "";

		// Start execution
		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("main"));
		r = ctx->Execute();
		if (r != asEXECUTION_SUSPENDED)
			TEST_FAILED;

		// Serialize the context
		CContextSerializer storage;
		r = storage.Serialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Destroy the context
		ctx->Release();

		// Create a new context
		ctx = engine->CreateContext();

		// Deserialize the context
		r = storage.Deserialize(ctx);
		if (r < 0)
			TEST_FAILED;

		// Resume execution
		r = ctx->Execute();
		if (r != asEXECUTION_FINISHED)
		{
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("Exception: %s\n", GetExceptionInfo(ctx).c_str());
			TEST_FAILED;
		}
		ctx->Release();

		if (g_buf != "result: 3\n")
		{
			PRINTF("%s", g_buf.c_str());
			TEST_FAILED;
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test clean-up of variables after aborting a context
	// https://www.gamedev.net/forums/topic/711858-return-value-destructed-without-being-initialized/5445727/
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void abort()", asFUNCTION(abort), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void Render() { \n"
			"  loop(); \n"
			"} \n"

			"string loop() { \n"
			"  float zero = 0; \n"
			"  float one = 1; \n"
			"  if (one <= zero) return 'aaaa!'; \n"

			"  while (true) {abort();} \n"

			"  return 'bbbb!'; \n"
			"} \n" );
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asIScriptContext* ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("Render"));
		r = ctx->Execute();
		if (r != asEXECUTION_ABORTED)
			TEST_FAILED;
		ctx->Unprepare();
		ctx->Release();

		engine->ShutDownAndRelease();
	}

	// Success
	return fail;
}

} // namespace

