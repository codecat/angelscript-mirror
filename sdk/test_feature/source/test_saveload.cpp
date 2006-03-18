//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include <vector>

namespace TestSaveLoad
{

#define TESTNAME "TestSaveLoad"


class CBytecodeStream : public asIBinaryStream
{
public:
	CBytecodeStream() {wpointer = 0;rpointer = 0;}

	void Write(void *ptr, int size) {buffer.resize(buffer.size() + size); memcpy(&buffer[wpointer], ptr, size); wpointer += size;}
	void Read(void *ptr, int size) {memcpy(ptr, &buffer[rpointer], size); rpointer += size;}

	int rpointer;
	int wpointer;
	std::vector<asBYTE> buffer;
};


static const char *script1 =
"import void Test() from \"DynamicModule\";   \n"
"OBJ g_obj;                                   \n"
"void main()                                  \n"
"{                                            \n"
"  Test();                                    \n"
"}                                            \n"
"void TestObj(OBJ & obj)                      \n"
"{                                            \n"
"}                                            \n";

static const char *script2 =
"void Test()                               \n"
"{                                         \n"
"  int[] a(3);                             \n"
"  a[0] = 23;                              \n"
"  a[1] = 13;                              \n"
"  a[2] = 34;                              \n"
"  if( a[0] + a[1] + a[2] == 23+13+34 )    \n"
"    number = 1234567890;                  \n"
"}                                         \n";

bool Test()
{
	bool fail = false;

	int number = 0;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalProperty("int number", &number);

	engine->RegisterObjectType("OBJ", sizeof(int), asOBJ_PRIMITIVE);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);
	engine->Build(0, &out);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule", &out);

	// Bind all functions that the module imports
	engine->BindAllImportedFunctions(0);

	// Save the compiled byte code
	CBytecodeStream stream;
	engine->SaveByteCode(0, &stream);

	// Load the compiled byte code into the same module
	engine->LoadByteCode(0, &stream);

	// Bind the imported functions again
	engine->BindAllImportedFunctions(0);

	engine->ExecuteString(0, "main()", &out);

	engine->Release();

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		fail = true;
	}

	// Success
	return fail;
}

} // namespace

