#include <jde/ql/types/FilterQL.h>
#include <regex>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/TableQL.h>

#define let const auto

namespace Jde::QL{
	inline constexpr std::array<sv,11> QLOperatorStrings = { "eq", "ne", "regex", "glob", "in", "nin", "gt", "gte", "lt", "lte", "elemMatch" };//matches GraphQL

	α Filter::Test( const DB::Value::Underlying& value, const vector<FilterValue>& filters, ELogTags logTags )ι->bool{
		bool passesFilters{true};
		for( auto p=filters.begin(); passesFilters && p!=filters.end(); ++p )
			passesFilters = p->Test( value, logTags );
		return passesFilters;
	}
	α Filter::TestAnd( str columnName, uint value )Ι->bool{
		bool valid{ true };
		if( let filters = ColumnFilters.find(columnName); filters!=ColumnFilters.end() ){
			for( let& filterValue : filters->second ){
				if( valid = filterValue.TestAnd(value); valid )
					break;
			}
		}
		return valid;
	}


	α FilterValue::Test( const DB::Value& db, ELogTags logTags )Ι->bool{
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
			case Greater: passesFilters = db.ToJson() > Value; break;
			case GreaterOrEqual: passesFilters = db.ToJson() >= Value; break;
			case Less: passesFilters = db.ToJson() < Value; break;
			case LessOrEqual: passesFilters = db.ToJson() <= Value; break;
			case ElementMatch: BREAK; break;
			}
		}
		catch( const exception& e ){
			DBGT( logTags | ELogTags::Exception, "FilterValue::Test exception={}", e.what() );
		}
		catch( ... ){
			CRITICALT( logTags | ELogTags::Exception, "FilterValue::unknown exception" );
		}
		return passesFilters;
	}
	α FilterValue::TestAnd( uint value )Ι->bool{
		auto filter = Value.try_to_number<uint>();
		return filter && (*filter & value);
	}

}
namespace Jde{
	α QL::ToString( DB::EOperator op )ι->string{
		return FromEnum( QLOperatorStrings, op );
	}

	α QL::ToQLOperator( string op )ι->DB::EOperator{
		return ToEnum<DB::EOperator>( QLOperatorStrings, op ).value_or( DB::EOperator::Equal );
	}

	α QL::ToWhereClause( const TableQL& table, const DB::View& dbTable, bool includeDeleted )->DB::WhereClause{
		let pFilter = Json::FindObject( table.Args, "filter" );
		let& j = pFilter ? *pFilter : table.Args;
		DB::WhereClause where;
		for( let& [name,value] : j ){
			auto pColumn = name!="id"
				? dbTable.GetColumnPtr( DB::Names::FromJson(name) )
				: AsTable(dbTable).Extends ? AsTable(dbTable).Extends->GetPK() : dbTable.GetPK(); //can't have left join users where users.id=?
			if( name=="deleted" )
				includeDeleted = false;
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
				if( value.is_string() || value.is_number() || value.is_null() || value.is_bool() )
					pJson = &value;
				else if( let o = value.try_as_object(); o && o->size() ){
					let& first = *o->begin();
					op = first.key()=="notIn" ? DB::EOperator::NotIn : DB::ToOperator( first.key() );
					pJson = &first.value();
				}
				else
					THROW("Invalid filter value type '{}'.", Json::Kind(value.kind()) );
				if( pJson->is_array() )
						where.Add( pColumn, op, DB::ToValue<uint>(Json::FromArray<uint>(pJson->get_array())) );
					else
						where.Add( pColumn, op, DB::Value{pColumn->Type, *pJson} );
			}
		}
		if( auto deleted = includeDeleted ? nullptr : dbTable.FindColumn("deleted"); deleted )
			where.Add( deleted, DB::Value{} );
		return where;
	}
}