#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/TableQL.h>

namespace Jde::QL{
	struct Parser{
		Parser( string text, sv delimiters )ι: _text{move(text)}, Delimiters{delimiters}{}
		α Next()ι->string;
		α Next( char end )ι->string;
		α Peek()ι->str{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		α Index()ι->uint{ return i; }
		α Text()ι->string{ return i<_text.size() ? _text.substr(i) : string{}; }
		α AllText()ι->string{ return _text; }

		α LoadMutation()ε->MutationQL;
		α LoadTables( sv jsonName, bool returnRaw )ε->vector<TableQL>;
	private:
		α LoadTable( sv jsonName )ε->TableQL;
		α ParseJson()ε->jobject;
		uint i{0};
		string _text;
		sv Delimiters;
		string _peekValue;
	};
}