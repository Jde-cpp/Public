#include <jde/ql/FilterQL.h>
#include <jde/ql/TableQL.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Table.h>
#include <regex>

#define let const auto

namespace Jde::QL{
	inline constexpr std::array<sv,11> QLOperatorStrings = { "eq", "ne", "regex", "glob", "in", "nin", "gt", "gte", "lt", "lte", "elemMatch" };//matches GraphQL

	α FilterQL::Test( const DB::Value& value, const vector<FilterValueQL>& filters, ELogTags logTags )ι->bool{
		bool passesFilters{true};
		for( auto p=filters.begin(); passesFilters && p!=filters.end(); ++p )
			passesFilters = p->Test( value, logTags );
		return passesFilters;
	}

	α FilterValueQL::Test( const DB::Value& db, ELogTags logTags )Ι->bool{
		using namespace Json;
		bool passesFilters{};
		try{
			switch( Operator ){
			using enum DB::EOperator;
			case Equal: passesFilters = Value==db.ToJson(); break;
			case NotEqual: passesFilters = Value!=db.ToJson(); break;
			case Regex: passesFilters = std::regex_match( db.ToString(), std::regex{AsString(Value)} ); break;
			case Glob:
				passesFilters = std::regex_match( db.ToString(), std::regex{AsString(Value)} );//TODO test this
				break;
			case In: passesFilters = Json::Find( Value, db.ToJson() ); break;
			case NotIn: passesFilters = !Json::Find( Value, db.ToJson() ); break;
			case Greater: passesFilters = Value > db.ToJson(); break;
			case GreaterOrEqual: passesFilters = Value >= db.ToJson(); break;
			case Less: passesFilters = Value < db.ToJson(); break;
			case LessOrEqual: passesFilters = Value <= db.ToJson(); break;
			case ElementMatch: BREAK; break;
			}
		}
		catch( const std::exception& e ){
			Debug( logTags | ELogTags::Exception, "FilterValueQL::Test exception={}", e.what() );
		}
		catch( ... ){
			Critical( logTags | ELogTags::Exception, "FilterValueQL::unknown exception" );
		}
		return passesFilters;
	}
}
namespace Jde{
	α QL::ToString( DB::EOperator op )ι->string{
		return FromEnum( QLOperatorStrings, op );
	}

	α QL::ToQLOperator( string op )ι->DB::EOperator{
		return ToEnum<DB::EOperator>( QLOperatorStrings, op ).value_or( DB::EOperator::Equal );
	}

	α QL::ToWhereClause( const TableQL& table, const DB::Table& schemaTable )->DB::WhereClause{
		let pFilter = Json::FindObject( table.Args, "filter" );
		let& j = pFilter ? table.Args : *pFilter;
		DB::WhereClause where;
		for( let& [name,value] : j ){
			auto pColumn = schemaTable.GetColumnPtr( DB::Schema::FromJson(name) );
			using enum DB::EOperator;
			DB::EOperator op = Equal;
			const jvalue* pJson{};
			vector<DB::Value> params;
			if( let array = value.try_as_array(); array ){
				for( let& v : *array )
					params.push_back( DB::Value{pColumn->Type, v} );
				where.Add( pColumn, params );
			}
			else{
				if( value.is_string() || value.is_number() || value.is_null() )
					pJson = &value;
				else if( let o = value.is_object() && value.as_object().size() ? value.as_object() : optional<jobject>{}; o ){
					let& first = *o->begin();
					op = DB::ToOperator( first.key() );
					pJson = &first.value();
				}
				else
					THROW("Invalid filter value type '{}'.", Json::TypeName(value) );
				if( pJson ){
					where << op << "?";
					parameters.push_back( ToObject(pColumn->Type, *pJson, name) );
				}
			}
		}
		return where;
	}
}
