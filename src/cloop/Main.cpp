#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include <stdio.h>

using std::auto_ptr;
using std::deque;
using std::map;
using std::pair;
using std::stack;
using std::string;
using std::vector;
using std::runtime_error;
using std::cerr;
using std::endl;


#define DUMMY_VTABLE	1
#define DUMMY_INSTANCE	1
#define TOKEN(c)	static_cast<::Token::Type>(c)


class Token
{
public:
	enum Type
	{
		TYPE_EOF = 256,
		TYPE_IDENTIFIER,
		// keywords
		TYPE_INTERFACE,
		// types
		TYPE_VOID,
		TYPE_INT
	};

public:
	Type type;
	string text;
};


class Lexer
{
public:
	Lexer(const string& filename)
	{
		in = fopen(filename.c_str(), "r");

		if (!in)
			throw runtime_error("Input file not found.");
	}

	Token& getToken(Token& token)
	{
		if (!tokens.empty())
		{
			token = tokens.top();
			tokens.pop();
			return token;
		}

		token.text = "";

		int c;

		while ((c = fgetc(in)) == ' ' || c == '\t' || c == '\r' || c == '\n')
			;

		if (c == -1)
		{
			token.type = Token::TYPE_EOF;
			return token;
		}

		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
		{
			while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
				   (c >= '0' && c <= '9'))
			{
				token.text += c;
				c = fgetc(in);
			}

			ungetc(c, in);

			if (token.text == "interface")
				token.type = Token::TYPE_INTERFACE;
			else if (token.text == "void")
				token.type = Token::TYPE_VOID;
			else if (token.text == "int")
				token.type = Token::TYPE_INT;
			else
				token.type = Token::TYPE_IDENTIFIER;
		}
		else
		{
			token.type = static_cast<Token::Type>(c);
			token.text = c;
		}

		return token;
	}

	void pushToken(const Token& token)
	{
		tokens.push(token);
	}

	~Lexer()
	{
		fclose(in);
	}

private:
	FILE* in;
	stack<Token> tokens;
};


struct Parameter
{
	string name;
	Token type;
};


struct Method
{
	string name;
	Token returnType;
	vector<Parameter*> parameters;
};


struct Interface
{
	Interface()
		: super(NULL)
	{
	}

	string name;
	Interface* super;
	vector<Method*> methods;
};


class Parser
{
public:
	Parser(Lexer* lexer)
		: lexer(lexer)
	{
	}

	void parse()
	{
		Token token;

		while (getToken(token, Token::TYPE_INTERFACE, true).type != Token::TYPE_EOF)
		{
			Interface* interface = new Interface();
			interfaces.push_back(interface);

			interface->name = getToken(token, Token::TYPE_IDENTIFIER).text;
			interfacesByName.insert(pair<string, Interface*>(interface->name, interface));

			if (lexer->getToken(token).type == ':')
			{
				string superName = getToken(token, Token::TYPE_IDENTIFIER).text;
				auto it = interfacesByName.find(superName);
				interface->super = it->second;
			}
			else
				lexer->pushToken(token);

			getToken(token, TOKEN('{'));

			while (lexer->getToken(token).type != TOKEN('}'))
			{
				lexer->pushToken(token);

				Method* method = new Method();
				interface->methods.push_back(method);

				method->returnType = parseType(token);
				method->name = getToken(token, Token::TYPE_IDENTIFIER).text;
				getToken(token, TOKEN('('));

				if (lexer->getToken(token).type != TOKEN(')'))
				{
					lexer->pushToken(token);

					while (true)
					{
						Parameter* parameter = new Parameter();
						method->parameters.push_back(parameter);

						parameter->type = parseType(token);
						parameter->name = getToken(token, Token::TYPE_IDENTIFIER).text;

						lexer->getToken(token);
						lexer->pushToken(token);

						if (token.type == TOKEN(')'))
							break;

						getToken(token, TOKEN(','));
					}

					getToken(token, TOKEN(')'));
				}

				getToken(token, TOKEN(';'));
			}
		}
	}

private:
	Token& getToken(Token& token, Token::Type expected, bool allowEof = false)
	{
		lexer->getToken(token);

		if (token.type != expected && !(allowEof && token.type == Token::TYPE_EOF))
			throw runtime_error(string("Syntax error at '") + token.text + "'.");

		return token;
	}

