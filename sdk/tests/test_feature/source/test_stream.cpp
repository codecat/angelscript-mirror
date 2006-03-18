#include "utils.h"
#include <sstream>
using namespace std;

namespace TestStream
{

#define TESTNAME "TestStream"

stringstream stream;

class CScriptStream
{
public:
	CScriptStream();
	~CScriptStream();

	CScriptStream &operator=(const CScriptStream&);

	stringstream *s;
};

CScriptStream &operator<<(CScriptStream &s, const string &other)
{
//	printf("(%X) << \"%s\"\n", &s, other.c_str());

	*s.s << other;
	return s;
}

// TODO: Make this work
/*
CScriptStream &operator>>(CScriptStream &s, string &other)
{
	*s.s >> other;
	return s;
}
*/

CScriptStream::CScriptStream()
{
//	printf("new (%X)\n", this);

	s = &stream;
}

CScriptStream::~CScriptStream()
{
//	printf("del (%X)\n", this);
}

CScriptStream &CScriptStream::operator=(const CScriptStream &other)
{
//	printf("(%X) = (%X)\n", this, &other);

	s = other.s;

	return *this;
}

void CScriptStream_Construct(CScriptStream *o)
{
	new(o) CScriptStream;
}

void CScriptStream_Destruct(CScriptStream *o)
{
	o->~CScriptStream();
}

static const char *script1 =
"void Test()                       \n"
"{                                 \n"
"  stream s;                       \n"
"  s << \"a\" << \"b\" << \"c\";   \n"
"}                                 \n";
// TODO: Make this work
/*
"void Test2()                      \n"
"{                                 \n"
"  stream s;                       \n"
"  s << \"a b c\";                 \n"
"  string a,b,c;                   \n"
"  s >> a >> b >> c;               \n"
"}                                 \n";*/

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	engine->RegisterObjectType("stream", sizeof(CScriptStream), asOBJ_CLASS_CDA);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(CScriptStream_Construct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(CScriptStream_Destruct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("stream", asBEHAVE_ASSIGNMENT, "stream &f(const stream &in)", asMETHOD(CScriptStream, operator=), asCALL_THISCALL);
	engine->RegisterGlobalBehaviour(asBEHAVE_BIT_SLL, "stream &f(stream &in, string &in)", asFUNCTIONPR(operator<<, (CScriptStream &s, const string &other), CScriptStream &), asCALL_CDECL);
	// TODO: Make this work
//	engine->RegisterGlobalBehaviour(asBEHAVE_BIT_SRL, "stream &f(stream &in, string &out)", asFUNCTIONPR(operator>>, (CScriptStream &s, string &other), CScriptStream &), asCALL_CDECL);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	engine->SetCommonMessageStream(&out);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	if( stream.str() != "abc" )
	{
		printf("%s: Failed to create the correct stream\n", TESTNAME);
		fail = true;
	}

	stream.clear();
	

	engine->Release();

	// Success
	return fail;
}

} // namespace

