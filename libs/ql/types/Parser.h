#pragma once
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/TableQL.h>


namespace Jde::QL{
	struct Subscription;
	constexpr array<sv,9> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove", "start", "stop" };
	Ξ ToString( EMutationQL type )ι->string{ return FromEnum(MutationQLStrings, type); }
	struct Parser{
		Parser( string text, sv delimiters )ι: _text{move(text)}, Delimiters{delimiters}{}
		α Next()ι->string;
		α Next( char end )ι->string;
		α Peek()ι->str{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }
		α Index()ι->uint{ return i; }
		α Text()ι->string{ return i<_text.size() ? _text.substr(i) : string{}; }
		α AllText()ι->string{ return _text; }

		α LoadMutations( string&& command, bool returnRaw, const vector<sp<DB::AppSchema>>& schemas )ε->vector<MutationQL>;
		α LoadTables( string jsonName, const vector<sp<DB::AppSchema>>& schemas, bool returnRaw, SRCE )ε->vector<TableQL>;
		α LoadSubscriptions( const vector<sp<DB::AppSchema>>& schemas )ε->vector<Subscription>;
		α LoadUnsubscriptions()ε->vector<SubscriptionId>;
	private:
		α LoadTable( string jsonName, const vector<sp<DB::AppSchema>>& schemas, bool system=false, SRCE )ε->TableQL;

		α LoadSubscription( const vector<sp<DB::AppSchema>>& schemas )ε->Subscription;
		α ParseArgs()ε->jobject;
		uint i{0};
		string _text;
		sv Delimiters;
		string _peekValue;
	};
}