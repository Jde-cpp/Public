#include "Parser.h"
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>

#define let const auto
namespace Jde{
	flat_set<string> _systemTables{};
	α QL::SetSystemTables( flat_set<string>&& x )ι->void{
		for( auto&& name : x )
			_systemTables.emplace( move(name) );
	}
	Ω isSystem( str name )ι->bool{
		return name.starts_with("__") || name.starts_with("setting") || name=="status" || name=="logs" || _systemTables.contains(name);
	}

	α QL::IsSystemQuery( const QL::RequestQL& q )ι->bool{
		bool y{};
		if( q.IsQueries() ){
			for( auto table = q.Queries().begin(); !y && table!=q.Queries().end(); ++table )
				y = isSystem( table->JTableName() );
		}
		else if( q.IsMutation() ){
			for( auto mutation = q.Mutations().begin(); !y && mutation!=q.Mutations().end(); ++mutation )
				y = isSystem( mutation->JTableName() );
		}
		return y;
	}

	α QL::Parse( string query, jobject variables, const vector<sp<DB::AppSchema>>& schemas, bool returnRaw, SL /*sl*/ )ε->RequestQL{
		Parser parser{ Str::TrimFirstLast(move(query), '{', '}'), "{}()," };
		if( parser.Trim("query") )
			returnRaw = true;
		auto name = parser.Next();
		auto vars = ms<jobject>( move(variables) );
		if( name=="subscription" )
			return RequestQL{ parser.LoadSubscriptions(vars, schemas) };
		else if( name=="unsubscribe" )
			return RequestQL{ parser.LoadUnsubscriptions() };
		else if( MutationQL::IsMutation(name) ){
			//returnRaw = name!="mutation"; should be what parameter is
			if( parser.Peek()=="{" )
				parser.Next();
			return RequestQL{ {parser.LoadMutations(name=="mutation" ? parser.Next() : move(name), vars, returnRaw, schemas)} };
		}else
			return RequestQL{ parser.LoadTables(move(name), vars, schemas, returnRaw) };
	}
	α QL::ParseSubscriptions( string query, jobject vars, const vector<sp<DB::AppSchema>>& schemas, SL sl )ε->vector<Subscription>{
		auto request = Parse( move(query), move(vars), schemas, true, sl ); THROW_IFSL( !request.IsSubscription(), "Expected subscription query." );
		return request.Subscriptions();
	}

