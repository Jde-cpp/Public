#include "Parser.h"
#include <jde/ql/ql.h>

#define let const auto
namespace Jde{
	α QL::Parse( string query, SL sl )ε->RequestQL{
		sv trimmed = Str::Trim( query );//TODO move(query).
		Parser parser{ string{trimmed.starts_with("{") ? trimmed.substr(1) : trimmed}, "{}()," };
		auto name = parser.Next();
		bool returnRaw = name!="query";
		if( !returnRaw ){
			parser.Next();
			name = parser.Next();
		}
		if( name=="subscription" )
			return RequestQL{ parser.LoadSubscriptions() };
		else if( name=="unsubscribe" )
			return RequestQL{ parser.LoadUnsubscriptions() };
		else if( MutationQL::IsMutation(name) ){
			returnRaw = name!="mutation";
			return RequestQL{ parser.LoadMutation(!returnRaw ? parser.Next() : name, returnRaw) };
		}else
			return RequestQL{ parser.LoadTables(name, returnRaw) };
	}
	α QL::ParseSubscriptions( string query, SL sl )ε->vector<Subscription>{
		auto request = Parse( move(query), sl ); THROW_IFSL( !request.IsSubscription(), "Expected subscription query." );
		return request.Subscriptions();
	}
}
namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL | ELogTags::Parsing };

	α Parser::Next()ι->string{
		string result = move( _peekValue );
		if( result.empty() ){
			if( i<_text.size() )
				for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = i<_text.size()-1 ? _text[++i] : _text[i++] );
			if( i<_text.size() ){
				uint start=i;
				i = start+std::distance( _text.begin()+i, std::find_if(_text.begin()+i, _text.end(), [this]( char ch )ι{ return isspace(ch) || Delimiters.find(ch)!=sv::npos;}) );
				result = i==start ? _text.substr( i++, 1 ) : _text.substr( start, i-start );
			}
		}
		else
			_peekValue.clear();

		return result;
	};

	α Parser::Next( char end )ι->string{
		string result;
		if( _peekValue.size() ){
			i = i-_peekValue.size();
			_peekValue.clear();
		}
		for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = _text[++i] );
		if( i<_text.size() ){
			uint start = i;
			for( auto ch = _text[i]; i<_text.size()-1 && ch!=end; ch = _text[++i] );
			++i;
			result = _text.substr( start, i-start );
		}
		return result;
	};

	Ω stringifyKeys( sv json )ε->string{
		string y{}; y.reserve( json.size()*2 );
		bool inValue{};
		for( uint i=0; i<json.size(); ++i ){
			char ch = json[i];
			if( std::isspace(ch) || ch==',' )
				y += ch;
			else if( ch=='{' || ch=='}' ){
				y += ch;
				inValue = false;
			}
			else if( inValue ){
				if( ch=='{' ){//object
					for( uint i2=i+1, openCount=1; i2<json.size(); ++i2 ){
						if( json[i2]=='{' )
							++openCount;
						else if( json[i2]=='}' && --openCount==0 ){
							y += stringifyKeys( json.substr(i,i2-i) );
							i = i2+1;
							break;
						}
					}
				}
				else if( ch=='[' ){//array
					for( ; i<json.size() && ch!=']'; ch = json[++i] )
						y += ch;
					y += ']';
				}
				else if( ch=='"' ){//string
					y += ch;
					for( ch=json[++i]; i<json.size() && ch!='"'; ch = json[++i] )
						y += ch;
					y += ch;
				}
				else{//number
					for( ; i<json.size() && ch!=',' && ch!='}'; ch = json[++i] )
						y += ch;
					if( i<json.size() )
						y += ch;
				}
				inValue = false;
			}
			else{//string key
				let quoted = ch=='"';
				if( !quoted ) --i;
				for( ch = '"'; i<json.size() && ch!=':'; ch=json[++i] ){
					y += ch;
					THROW_IF( i+1==json.size(), "Could not find ':' in '{}'", json );
				}
				if( !quoted )
					y += '"';
				y += ch;
				inValue = true;
			}
		}
		return y;
	}

	α Parser::ParseArgs()ε->jobject{
		string params{ Next(')') };
		THROW_IF( params.empty(), "params.empty()" );
		THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), Index()-1, Text() );
		params.front()='{'; params.back() = '}';
		params = stringifyKeys( params );
		return Json::Parse( params );
	}

	α Parser::LoadMutation( string&& command, bool returnRaw )ε->MutationQL{
		auto args = ParseArgs();
		let wantsResult = Peek()=="{";
		optional<TableQL> result = wantsResult ? LoadTable("") : optional<TableQL>{};
		MutationQL ql{ move(command), move(args), move(result), returnRaw };
		return ql;
	}

	α Parser::LoadTable( sv jsonName )ε->TableQL{//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
		let j = Peek()=="(" ? ParseArgs() : jobject{};

		TableQL table{ string{jsonName}, j };
		if( Peek()=="{" ){//has columns
			Next();
			for( auto token = Next(); token!="}" && token.size(); token = Next() ){
				if( Peek()=="{" || Peek()=="(" )
					table.Tables.push_back( LoadTable(token) );
				else
					table.Columns.emplace_back( ColumnQL{string{token}} );
			}
		}
		return table;
	}

	α Parser::LoadTables( sv jsonName, bool returnRaw )ε->vector<TableQL>{
		vector<TableQL> results;
		do{
			auto table = LoadTable(jsonName);
			table.ReturnRaw = returnRaw;
			results.push_back( move(table) );
			jsonName = {};
			if( Peek()=="," ){
				Next();
				jsonName = Next();
			}
		}while( jsonName.size() );
		return results;
	}
	α Parser::LoadSubscription()ε->Subscription{
		let name = Next();
		//Sync with MutationQL::EMutationQL
		constexpr array<sv,9> SubscriptionSuffexes{ "Created", "Updated", "Deleted", "Restored", "Purged", "Added", "Removed", "Started", "Stopped" };
		optional<EMutationQL> type; string tableName;
		for( uint i=0; !type && i<SubscriptionSuffexes.size(); ++i ){
			if( name.ends_with(SubscriptionSuffexes[i]) && name.size()>SubscriptionSuffexes[i].size() ){
				tableName = DB::Names::ToPlural(DB::Names::FromJson( name.substr(0, name.size()-SubscriptionSuffexes[i].size())) );
				type = (EMutationQL)i;
			}
		}
		THROW_IF( !type, "Could not find subscription type for '{}'", name );
		Next();	//{
		return Subscription( tableName, *type, LoadTable(Next()) );

	}
	α Parser::LoadSubscriptions()ε->vector<Subscription>{
		vector<Subscription> y;
		do{
			y.push_back( LoadSubscription() );
		}while( Next()=="subscription" );

		return y;
	}
	α Parser::LoadUnsubscriptions()ε->vector<SubscriptionId>{
		let text{ Next('}') };
		THROW_IF( text.empty(), "text.empty()" );
		THROW_IF( text.front()!='{', "Expected '{{' vs {} @ '{}' to start unsubscribe - '{}'.", text.front(), Index()-1, Text() );
		return Json::FromArray<SubscriptionId>( Json::AsArray(Json::Parse(stringifyKeys(text)), "id") );
	}
}