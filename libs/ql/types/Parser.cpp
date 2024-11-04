#include "Parser.h"
#include <jde/ql/ql.h>
#define let const auto
namespace Jde{
	α QL::Parse( string query )ε->RequestQL{
		sv trimmed = Str::Trim(query);//TODO move(query).
		Parser parser{ string{trimmed.starts_with("{") ? trimmed.substr(1) : trimmed}, "{}()," };
		auto name = parser.Next();
		if( name=="query" ){
			parser.Next();
			name = parser.Next();
		}
		return name=="mutation" ? RequestQL{ parser.LoadMutation() } : RequestQL{ parser.LoadTables(name) };
	}
}
namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL | ELogTags::Parsing };
	constexpr array<sv,9> MutationQLStrings = { "create", "update", "delete", "restore", "purge", "add", "remove", "start", "stop" };

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

	α stringifyKeys( sv json )ε->string{
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

	α Parser::ParseJson()ε->jobject{
		string params{ Next(')') }; THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), Index()-1, Text() );
		params.front()='{'; params.back() = '}';
		params = stringifyKeys( params );
		return Json::Parse( params );
	}

	α Parser::LoadMutation()ε->MutationQL{
		if( Peek()=="{" )
			throw Exception{ _tags, SRCE_CUR, "mutation not expecting '{{' as 1st character." };
		let command = Next();
		uint iType=0;
		for( ;iType<MutationQLStrings.size() && !command.starts_with(MutationQLStrings[iType]); ++iType );
		THROW_IF( iType==MutationQLStrings.size(), "Could not find mutation {}", command );

		auto tableJsonName = string{ command.substr(MutationQLStrings[iType].size()) };
		tableJsonName[0] = (char)tolower( tableJsonName[0] );
		let type = (EMutationQL)iType;
		let j = ParseJson();
		let wantsResult  = Peek()=="{";
		optional<TableQL> result = wantsResult ? LoadTable( tableJsonName ) : optional<TableQL>{};
		MutationQL ql{ string{tableJsonName}, type, j, result/*, parentJsonName*/ };
		return ql;
	}

	α Parser::LoadTable( sv jsonName )ε->TableQL{//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
		let j = Peek()=="(" ? ParseJson() : jobject{};

		TableQL table{ string{jsonName}, j };
		if( Next()=="{" ){//has columns
			for( auto token = Next(); token!="}" && token.size(); token = Next() ){
				if( Peek()=="{" || Peek()=="(" )
					table.Tables.push_back( LoadTable(token) );
				else
					table.Columns.emplace_back( ColumnQL{string{token}} );
			}
		}
		return table;
	}

	α Parser::LoadTables( sv jsonName )ε->vector<TableQL>{
		vector<TableQL> results;
		do{
			results.push_back( LoadTable(jsonName) );
			jsonName = {};
			if( Peek()=="," ){
				Next();
				jsonName = Next();
			}
		}while( jsonName.size() );
		return results;
	}

}