#include <jde/ql/types/Input.h>

#define let const auto

namespace Jde::QL{
	Ω addFilters( const jvalue& value, const sp<jobject>& variables )ε->vector<FilterValue>{
		vector<FilterValue> columnFilters;
		if( value.is_string() || value.is_number() || value.is_null() || value.is_bool() ) //( id: 42 ) or ( schemaName:"opc.default" ) or ( deleted: null )
			columnFilters.emplace_back( DB::EOperator::Equal, value );
		else if( value.is_object() ){ //criteria:{in:[null,"ns=4;i=6012"]}
			for( let& [joperator,opValue] : value.as_object() ){
				let oprtr{ ToQLOperator(joperator) };
				if( opValue.is_string() && opValue.get_string().starts_with("\b$") ){
					if( auto p = variables->if_contains(sv{opValue.get_string()}.substr(2)); p ){
						columnFilters.emplace_back( oprtr, *p );
						continue;
					}
				}
				columnFilters.emplace_back( oprtr, opValue );
			}
		}
		else if( value.is_array() ) //( id: [1,2,3] ) or ( name: ["charlie","bob"] )
			columnFilters.emplace_back( DB::EOperator::In, value );
		else
			THROW( "Invalid filter value type '{}'.", Json::Kind(value.kind()) );
		return columnFilters;
	}

	α Input::Filter()Ι->QL::Filter{
		if( _filter )
			return *_filter;
		_filter = QL::Filter{};
		for( let& [jsonColumnName,value] : Args ){
			jvalue* variable = nullptr;
			if( value.is_string() && value.get_string().starts_with("\b$") ){
				if( auto p = Variables->if_contains(sv{value.get_string()}.substr(2)); p )
					variable = p;
			}
			_filter->ColumnFilters.emplace( jsonColumnName, addFilters(variable ? *variable : value, Variables) );
		}
		return *_filter;
	}

	α Input::ExtrapolateVariables()Ι->jobject{
		auto extrapolateString = [this]( const jstring& s )ι->jvalue {
			let varName = sv{ s }.substr( 2 );
			let varValue = Variables->if_contains( varName );
			return varValue ? *varValue : jvalue{};
		};
		function<jobject( const jobject& )> extrapolate = [&]( const jobject& o )ι->jobject {
			jobject y;
			for( let& [key, value] : o ){
				constexpr sv escape{ "\b$" };
				if( value.is_string() && value.get_string().starts_with(escape) )
					y.emplace( key, extrapolateString(value.get_string()) );
				else if( value.is_object() )
					y.emplace( key, extrapolate(Json::AsObject(value)) );
				else if( value.is_array() ){
					jarray newArray;
					for( auto&& item : value.get_array() ){
						if( item.is_string() && item.get_string().starts_with(escape) )
							newArray.emplace_back( extrapolateString(item.get_string()) );
						else if( item.is_object() )
							newArray.emplace_back( extrapolate(item.get_object()) );
						else
							newArray.emplace_back( item );
					}
					y.emplace( key, newArray );
				}
				else
					y.emplace( key, value );
			}
			return y;
		};
		return extrapolate( Args );
	}
	α Input::GetKey( SL sl )Ε->DB::Key{
		let y = FindKey();
		THROW_IFSL( !y, "Could not find id or target in mutation  query: {}, variables: {}", serialize(Args), serialize(*Variables) );
		return *y;
	}
	α Input::FindKey()Ι->optional<DB::Key>{
		optional<DB::Key> y;
		if( let id = FindId<uint>(); id )
			y = DB::Key{ *id };
		else if( let target = FindPtr<jstring>("target"); target )
			y = DB::Key{ string{*target} };
		return y;
	}
	α Input::OrderBy()Ι->const vector<std::pair<string,bool>>&{
		if( _orderBy )
			return *_orderBy;
		_orderBy = vector<std::pair<string,bool>>{};
		let orderBy = FindPtr<jvalue>("orderBy");
		if( !orderBy )
			return *_orderBy;
		Json::Visit( *orderBy, [&]( const jvalue& v ){
			if( v.is_string() )
				_orderBy->emplace_back( string{ v.get_string() }, true );
			else if( v.is_object() && !v.get_object().empty() ){
				auto p = v.get_object().begin();
				_orderBy->emplace_back( string{p->key()}, !p->value().is_string() || p->value().get_string()!="desc" );
			}
		} );
		return *_orderBy;
	}
}