#include "Parser.h"
#include <jde/ql/ql.h>

#define let const auto
namespace Jde{
	α QL::Parse( string query, bool returnRaw, SL sl )ε->RequestQL{
		sv trimmed = Str::Trim( query );//TODO move(query).
		Parser parser{ string{trimmed.starts_with("{") ? trimmed.substr(1) : trimmed}, "{}()," };
		auto name = parser.Next();
		if( name=="query" ){
			returnRaw = true;
			parser.Next();
			name = parser.Next();
		}
		if( name=="subscription" )
			return RequestQL{ parser.LoadSubscriptions() };
		else if( name=="unsubscribe" )
			return RequestQL{ parser.LoadUnsubscriptions() };
		else if( MutationQL::IsMutation(name) ){
			returnRaw = name!="mutation";
			if( parser.Peek()=="{" )
				parser.Next();
			return RequestQL{ {parser.LoadMutations(returnRaw ? name : parser.Next(), returnRaw)} };
		}else
			return RequestQL{ parser.LoadTables(name, returnRaw) };
	}
	α QL::ParseSubscriptions( string query, SL sl )ε->vector<Subscription>{
		auto request = Parse( move(query), true, sl ); THROW_IFSL( !request.IsSubscription(), "Expected subscription query." );
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

	Ω parseWhitespace( sv json, string& y )ε->uint{
		if( json.empty() )
			return 0;
		uint i{};
		for( char ch=json[i]; isspace(ch) && i<json.size(); ch=json[++i] ){
			y += ch;
			if( i+1==json.size() )
				break;
		}
		return i;
	}
	Ω parseValue( sv json, string& y )ε->uint;
	Ω parseArray( sv json, string& y )ε->uint{
		uint i=0;
		char ch = json[i++];
		ASSERT( ch=='[' );
		y += ch;
		i += parseWhitespace( json.substr(i), y );
		THROW_IF( i>=json.size(), "Expected ']' vs '{}' @ '{}'.", json, i );
		for( char ch = json[i]; ch!=']'; ch = json[i] ){
			i += parseValue( json.substr(i), y );
			i += parseWhitespace( json.substr(i), y );
			THROW_IF( i>=json.size(), "Expected ']' vs '{}' @ '{}'.", json, i );
			if( json[i]==',' )
				y += json[i++];
		}
		y += json[i++];
		return i;
	}
	Ω parseString( sv json, string& y )ε->uint{
		uint i=0;
		char ch = json[i++]; THROW_IF( i==json.size() || ch!='"', "Expected ending quote '{}' @ '{}'.", json, i );
		ASSERT( ch=='"' );
		y += ch;
		for( char ch=json[i++]; ch!='"' && i<json.size(); ch = json[i++] )
			y += ch;
		y += ch;
		return i;
	}
	Ω parseObject( sv json, string& y )ε->uint;
	Ω parseValue( sv json, string& y )ε->uint{
		uint i=0;
		i += parseWhitespace( json.substr(i), y );
		char ch=json[i];
		if( ch=='{' )
			i += parseObject( json.substr(i), y );
		else if( ch=='[' )
			i += parseArray( json.substr(i), y );
		else if( ch=='"' )
			i += parseString( json.substr(i), y );
		else if( ch=='n' ){
			THROW_IF( json.size()-i<5, "Unexpected end vs '{}' @ '{}'.", json, i );
			let null = json.substr( i, 4 );
			THROW_IF( null!="null", "Expected 'null' vs '{}' in '{}' @ '{}'.", null, json, i );
			y += null;
			i += 4;
		}else if( ch=='N' ){
			THROW_IF( json.size()-i<4, "Unexpected end vs '{}' @ '{}'.", json, i );
			let nan = json.substr( i, 3 );
			THROW_IF( nan!="NaN", "Expected 'NaN' vs '{}' in '{}' @ '{}'.", nan, json, i );
			y += nan;
			i += 3;
		}else if( isdigit(ch) || ch=='-' || ch=='.' ){
			for( ; i<json.size() && isdigit(ch); ch = json[++i] ){
				y += ch;
				if( i+1==json.size() )
					break;
			}
		}
		else if( ch!=',' )
			THROW( "Unexpected character '{}' @ '{}'.", ch, i );
		return i;
	}

	Ω parseObject( sv json, string& y )ε->uint{
		uint i=0;
		ASSERT( json[i]=='{' );
		y += json[i++];
		function<void()> memberValueParse = [&]()->void {
			i += parseWhitespace( json.substr(i), y );
			THROW_IF( i>=json.size(), "Expected object to end '{}' @ '{}'.", json, i );
			char ch = json[i];
			if( ch=='}' ){
//				y += json[i++];
				return;
			}
			else if( ch=='"' )
				i += parseString( json.substr(i), y )+1;
			else{
				string name{'"'};
				for( ++i; ch!=':' && i<json.size(); ch=json[i++] ){
					name += ch;
					THROW_IF( i==json.size(), "Could not find ':' in '{}' @ {}", json, i );
				}
				y += Str::RTrim( name+'"' );
			}
			y += ":";
			i += parseValue( json.substr(i), y );
			i+=parseWhitespace( json.substr(i), y );
			THROW_IF( i>=json.size(), "Expected '}}' vs '{}' in '{}' @ '{}'.", json[i], json, i );
			if( json[i]==',' ){
				y += json[i++];
				memberValueParse();
			}
		};
		memberValueParse();
		THROW_IF( i>=json.size() || json[i]!='}', "Expected '}}' vs '{}' in {} @ '{}'.", json[i], json, i );
		y+=json[i++];
		return i;
	}
/*
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
					for( ; i<json.size() && ch!=']'; ch = json[++i] ){
						if( ch=='{' )
							y += parseObject( json.substr(i), y );
						y += ch;
					}
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
					THROW_IF( i+1==json.size(), "Could not find ':' in '{}' @ {}", json, i );
				}
				if( !quoted )
					y += '"';
				y += ch;
				inValue = true;
			}
		}
		return y;
	}
*/
	α Parser::ParseArgs()ε->jobject{
		string params{ Next(')') };
		THROW_IF( params.empty(), "params.empty()" );
		THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), Index()-1, Text() );
		params.front()='{'; params.back() = '}';
		string stringified; stringified.reserve( params.size()*2 );
		parseObject( params, stringified );
		return Json::Parse( stringified );
	}

	α Parser::LoadMutations( string&& command, bool returnRaw )ε->vector<MutationQL>{
		vector<MutationQL> y;
		do{
			auto args = ParseArgs();
			let wantsResult = Peek()=="{";
			optional<TableQL> result = wantsResult ? LoadTable("") : optional<TableQL>{};
			//MutationQL ql{ move(command), move(args), move(result), returnRaw };
			y.push_back( { move(command), move(args), move(result), returnRaw } );
			command = Next();
		}while( MutationQL::IsMutation(command) );
		return y;
	}

	α Parser::LoadTable( sv jsonName )ε->TableQL{//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
		let j = Peek()=="(" ? ParseArgs() : jobject{};

		TableQL table{ string{jsonName}, j };
		if( Peek()=="{" ){//has columns
			Next();
			for( auto token = Next(); token!="}" && token.size(); token = Next() ){
				if( Peek()=="{" || Peek()=="(" )
					table.Tables.push_back( LoadTable(token) );
				else{
					THROW_IF( token==",", "Unexpected column ',' '{}' @ '{}'.", _text, Index()-1 );
					table.Columns.emplace_back( ColumnQL{string{token}} );
				}
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
		string stringified; stringified.reserve( text.size()*2 );
		parseObject(text, stringified);
		return Json::FromArray<SubscriptionId>( Json::AsArray(Json::Parse(stringified), "id") );
	}
}