	Token& parseType(Token& token)
	{
		lexer->getToken(token);

		switch (token.type)
		{
			case Token::TYPE_VOID:
			case Token::TYPE_INT:
				break;

			default:
				throw runtime_error(string("Syntax error at '") + token.text + "'. Expected a type.");
		}

		return token;
	}

public:
	vector<Interface*> interfaces;
	map<string, Interface*> interfacesByName;

private:
	Lexer* lexer;
};


class Generator
{
public:
	virtual ~Generator()
	{
	}

	virtual void generate() = 0;
};


class FileGenerator : public Generator
{
public:
	FileGenerator(const string& filename)
	{
		out = fopen(filename.c_str(), "w+");
	}

	virtual ~FileGenerator()
	{
		fclose(out);
	}

protected:
	FILE* out;
};


class CppGenerator : public FileGenerator
{
public:
	CppGenerator(const string& filename, Parser* parser, const string& headerGuard,
			const string& className)
		: FileGenerator(filename),
		  parser(parser),
		  headerGuard(headerGuard),
		  className(className)
	{
	}

public:
	virtual void generate()
	{
		fprintf(out, "#ifndef %s\n", headerGuard.c_str());
		fprintf(out, "#define %s\n\n", headerGuard.c_str());
		fprintf(out, "#include <stdint.h>\n");
		fprintf(out, "#include <stdarg.h>\n\n\n");

		fprintf(out, "template <typename Policy>\n");
		fprintf(out, "class %s\n", className.c_str());
		fprintf(out, "{\n");
		fprintf(out, "private:\n");
		fprintf(out, "\ttemplate <typename T>\n");
		fprintf(out, "\tclass VTableInitializer\n");
		fprintf(out, "\t{\n");
		fprintf(out, "\tpublic:\n");
		fprintf(out, "\t\tVTableInitializer(unsigned version, ...)\n");
		fprintf(out, "\t\t{\n");
		fprintf(out, "\t\t\tva_list va;\n");
		fprintf(out, "\t\t\tva_start(va, version);\n");
		fprintf(out, "\n");
		fprintf(out, "\t\t\tvTable.version = version;\n");
		fprintf(out, "\n");
		fprintf(out, "\t\t\tvoid** end = ((void**) &vTable.version) + version;\n");
		fprintf(out, "\n");
		fprintf(out, "\t\t\tfor (void** p = (void**) &vTable.version; p < end; )\n");
		fprintf(out, "\t\t\t\t*++p = va_arg(va, void*);\n");
		fprintf(out, "\n");
		fprintf(out, "\t\t\tva_end(va);\n");
		fprintf(out, "\t\t}\n");
		fprintf(out, "\n");
		fprintf(out, "\t\tT vTable;\n");
		fprintf(out, "\t};\n");
		fprintf(out, "\n");

		fprintf(out, "public:\n");

		fprintf(out, "\t// Interfaces declarations\n\n");

		for (auto& interface : parser->interfaces)
		{
			if (!interface->super)
				fprintf(out, "\tclass %s\n", interface->name.c_str());
			else
			{
				fprintf(out, "\tclass %s : public %s\n",
					interface->name.c_str(), interface->super->name.c_str());
			}

			fprintf(out, "\t{\n");
			fprintf(out, "\tprotected:\n");

			if (!interface->super)
			{
				fprintf(out, "\t\tstruct VTable\n");
				fprintf(out, "\t\t{\n");
				fprintf(out, "\t\t\tvoid* cloopDummy[%d];\n", DUMMY_VTABLE);
				fprintf(out, "\t\t\tuintptr_t version;\n");
			}
			else
			{
				fprintf(out, "\t\tstruct VTable : public %s::VTable\n",
					interface->super->name.c_str());
				fprintf(out, "\t\t{\n");
			}

			for (auto& method : interface->methods)
			{
				fprintf(out, "\t\t\t%s (*%s)(%s* self",
					method->returnType.text.c_str(), method->name.c_str(), interface->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s %s", parameter->type.text.c_str(), parameter->name.c_str());

				fprintf(out, ");\n");
			}

			fprintf(out, "\t\t};\n");
			fprintf(out, "\n");

			if (!interface->super)
			{
				fprintf(out, "\tprotected:\n");
				fprintf(out, "\t\tvoid* cloopDummy[%d];\n", DUMMY_INSTANCE);
				fprintf(out, "\t\tVTable* cloopVTable;\n");
				fprintf(out, "\n");
			}

			fprintf(out, "\tpublic:\n");
			fprintf(out, "\t\tstatic const int VERSION = sizeof(VTable) / sizeof(void*) - %d;\n",
				(1 + DUMMY_VTABLE));

			unsigned methodNumber = (interface->super ? interface->super->methods.size() : 0);
			for (auto& method : interface->methods)
			{
				fprintf(out, "\n");
				fprintf(out, "\t\t%s %s(", method->returnType.text.c_str(), method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, ", ");

					fprintf(out, "%s %s", parameter->type.text.c_str(), parameter->name.c_str());
				}

				fprintf(out, ")\n");
				fprintf(out, "\t\t{\n");
				fprintf(out, "\t\t\tPolicy::template checkVersion<%d>(this);\n", ++methodNumber);

				fprintf(out, "\t\t\t");

				if (method->returnType.type != Token::TYPE_VOID)
				{
					fprintf(out, "return ");
					//// TODO: Policy::upgrade
				}

				fprintf(out, "static_cast<VTable*>(this->cloopVTable)->%s(this", method->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s", parameter->name.c_str());

				fprintf(out, ");\n");
				fprintf(out, "\t\t}\n");
			}

			fprintf(out, "\t};\n\n");
		}

		fprintf(out, "\t// Interfaces implementations\n");

		for (auto& interface : parser->interfaces)
		{
			deque<Method*> methods;

			for (Interface* p = interface; p; p = p->super)
				methods.insert(methods.begin(), p->methods.begin(), p->methods.end());

			fprintf(out, "\n");
			fprintf(out, "\ttemplate <typename Name, typename Base>\n");
			fprintf(out, "\tclass %sBaseImpl : public Base\n", interface->name.c_str());
			fprintf(out, "\t{\n");
			fprintf(out, "\tpublic:\n");
			fprintf(out, "\t\t%sBaseImpl()\n", interface->name.c_str());
			fprintf(out, "\t\t{\n");
			fprintf(out, "\t\t\tstatic VTableInitializer<typename Base::VTable> vTableInit(\n");
			fprintf(out, "\t\t\t\tBase::VERSION");

			for (auto& method : methods)
			{
				fprintf(out, ",\n");
				fprintf(out, "\t\t\t\t&Name::cloop%sDispatcher", method->name.c_str());
			}

			fprintf(out, "\n");
			fprintf(out, "\t\t\t);\n\n");
			fprintf(out, "\t\t\tthis->cloopVTable = &vTableInit.vTable;\n");
			fprintf(out, "\t\t}\n");

			for (auto& method : interface->methods)
			{
				fprintf(out, "\n");
				fprintf(out, "\t\tstatic %s cloop%sDispatcher(%s* self",
					method->returnType.text.c_str(), method->name.c_str(), interface->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s %s", parameter->type.text.c_str(), parameter->name.c_str());

				fprintf(out, ") throw()\n");
				fprintf(out, "\t\t{\n");
				fprintf(out, "\t\t\t");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, "return ");

				fprintf(out, "static_cast<Name*>(self)->Name::%s(", method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, ", ");

					fprintf(out, "%s", parameter->name.c_str());
				}

				fprintf(out, ");\n");
				fprintf(out, "\t\t}\n");
			}

			fprintf(out, "\t};\n\n");

			if (!interface->super)
			{
				fprintf(out, "\ttemplate <typename Name, typename Base = %s>\n",
					interface->name.c_str());
			}
			else
			{
				fprintf(out,
					"\ttemplate <typename Name, typename Base = %sImpl<Name, %s> >\n",
					interface->super->name.c_str(),
					interface->name.c_str());
			}

			fprintf(out, "\tclass %sImpl : public %sBaseImpl<Name, Base>\n",
				interface->name.c_str(), interface->name.c_str());
			fprintf(out, "\t{\n");
			fprintf(out, "\tpublic:\n");
			fprintf(out, "\t\tvirtual ~%sImpl()\n", interface->name.c_str());
			fprintf(out, "\t\t{\n");
			fprintf(out, "\t\t}\n");
			fprintf(out, "\n");

			for (auto& method : interface->methods)
			{
				fprintf(out, "\t\tvirtual %s %s(",
					method->returnType.text.c_str(), method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, ", ");

					fprintf(out, "%s %s", parameter->type.text.c_str(), parameter->name.c_str());
				}

				fprintf(out, ") = 0;\n");
			}

			fprintf(out, "\t};\n");
		}

