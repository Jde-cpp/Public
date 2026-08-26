#include "VariantAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/DataType.h"

#define let const auto
namespace Jde::Opc::Server {
	α VariantMembersAwait::Execute()ι->DB::SelectAwait::Task{
		let table = GetViewPtr( "variant_members" );
		try{
			auto rows = co_await DS().SelectAsync( DB::Statement{
				{table->Columns},
				{table},
				{table->GetSK0(), move(_pks)},
				"variant_id, idx, value"
			}.Move() );

			VariantMembers values;
			for( auto&& row : rows )
				values.try_emplace( values.end(), row.GetUInt32(0) )->second.try_emplace( row.GetUInt32(1), row.GetString(2) );
			Resume( move(values) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α VariantAwait::Execute()ι->VariantMembersAwait::Task{
		try{
			auto values = co_await VariantMembersAwait{ _pks, _sl };
			SelectVariants( move(values) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α VariantAwait::SelectVariants( VariantMembers values )->DB::SelectAwait::Task{
		let table = GetViewPtr( "variants" );
		try{
			let variantRows = co_await DS().SelectAsync(DB::Statement{
				{ table->Columns },
				{ table },
				{ table->GetPK(), move(_pks) },
				"variant_id"
			}.Move() );

			auto variants = ReserveMap<VariantPK, Variant>( variantRows.size() );
			for( auto&& row : variantRows ){
				let variantPK = row.GetUInt32(0);
				let& dt = DT( row.GetUInt(1) );
				//Stored arrayDimensions: the shape a one-element array cannot state by its count.  Empty *or* null is a
				//scalar - `!IsNull` alone was true for every row the old ArrayDimString binding wrote, and it is still
				//true for every such row already in the db.  VariableAwait derives it from the same emptiness test.
				let isArray = !row.GetString( 2 ).empty();
				auto dims = isArray ? Variant::ToArrayDims( row.GetString(2) ) : make_tuple( (UA_UInt32*)nullptr, uint{0} );
				variants.try_emplace( variants.end(), variantPK, Variant{variantPK, Variant::ToUAValues(dt, Members(values, variantPK), isArray), move(dims), dt} );
			};
			Resume( move(variants) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}