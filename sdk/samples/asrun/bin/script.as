/*

  Script must have 'int main()' or 'void main()' as the entry point.

  Some functions that are available:

   void           print(const string &in str);
   array<string> @getCommandLineArgs();

  Some objects that are available:

   string
   array<T>
   file

*/

string g_str = getDefaultString();

int main()
{
  array<string> @args = getCommandLineArgs();

  print("Received the following args : " + join(args, "|") + "\n");

  function();

  return 0;
}

void function()
{
  print("Currently in a different function\n");

  int n = 0;
  {
    int n = 1;
    string s = "hello";
  }
  {
    int n = 2;
  }
}

string getDefaultString()
{
  return "default";
}