		fprintf(out, "};\n\n");

		fprintf(out, "#endif\t// %s\n", headerGuard.c_str());
	}

private:
	Parser* parser;
	string headerGuard;
	string className;
};


class CHeaderGenerator : public FileGenerator
{
public:
	CHeaderGenerator(const string& filename, Parser* parser, const string& headerGuard)
		: FileGenerator(filename),
		  parser(parser),
		  headerGuard(headerGuard)
	{
	}

public:
	virtual void generate()
	{
		fprintf(out, "#ifndef %s\n", headerGuard.c_str());
		fprintf(out, "#define %s\n\n", headerGuard.c_str());
		fprintf(out, "#include <stdint.h>\n\n");

		fprintf(out, "#ifndef CLOOP_EXTERN_C\n");
		fprintf(out, "#ifdef __cplusplus\n");
		fprintf(out, "#define CLOOP_EXTERN_C extern \"C\"\n");
		fprintf(out, "#else\n");
		fprintf(out, "#define CLOOP_EXTERN_C\n");
		fprintf(out, "#endif\n");
		fprintf(out, "#endif\n\n\n");

		for (auto& interface : parser->interfaces)
		{
			fprintf(out, "#define %s_VERSION %d\n\n", interface->name.c_str(), (1 + DUMMY_VTABLE));

			fprintf(out, "struct %s;\n\n", interface->name.c_str());

			fprintf(out, "struct %sVTable\n", interface->name.c_str());
			fprintf(out, "{\n");
			fprintf(out, "\tvoid* cloopDummy[%d];\n", DUMMY_VTABLE);
			fprintf(out, "\tuintptr_t version;\n");

			deque<Method*> methods;

			for (Interface* p = interface; p; p = p->super)
				methods.insert(methods.begin(), p->methods.begin(), p->methods.end());

			for (auto& method : methods)
			{
				fprintf(out, "\t%s (*%s)(struct %s* self",
					method->returnType.text.c_str(), method->name.c_str(), interface->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s %s", parameter->type.text.c_str(), parameter->name.c_str());

				fprintf(out, ");\n");
			}

			fprintf(out, "};\n\n");

			fprintf(out, "struct %s\n", interface->name.c_str());
			fprintf(out, "{\n");
			fprintf(out, "\tvoid* cloopDummy[%d];\n", DUMMY_INSTANCE);
			fprintf(out, "\tstruct %sVTable* vtable;\n", interface->name.c_str());
			fprintf(out, "};\n\n");

			for (auto& method : methods)
			{
				fprintf(out, "CLOOP_EXTERN_C %s %s_%s(struct %s* self",
					method->returnType.text.c_str(),
					interface->name.c_str(),
					method->name.c_str(),
					interface->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s %s", parameter->type.text.c_str(), parameter->name.c_str());

				fprintf(out, ");\n\n");
			}
		}

		fprintf(out, "#endif\t// %s\n", headerGuard.c_str());
	}

private:
	Parser* parser;
	string headerGuard;
};


class CImplGenerator : public FileGenerator
{
public:
	CImplGenerator(const string& filename, Parser* parser, const string& includeFilename)
		: FileGenerator(filename),
		  parser(parser),
		  includeFilename(includeFilename)
	{
	}

public:
	virtual void generate()
	{
		fprintf(out, "#include \"%s\"\n\n\n", includeFilename.c_str());

		for (auto& interface : parser->interfaces)
		{
			deque<Method*> methods;

			for (Interface* p = interface; p; p = p->super)
				methods.insert(methods.begin(), p->methods.begin(), p->methods.end());

			for (auto& method : methods)
			{
				fprintf(out, "CLOOP_EXTERN_C %s %s_%s(struct %s* self",
					method->returnType.text.c_str(),
					interface->name.c_str(),
					method->name.c_str(),
					interface->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s %s", parameter->type.text.c_str(), parameter->name.c_str());

				fprintf(out, ")\n");
				fprintf(out, "{\n");
				fprintf(out, "\t");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, "return ");

				fprintf(out, "self->vtable->%s(self", method->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s", parameter->name.c_str());

				fprintf(out, ");\n");
				fprintf(out, "}\n\n");
			}
		}
	}

private:
	Parser* parser;
	string includeFilename;
};


class PascalGenerator : public FileGenerator
{
public:
	PascalGenerator(const string& filename, Parser* parser, const string& unitName)
		: FileGenerator(filename),
		  parser(parser),
		  unitName(unitName)
	{
	}

public:
	virtual void generate()
	{
		fprintf(out, "unit %s;\n\n", unitName.c_str());
		fprintf(out, "interface\n\n");
		fprintf(out, "uses Classes;\n\n");
		fprintf(out, "type\n");

		for (auto& interface : parser->interfaces)
		{
			for (auto& method : interface->methods)
			{
				fprintf(out, "\t%s_%sPtr = %s(this: Pointer",
					interface->name.c_str(), method->name.c_str(),
					(method->returnType.type == Token::TYPE_VOID ? "procedure" : "function"));

				for (auto& parameter : method->parameters)
				{
					fprintf(out, "; %s: %s",
						parameter->name.c_str(), convertType(parameter->type).c_str());
				}

				fprintf(out, ")");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, ": %s", convertType(method->returnType).c_str());

				fprintf(out, "; cdecl;\n");
			}
		}

		fprintf(out, "\n");

		for (auto& interface : parser->interfaces)
		{
			fprintf(out, "\t%sVTable = class", interface->name.c_str());

			if (interface->super)
				fprintf(out, "(%sVTable)", interface->super->name.c_str());

			fprintf(out, "\n");

			if (!interface->super)
			{
				fprintf(out, "{$ifndef FPC}\n");
				fprintf(out, "\t\tdummy: PtrInt;\n");
				fprintf(out, "{$endif}\n");
				fprintf(out, "\t\tversion: PtrInt;\n");
			}

			for (auto& method : interface->methods)
			{
				fprintf(out, "\t\t%s: %s_%sPtr;\n", method->name.c_str(), interface->name.c_str(),
					method->name.c_str());
			}

			fprintf(out, "\tend;\n\n");

			fprintf(out, "\t%s = class", interface->name.c_str());

			if (interface->super)
				fprintf(out, "(%s)", interface->super->name.c_str());

			fprintf(out, "\n");

			if (!interface->super)
			{
				fprintf(out, "{$ifndef FPC}\n");
				fprintf(out, "\t\tdummy: PtrInt;\n");
				fprintf(out, "{$endif}\n");
				fprintf(out, "\t\tvTable: %sVTable;\n", interface->name.c_str());
			}

			for (auto& method : interface->methods)
			{
				fprintf(out, "\t\t%s %s(",
					(method->returnType.type == Token::TYPE_VOID ? "procedure" : "function"),
					method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, "; ");

					fprintf(out, "%s: %s",
						parameter->name.c_str(), convertType(parameter->type).c_str());
				}

				fprintf(out, ")");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, ": %s", convertType(method->returnType).c_str());

				fprintf(out, ";\n");
			}

			fprintf(out, "\tend;\n\n");

			fprintf(out, "\t%sImpl = class(%s)\n",
				interface->name.c_str(), interface->name.c_str());
			fprintf(out, "\t\tconstructor create;\n\n");

			deque<Method*> methods;

			for (Interface* p = interface; p; p = p->super)
				methods.insert(methods.begin(), p->methods.begin(), p->methods.end());

			for (auto& method : methods)
			{
				fprintf(out, "\t\t%s %s(",
					(method->returnType.type == Token::TYPE_VOID ? "procedure" : "function"),
					method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, "; ");

					fprintf(out, "%s: %s",
						parameter->name.c_str(), convertType(parameter->type).c_str());
				}

				fprintf(out, ")");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, ": %s", convertType(method->returnType).c_str());

				fprintf(out, "; virtual; abstract;\n");
			}

