#pragma once
#include <jde/ql/QLSubscriptions.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/TableQL.h>


namespace Jde::QL{
	struct Subscription;
	constexpr array<sv,9> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove", "start", "stop" };

	struct Parser{
		Parser( string text, sv delimiters )ι: _text{move(text)}, Delimiters{delimiters}{}
		α Next()ι->string;
		α Next( char end )ι->string;
		α Peek()ι->str{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		α Index()ι->uint{ return i; }
		α Text()ι->string{ return i<_text.size() ? _text.substr(i) : string{}; }
		α AllText()ι->string{ return _text; }

		α LoadMutation( string&& command, bool returnRaw )ε->MutationQL;
		α LoadTables( sv jsonName, bool returnRaw )ε->vector<TableQL>;
		α LoadSubscriptions()ε->vector<Subscription>;
		α LoadUnsubscriptions()ε->vector<SubscriptionId>;
	private:
		α LoadTable( sv jsonName )ε->TableQL;

		α LoadSubscription()ε->Subscription;
		α ParseArgs()ε->jobject;
		uint i{0};
		string _text;
		sv Delimiters;
		string _peekValue;
	};
}