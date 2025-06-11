#include "VariantAwait.h"
#include "ServerConfigAwait.h"

namespace Jde::Opc::Server {

	α VariantAwait::Execute()ι->DB::SelectAwait::Task{
		let vTable = GetViewPtr( "variants" );
		try{
			let memberTable = GetViewPtr( "variant_members" );
			auto memberRows = co_await DS().SelectAsync( DB::Statement{
				{memberTable->Columns},
				{memberTable},
				{memberTable->GetSK0(), _pks},
				"variant_id, member_id"
			}.Move() );
			flat_map<VariantPK, flat_map<uint,string>> values; memberRows.reserve( memberRows.size() );
			auto mhint = values.begin();
			for( auto&& row : memberRows ){
				mhint = values.try_emplace( mhint, row.GetUInt32(0) );
				mhint->second.try_emplace( row.GetUInt32(1), row.GetString(2) );
			}

			let variantRows = co_await DS().SelectAsync(DB::Statement{
				{ vTable->Columns },
				{ vTable },
				{ vTable->GetPK(), move(_pks) },
				"variant_id"
			}.Move() );

			flat_map<VariantPK, Variant> variants; variants.reserve( variantRows.size() );
			mhint = values.begin();
			auto vhint = variants.begin();
			for( auto&& row : variantRows ){
				let variantPK = row.GetUInt32(0);
				let& dt = DataType(row.GetUInt(1));
				auto dims = row.IsNull(2) ? make_tuple(nullptr, 0) : Variant::ToArrayDims( row.GetString(2) );
				vhint = variants.try_emplace( vhint, variantPK, Variant{variantPK, Variant::ToUAValues(dt, move(values.at(variantPK))), move(dims), dt} );
			};
			Resume( move(variants) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}