	α QL::ParseQuery( string query, jobject variables, const vector<sp<DB::AppSchema>>& schemas, bool returnRaw, SL sl )ε->TableQL{
		auto ql = Parse( move(query), move(variables), schemas, returnRaw, sl );
		THROW_IFSL( !ql.IsQueries() || ql.Queries().size()!=1, "Expected single query." );
		return move(ql.Queries().front());
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
			for( auto ch = _text[i]; i<_text.size()-1 && ch!=end; ch = _text[++i] ){
				if( ch=='"' ){//string
					++i;
					for( ch = _text[i]; i<_text.size() && ch!='"'; ch = _text[++i] ){
						if( ch=='\\' && i<_text.size()-1 && _text[i+1]=='"' )
							++i;
					}
					THROW_IF( i>=_text.size(), "Expected ending quote '{}' @ '{}'.", _text, i );
				}
			}
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
	Ω parseVariable( sv json, string& y )ε->uint{
		uint i=0;
		char ch = json[i++];
		ASSERT( ch=='$' );
		y += "\"\\b";
		y += ch;
		for( ch=json[i]; (isalnum(ch) || ch=='_') && i<json.size(); ch = json[++i] )
			y += ch;
		y += "\"";
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
		else if ( ch=='$' )
			i += parseVariable( json.substr(i), y );
		else if( ch=='f' ){
			THROW_IF( json.size()-i<6, "Unexpected end vs '{}' @ '{}'.", json, i );
			let false_ = json.substr( i, 5 );
			THROW_IF( false_!="false", "Expected 'false' vs '{}' in '{}' @ '{}'.", false_, json, i );
			y += false_;
			i += 5;
		}
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
		}else if( ch=='t' ){
			THROW_IF( json.size()-i<5, "Unexpected end vs '{}' @ '{}'.", json, i );
			let true_ = json.substr( i, 4 );
			THROW_IF( true_!="true", "Expected 'true' vs '{}' in '{}' @ '{}'.", true_, json, i );
			y += true_;
			i += 4;
		}
		else if( isdigit(ch) || ch=='-' || ch=='.' ){
			for( ; i<json.size() && (isdigit(ch) || ch=='-' || ch=='.'); ch = json[++i] ){
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
			if( ch=='}' )
				return;
			else if( ch=='"' )
				i += parseString( json.substr(i), y )+1;
			else{
				string name{'"'};
				for( ++i; ch!=':' && i<json.size(); ch=json[i++] ){
					name += ch;
					THROW_IF( i==json.size(), "Could not find ':' in '{}' @ {}", json, i );
				}
				y += Str::RTrim( move(name) )+'"';
			}
			y += ":";
			i += parseValue( json.substr(i), y );
			i+=parseWhitespace( json.substr(i), y );
			THROW_IF( i>=json.size(), "Expected '}}' in '{}' @ '{}'.", json, i );
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

	α Parser::ParseArgs()ε->jobject{
		string params{ Next(')') };
		THROW_IF( params.empty(), "params.empty()" );
		THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), Index()-1, Text() );
		params.front()='{'; params.back() = '}';
		string stringified; stringified.reserve( params.size()*2 );
		parseObject( params, stringified );
		return Json::Parse( Str::Replace(stringified, "\n", "\\n") );
	}

	α Parser::LoadMutations( string&& command, sp<jobject> vars, bool returnRaw, const vector<sp<DB::AppSchema>>& schemas )ε->vector<MutationQL>{
		vector<MutationQL> y;
		do{
			auto args = ParseArgs();
			let wantsResult = Peek()=="{";
			auto name = get<0>( MutationQL::ParseCommand(command) );
			let system = isSystem(name);
			auto returnCols = wantsResult ? LoadTable( move(name), vars, schemas, system ) : optional<TableQL>{};
			y.push_back( {move(command), move(args), move(vars), move(returnCols), returnRaw, schemas, system} );
			command = Next();
		}while( MutationQL::IsMutation(command) );
		return y;
	}

	α Parser::LoadTable( string jsonName, sp<jobject> vars, const vector<sp<DB::AppSchema>>& schemas, bool system, SL sl )ε->TableQL{//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
		let j = Peek()=="(" ? ParseArgs() : jobject{};

		TableQL table{ move(jsonName), j, vars, schemas, system, sl };
		if( Peek()=="{" ){//has columns
			Next();
			for( auto token = Next(); token!="}" && token.size(); token = Next() ){
				if( Peek()=="{" || Peek()=="(" ){
					table.Tables.push_back( LoadTable(token, vars, schemas, system || isSystem(token), sl) );
				}else{
					THROW_IF( token==",", "don't separate columns with: ',' '{}' @ '{}'.", _text, Index()-1 );
					if( token=="..." ){
						THROW_IF( "on"!=Next(), "Expected 'on' after '...' in '{}' @ '{}'.", _text, Index()-1 );
						table.InlineFragments.push_back( LoadTable(Next(), vars, schemas, system, sl) );
						continue;
					}
					table.Columns.emplace_back( ColumnQL{string{token}} );
				}
			}
		}
		return table;
	}
	α Parser::LoadTables( string jsonName, sp<jobject> vars, const vector<sp<DB::AppSchema>>& schemas, bool returnRaw, SL sl )ε->vector<TableQL>{
		vector<TableQL> results;
		do{
			auto alias = jsonName.ends_with(':') ? jsonName.substr( 0, jsonName.size()-1 ) : string{};
			if( alias.size() )
				jsonName = Next();
			let system = isSystem(jsonName) ? jsonName : string{};
			auto table = LoadTable( move(jsonName), vars, schemas, system.size(), sl );
			table.Alias = move(alias);
			if( system.size() ){
				if( system=="__type" ){
					if( auto typeName = table.FindPtr<jstring>( "name" ); typeName )
						table.SetDBTable( DB::AppSchema::GetViewPtr( schemas, DB::Names::ToPlural(DB::Names::FromJson(*typeName)), sl ) );
				}
				else if( system=="__schema" ){
					THROW_IF( schemas.empty() || schemas[0]->Tables.empty(), "No schemas found." );
					table.SetDBTable( schemas[0]->Tables.begin()->second );
				}
			}
			table.ReturnRaw = returnRaw;
			results.push_back( move(table) );
			if( Peek().size() )
				jsonName = Next();
		}while( jsonName.size() );
		return results;
	}
	α Parser::LoadSubscription( sp<jobject> vars, const vector<sp<DB::AppSchema>>& schemas )ε->Subscription{
		let name = Next();
		//Sync with MutationQL::EMutationQL
		constexpr array<sv,9> SubscriptionSuffexes{ "Created", "Updated", "Deleted", "Restored", "Purged", "Added", "Removed", "Started", "Stopped" };
		optional<EMutationQL> type; string tableName;
		for( uint iSuffix=0; !type && iSuffix<SubscriptionSuffexes.size(); ++iSuffix ){
			if( name.ends_with(SubscriptionSuffexes[iSuffix]) && name.size()>SubscriptionSuffexes[iSuffix].size() ){
				tableName = DB::Names::ToPlural(DB::Names::FromJson( name.substr(0, name.size()-SubscriptionSuffexes[iSuffix].size())) );
				type = (EMutationQL)iSuffix;
			}
		}
		THROW_IF( !type, "Could not find subscription type for '{}'", name );
		Next();	//{
		Next(); //[userCreated]
		auto table = LoadTable( tableName, vars, schemas, tableName=="logs" );
		return Subscription{ move(tableName), *type, move(table) };

	}
	α Parser::LoadSubscriptions( sp<jobject> vars, const vector<sp<DB::AppSchema>>& schemas )ε->vector<Subscription>{
		vector<Subscription> y;
		do{
			y.push_back( LoadSubscription(vars, schemas) );
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
	α Parser::Trim( sv token )ι->bool{
		let trimmed = Peek()==token;
		if( trimmed ){
			_peekValue = {};
			i = 0;
			_text = _text.substr(token.size() );
			_text = Str::TrimFirstLast( move(_text), '{', '}' );
		}
		return trimmed;
	}
}