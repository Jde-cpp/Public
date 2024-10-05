#include "Parser.h"
#define var const auto
namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL | ELogTags::Parsing };

	α Parse( sv query )ε->RequestQL{
		uint i = query.find_first_of( "{" ); THROW_IF( i==sv::npos || i>query.size()-2, "Invalid query '{}'", query );
		Parser parser{ query.substr(i+1), "{}()," };
		auto name = parser.Next();
		if( name=="query" )
			name = parser.Next();

		return name=="mutation" ? RequestQL{ parser.LoadMutation() } : RequestQL{ parser.LoadTables(name) };
	}

	α Parser::Next()ι->sv{
		sv result = _peekValue;
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
			_peekValue = {};
		return result;
	};

	α Parser::Next( char end )ι->sv{
		sv result;
		if( _peekValue.size() ){
			i = i-_peekValue.size();
			_peekValue = {};
		}
		for( auto ch = _text[i]; i<_text.size() && isspace(ch); ch = _text[++i] );
		if( i<_text.size() ){
			uint start = i;
			for( auto ch = _text[i]; i<_text.size() && ch!=end; ch = _text[++i] );
			++i;
			result = _text.substr( start, i-start );
		}
		return result;
	};

	α StringifyKeys( sv json )ι->string{
		string y{}; y.reserve( json.size()*2 );
		bool inValue = false;
		for( uint i=0; i<json.size(); ++i ){
			char ch = json[i];
			if( std::isspace(ch) || ch==',' )
				y += ch;
			else if( ch=='{' || ch=='}' ){
				y += ch;
				inValue = false;
			}
			else if( inValue ){
				if( ch=='{' ){
					for( uint i2=i+1, openCount=1; i2<json.size(); ++i2 ){
						if( json[i2]=='{' )
							++openCount;
						else if( json[i2]=='}' && --openCount==0 ){
							y += StringifyKeys( json.substr(i,i2-i) );
							i = i2+1;
							break;
						}
					}
				}
				else if( ch=='[' ){
					for( ; i<json.size() && ch!=']'; ch = json[++i] )
						y += ch;
					y += ']';
				}
				else{
					for( ; i<json.size() && ch!=',' && ch!='}'; ch = json[++i] )
						y += ch;
					if( i<json.size() )
						y += ch;
				}
				inValue = false;
			}
			else{
				var quoted = ch=='"';
				if( !quoted ) --i;
				for( ch = '"'; i<json.size() && ch!=':'; ch=json[++i] )
					y += ch;
				if( !quoted )
					y += '"';
				y += ":";
				inValue = true;
			}
		}
		return y;
	}

	α Parser::ParseJson()ε->json{
		string params{ Next(')') }; THROW_IF( params.front()!='(', "Expected '(' vs {} @ '{}' to start function - '{}'.",  params.front(), Index()-1, Text() );
		params.front()='{'; params.back() = '}';
		params = StringifyKeys( params );
		return Json::Parse( params );
	}

	α Parser::LoadMutation()ε->MutationQL{
		if( Peek()=="{" )
			throw Exception{ _tags, SRCE_CUR, "mutation not expecting '{{' as 1st character." };
		var command = Next();
		uint iType=0;
		for( ;iType<MutationQLStrings.size() && !command.starts_with(MutationQLStrings[iType]); ++iType );
		THROW_IF( iType==MutationQLStrings.size(), "Could not find mutation {}", command );

		auto tableJsonName = string{ command.substr(MutationQLStrings[iType].size()) };
		tableJsonName[0] = (char)tolower( tableJsonName[0] );
		var type = (EMutationQL)iType;
		var j = ParseJson();
		var wantsResult  = Peek()=="{";
		optional<TableQL> result = wantsResult ? LoadTable( tableJsonName ) : optional<TableQL>{};
		MutationQL ql{ string{tableJsonName}, type, j, result/*, parentJsonName*/ };
		return ql;
	}

	α Parser::LoadTable( sv jsonName )ε->TableQL{//__type(name: "Account") { fields { name type { name kind ofType{name kind} } } }
		var j = Peek()=="(" ? ParseJson() : json{};
		if( Next()!="{" )
			throw Exception{ _tags, SRCE_CUR, "Expected '{{' after table name. i='{}', text='{}'", Index(), AllText() };

		TableQL table{ string{jsonName}, j };
		for( auto token = Next(); token!="}" && token.size(); token = Next() ){
			if( Peek()=="{" )
				table.Tables.push_back( LoadTable(token) );
			else
				table.Columns.emplace_back( ColumnQL{string{token}} );
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