			fprintf(out, "\n");
			fprintf(out, "\tend;\n\n");
		}

		fprintf(out, "implementation\n\n");

		for (auto& interface : parser->interfaces)
		{
			for (auto& method : interface->methods)
			{
				fprintf(out, "%s %s.%s(",
					(method->returnType.type == Token::TYPE_VOID ? "procedure" : "function"),
					interface->name.c_str(),
					method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, "; ");

					fprintf(out, "%s: %s",
						parameter->name.c_str(), convertType(parameter->type).c_str());
				}

				fprintf(out, ")");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, ": %s", convertType(method->returnType).c_str());

				fprintf(out, ";\n");
				fprintf(out, "begin\n");
				fprintf(out, "\t");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, "Result := ");

				fprintf(out, "%sVTable(vTable).%s(Self",
					interface->name.c_str(), method->name.c_str());

				for (auto& parameter : method->parameters)
					fprintf(out, ", %s", parameter->name.c_str());

				fprintf(out, ");\n");
				fprintf(out, "end;\n\n");
			}
		}

		for (auto& interface : parser->interfaces)
		{
			deque<Method*> methods;

			for (Interface* p = interface; p; p = p->super)
				methods.insert(methods.begin(), p->methods.begin(), p->methods.end());

			for (auto& method : methods)
			{
				fprintf(out, "%s %sImpl_%sDispatcher(this: Pointer",
					(method->returnType.type == Token::TYPE_VOID ? "procedure" : "function"),
					interface->name.c_str(),
					method->name.c_str());

				for (auto& parameter : method->parameters)
				{
					fprintf(out, "; %s: %s",
						parameter->name.c_str(), convertType(parameter->type).c_str());
				}

				fprintf(out, ")");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, ": %s", convertType(method->returnType).c_str());

				fprintf(out, "; cdecl;\n");
				fprintf(out, "begin\n");
				fprintf(out, "\t");

				if (method->returnType.type != Token::TYPE_VOID)
					fprintf(out, "Result := ");

				fprintf(out, "%sImpl(this).%s(", interface->name.c_str(), method->name.c_str());

				bool firstParameter = true;
				for (auto& parameter : method->parameters)
				{
					if (firstParameter)
						firstParameter = false;
					else
						fprintf(out, ", ");

					fprintf(out, "%s", parameter->name.c_str());
				}

				fprintf(out, ");\n");
				fprintf(out, "end;\n\n");
			}

			fprintf(out, "var\n");
			fprintf(out, "\t%sImpl_vTable: %sVTable;\n\n",
				interface->name.c_str(), interface->name.c_str());

			fprintf(out, "constructor %sImpl.create;\n", interface->name.c_str());
			fprintf(out, "begin\n");
			fprintf(out, "\tvTable := %sImpl_vTable;\n", interface->name.c_str());
			fprintf(out, "end;\n\n");
		}

		fprintf(out, "initialization\n");

		for (auto& interface : parser->interfaces)
		{
			deque<Method*> methods;

			for (Interface* p = interface; p; p = p->super)
				methods.insert(methods.begin(), p->methods.begin(), p->methods.end());

			fprintf(out, "\t%sImpl_vTable := %sVTable.create;\n",
				interface->name.c_str(), interface->name.c_str());
			fprintf(out, "\t%sImpl_vTable.version := %d;\n",
				interface->name.c_str(), (int) methods.size());

			for (auto& method : methods)
			{
				fprintf(out, "\t%sImpl_vTable.%s := @%sImpl_%sDispatcher;\n",
					interface->name.c_str(),
					method->name.c_str(),
					interface->name.c_str(),
					method->name.c_str());
			}

			fprintf(out, "\n");
		}

		fprintf(out, "finalization\n");

		for (auto& interface : parser->interfaces)
			fprintf(out, "\t%sImpl_vTable.destroy;\n", interface->name.c_str());

		fprintf(out, "\n");
		fprintf(out, "end.\n");
	}

