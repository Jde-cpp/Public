#include "VariableAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/DataType.h"
#include "../uaTypes/VariableAttr.h"

namespace Jde::Opc::Server {
	α VariableAwait::Execute()ι->DB::SelectAwait::Task {
		try {
			let table = GetViewPtr("variable_nodes");
			auto rows = co_await DS().SelectAsync(DB::Statement{
				vector<sp<DB::Column>>{ table->Columns.begin()+1, table->Columns.end() },
				{ table },
				ServerConfigAwait::ServerWhereClause( *table, {} )
			}.Move() );

			vector<DB::Value> variants;
			for( auto&& row : rows ){
				if( !row.IsNull(21) )
			  	variants.emplace_back( row.GetUInt32(21) );
			}
			LoadVariants( move(variants), move(rows) );
		}
		catch (exception& e) {
			ResumeExp(move(e));
		}
	}
	α VariableAwait::LoadVariants( vector<DB::Value>&& pks, vector<DB::Row> rows )ι->VariantMembersAwait::Task{
		try{
			auto values = pks.size() ? co_await VariantMembersAwait{ move(pks) } : VariantMembers{};
			flat_map<VariablePK, Variable> nodes; nodes.reserve(rows.size());
			for( auto&& row : rows ){
				let variantPK = row.GetUInt32Opt(21);
				let dtPK = row.GetUInt32Opt(29);
				Variable node{
					row,
					GetUAServer().GetTypeDef( row.GetUInt32(12), _sl ),
					variantPK && dtPK ? Variant{ *variantPK, Variant::ToUAValues(DT(*dtPK), move(values.at(*variantPK))), Variant::ToArrayDims(row.GetString(30)), DT(*dtPK) } : UA_Variant{},
					DT( row.GetUInt32(22) ),
					Variant::ToArrayDims( row.GetString(24) )
				};
				node.Browse = GetUAServer().GetBrowse( node.Browse.PK, _sl );
				auto p = nodes.try_emplace( nodes.end(), node.PK, move(node) );
			}
			Resume( move(nodes) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	VariableInsertAwait::VariableInsertAwait( Variable&& node, SL sl )ι:
		base{ sl },
		_node{ move(node) }
	{}

	α VariableInsertAwait::Execute()ι->DB::ScalerAwait<uint>::Task{
		try{
			auto& ua = GetUAServer();
			BrowseNameAwait::GetOrInsert( _node.Browse );
			UA_NodeId vAttrId;
			_node = ua.AddVariable( move(_node) );
			DB::Value variantPK;
			if( _node.value.type ){
				DB::InsertClause insert{
					GetView("variants").InsertProcName(),
					 {{_node.value.type->typeId.identifier.numeric},
						{_node.arrayDimensionsSize ? DB::Value{VariableAttr::ArrayDimensionsString( _node.arrayDimensionsSize, _node.arrayDimensions)} : DB::Value{}}}
				};
				variantPK = co_await DS().InsertSeq<uint>( move(insert), _sl );
			}
			_node.PK = co_await DS().InsertSeq<uint>( DB::InsertClause{
				GetSchema().DBName( "variable_insert" ),
				_node.InsertParams( variantPK )
			} );
			ua._variables.try_emplace( _node.PK, _node );
			InsertMembers( variantPK );
		}
		catch (exception& e) {
			ResumeExp( move(e) );
		}
	}
	α VariableInsertAwait::InsertMembers( DB::Value variantPK )ι->DB::ExecuteAwait::Task{
		if( !variantPK.is_null() ){
			Variant v{ _node.value };
			try{
				for( auto&& [index, j] : v.ToJson() ){
					co_await DS().Execute( DB::Sql{
						Ƒ("INSERT INTO {}(variant_id, idx, value) VALUES (?,?,?)", GetSchema().DBName("variant_members")),
						{ variantPK,{index},{j} }
					} );
				}
			}
			catch (exception& e) {
				ResumeExp( move( e ) );
				co_return;
			}
		}
		for( auto&& ref : _node._refs ){
			ref->SourcePK = _node.PK;
			try{
				co_await DS().Execute( DB::Sql{
					Ƒ("INSERT INTO {}(source_node_id, target_node_id, ref_type_id, is_forward) VALUES (?,?,?,?)", GetSchema().DBName("references")),
					{ {ref->SourcePK}, {ref->TargetPK}, {ref->RefTypePK}, {ref->IsForward} }
				});
				GetUAServer().AddReference( *ref );
			}
			catch( exception& e ){
				ResumeExp( move(e) );
				co_return;
			}
		}
		Resume( move(_node) );
	}
}