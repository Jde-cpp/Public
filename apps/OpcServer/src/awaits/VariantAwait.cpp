#include "VariantAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/DataType.h"

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
			VariantMembers values; values.reserve( rows.size() );
			for( auto&& row : rows )
				values.try_emplace( values.end(), row.GetUInt32(0) )->second.try_emplace( row.GetUInt32(1), row.GetString(2) );
			Resume( move(values) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α VariantAwait::Execute()ι->VariantMembersAwait::Task{
		try{
			auto values = co_await VariantMembersAwait{ _pks, _sl };
			SelectVariants( move(values) );
		}
		catch( exception& e ){
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

			flat_map<VariantPK, Variant> variants; variants.reserve( variantRows.size() );
			for( auto&& row : variantRows ){
				let variantPK = row.GetUInt32(0);
				let& dt = DT( row.GetUInt(1) );
				auto dims = row.IsNull(2) ? make_tuple(nullptr, 0) : Variant::ToArrayDims( row.GetString(2) );
				variants.try_emplace( variants.end(), variantPK, Variant{variantPK, Variant::ToUAValues(dt, move(values.at(variantPK))), move(dims), dt} );
			};
			Resume( move(variants) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α VariantInsertAwait::Execute()ι->DB::ScalerAwait<VariantPK>::Task{
		try{
			let variantPK = co_await DS().InsertSeq<VariantPK>(DB::InsertClause{
				GetView("variants").InsertProcName(),
				{DB::Value{_variant.type->typeId.identifier.numeric}, {_variant.ArrayDimString()}}
			});

			for( auto&& [index, j] : _variant.ToJson() ){
				co_await DS().Execute( DB::Sql{
						Ƒ("INSERT INTO {}(variant_id, idx, value) VALUES (?,?,?)", GetSchema().DBName("variant_members")),
						{ {variantPK},
							{index},
							{j}
						}} );
			}

			ResumeScaler( variantPK );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}