private:
	string convertType(const Token& token)
	{
		switch (token.type)
		{
			case Token::TYPE_INT:
				return "Integer";

			default:
				return token.text;
		}
	}

private:
	Parser* parser;
	string unitName;
};


void run(int argc, const char* argv[])
{
	string inFilename(argv[1]);
	string outFormat(argv[2]);
	string outFilename(argv[3]);

	if (argc < 4)
		throw runtime_error("Invalid command line parameters.");

	Lexer lexer(inFilename);

	Parser parser(&lexer);
	parser.parse();

	auto_ptr<Generator> generator;

	if (outFormat == "c++")
	{
		if (argc < 6)
			throw runtime_error("Invalid command line parameters for C++ output.");

		string headerGuard(argv[4]);
		string className(argv[5]);

		generator.reset(new CppGenerator(outFilename, &parser, headerGuard, className));
	}
	else if (outFormat == "c-header")
	{
		if (argc < 5)
			throw runtime_error("Invalid command line parameters for C header output.");

		string headerGuard(argv[4]);

		generator.reset(new CHeaderGenerator(outFilename, &parser, headerGuard));
	}
	else if (outFormat == "c-impl")
	{
		if (argc < 5)
			throw runtime_error("Invalid command line parameters for C implementation output.");

		string includeFilename(argv[4]);

		generator.reset(new CImplGenerator(outFilename, &parser, includeFilename));
	}
	else if (outFormat == "pascal")
	{
		if (argc < 5)
			throw runtime_error("Invalid command line parameters for Pascal output.");

		string unitName(argv[4]);

		generator.reset(new PascalGenerator(outFilename, &parser, unitName));
	}
	else
		throw runtime_error("Invalid output format.");

	generator->generate();
}


int main(int argc, const char* argv[])
{
	try
	{
		run(argc, argv);
		return 0;
	}
	catch (std::exception& e)
	{
		cerr << e.what() << endl;
		return 1;
	}
}
