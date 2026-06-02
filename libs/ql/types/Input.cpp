#include <jde/ql/types/Input.h>

#define let const auto

namespace Jde::QL{
	α Input::Filter()Ι->QL::Filter{
		if( _filter )
			return *_filter;
		auto addFilters = []( jvalue&& value )ε->vector<FilterValue> {
			vector<FilterValue> columnFilters;
			if( value.is_string() || value.is_number() || value.is_null() || value.is_bool() ) //( id: 42 ) or ( schemaName:"opc.default" ) or ( deleted: null )
				columnFilters.emplace_back( DB::EOperator::Equal, move(value) );
			else if( value.is_object() ){ //criteria:{in:[null,"ns=4;i=6012"]}
				for( let& [joperator,opValue] : value.as_object() ){
					let oprtr{ ToQLOperator(joperator) };
					columnFilters.emplace_back( oprtr, move(opValue) );
				}
			}
			else if( value.is_array() ) //( id: [1,2,3] ) or ( name: ["charlie","bob"] )
				columnFilters.emplace_back( DB::EOperator::In, move(value) );
			else
				THROW( "Invalid filter value type '{}'.", Json::Kind(value.kind()) );
			return columnFilters;
		};
		_filter = QL::Filter{};
		auto args = ExtrapolateVariables();
		for( auto& [jsonColumnName,value] : args )
			_filter->ColumnFilters.emplace( jsonColumnName, addFilters(move(value)) );
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
				if( value.is_string() && value.get_string().starts_with(Escape) )
					y.emplace( key, extrapolateString(value.get_string()) );
				else if( value.is_object() )
					y.emplace( key, extrapolate(Json::AsObject(value)) );
				else if( value.is_array() ){
					jarray newArray;
					for( auto&& item : value.get_array() ){
						if( item.is_string() && item.get_string().starts_with(Escape) )
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
	α Input::OrderByJson()Ι->const vector<std::pair<string,bool>>&{	//[column,asc]
		if( _orderBy )
			return *_orderBy;
		_orderBy = vector<std::pair<string,bool>>{};
		let orderBy = FindPtr<jvalue>( "orderBy" );
		if( !orderBy )
			return *_orderBy;
		Json::Visit( *orderBy, [&](const jvalue& v){
			if( v.is_string() )
				_orderBy->emplace_back( string{v.get_string()}, true );
			else if( v.is_object() && !v.get_object().empty() ){
				auto p = v.get_object().begin();
				_orderBy->emplace_back( string{p->key()}, !p->value().is_string() || p->value().get_string()!="desc" );
			}
		} );
		return *_orderBy;
	}
	α Input::ArgString()Ι->string{
		if( Args.empty() )
			return {};
		string argStr{ '('};
		for( let& [key, value] : Args ){
			auto addArg = [&key, &argStr]( const jvalue& v ){
				if( v.is_string() )
					argStr += Ƒ( "{}: \"{}\", ", key, (sv)v.get_string() );
				else
					argStr += Ƒ( "{}: {}, ", key, serialize(v) );
			};
			if( let vname = VariableName(value); vname.size() )
				addArg( Variables->if_contains(vname) ? Variables->at(vname) : value );
			else
				addArg( value );
		}
		argStr.back() = ')';
		return argStr;
	}
}