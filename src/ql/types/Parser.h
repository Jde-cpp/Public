#pragma once
#include <jde/ql/GraphQL.h>

namespace Jde::QL{
	α Parse( sv query )ε->RequestQL;

	struct Parser{
		Parser( sv text, sv delimiters )ι: _text{text}, Delimiters{delimiters}{}
		α Next()ι->sv;
		α Next( char end )ι->sv;
		α Peek()ι->sv{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		α Index()ι->uint{ return i; }
		α Text()ι->sv{ return i<_text.size() ? _text.substr(i) : sv{}; }
		α AllText()ι->sv{ return _text; }

		α LoadMutation()ε->MutationQL;
		α LoadTables( sv jsonName )ε->vector<TableQL>;
	private:
		α LoadTable( sv jsonName )ε->TableQL;
		α ParseJson()ε->json;
		uint i{0};
		sv _text;
		sv Delimiters;
		sv _peekValue;
	};
}