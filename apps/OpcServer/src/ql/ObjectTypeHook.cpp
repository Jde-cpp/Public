#include "ObjectTypeHook.h"
#include <jde/db/meta/AppSchema.h>
#include "../UAServer.h"
#include "../awaits/BrowseNameAwait.h"
#include "../awaits/ReferenceAwait.h"
#include "../awaits/VariableAwait.h"
#include "../uaTypes/Object.h"
#include "../uaTypes/VariableAttr.h"
#include "../uaTypes/Variant.h"
 #include <jde/db/awaits/ScalerAwait.h>

#define let const auto
namespace Jde::Opc::Server{
	struct ObjectTypeQLAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		ObjectTypeQLAwait( QL::MutationQL m, UserPK executer, SL sl )ι:base{ sl }, _input{ move(m) }, _executer{ executer },
			_startup{ Json::FindBool(Args(),"$silent").value_or(false) }
		{}
		ObjectTypeQLAwait( jobject o, sp<ObjectType> oType, UserPK executer, SL sl )ι:base{ sl }, _root{oType}, _input{ move(o) }, _executer{ executer } {}

		α Suspend()ι->void override;
		α await_resume()ε->jvalue;

	private:
		α Args()ι->jobject&{ return _input.index()==0 ? get<QL::MutationQL>(_input).Args : get<jobject>(_input); }
		α GetBrowseName( jobject o, sp<ObjectType> parent )ι->BrowseNameAwait::Task;
		α Create( jobject o, BrowseName&& browse, sp<ObjectType> parent )ι->DB::ScalerAwait<NodePK>::Task;
		α CreateVariables( jobject o, sp<ObjectType> objectType )ι->VariableInsertAwait::Task;
		α CreateRefs( jobject o, sp<ObjectType> oType, flat_map<VariablePK,jarray>&& refs )ι->VoidAwait::Task;
		α CreateChildren( jobject o, sp<ObjectType> parent )ι->ObjectTypeQLAwait::Task;
		sp<ObjectType> _root;
		variant<QL::MutationQL,jobject> _input;
		Jde::UserPK _executer;
		bool _startup{};
	};

	α ObjectTypeQLAwait::Suspend()ι->void{
		try{
			if( !_root )
				_root = GetUAServer().GetTypeDef( NodeId{Args().at("parent").as_object()} );
			GetBrowseName( move(Args()), _root );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ObjectTypeQLAwait::GetBrowseName( jobject o, sp<ObjectType> parent )ι->BrowseNameAwait::Task{
		try{
			BrowseName browseName{ o.at("browseName").as_object() };
			co_await BrowseNameAwait{ &browseName };
			if( GetUAServer().Find(parent->PK, browseName.PK) ){
				if( _startup )
					Resume( jobject{{"complete", true}} );
				else
					ResumeExp( Exception{_sl, "Object Type exists parent: {}, browseName: '{}'", Ƒ("{:x}", parent->PK), browseName.ToString()} );
				co_return;
			}
			Create( move(o), move(browseName), parent );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectTypeQLAwait::Create( jobject o, BrowseName&& browse, sp<ObjectType> parent )ι->DB::ScalerAwait<NodePK>::Task{
		auto& ua = GetUAServer();
		auto& schema = GetSchema();
		try{
			auto oType = ms<ObjectType>( o, parent->PK, move(browse) );
			optional<NodePK> pk;
			if( _startup ){
				let table = schema.GetViewPtr( "object_types" );
				pk = DS().ScalerSyncOpt<uint>( DB::Statement{
					DB::SelectClause{ table->GetColumnPtr("node_id") },
					DB::FromClause{ table },
					DB::WhereClause{ {
						{ table->GetColumnPtr("browse_id"), oType->Browse.PK },
						{ table->GetColumnPtr("parent_node_id"), parent->PK }
					} }
				}.Move() );
				oType->PK = pk.value_or(0);
			}
			ua.AddObjectType( oType );
			if( !pk ){
				oType->PK = co_await DS().InsertSeq<NodePK>( DB::InsertClause{
					schema.DBName( "object_type_insert" ),
					oType->InsertParams()
				});
				ua._typeDefs.try_emplace( oType->PK, oType );
			}
			if( !pk )
				CreateVariables( move(o), oType );
			else
				Resume( jobject{{"complete", true}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectTypeQLAwait::CreateVariables( jobject o, sp<ObjectType> oType )ι->VariableInsertAwait::Task{
		try{
			//flat_map<VariablePK,jarray> variableRefs;
			if( auto variables = o.try_at("variables"); variables && variables->is_array() ){
				for( auto&& value : variables->get_array() ){
					auto& jvariable = value.as_object();
					BrowseName browse{ BrowseNameAwait::GetOrInsert(jvariable.at("browseName").as_object()) };
					Trace{ ELogTags::Test, "v={}", serialize(jvariable) };
					auto variable = co_await VariableInsertAwait{ Variable{move(jvariable), oType->PK, move(browse)}, _sl };
					// if( auto refs = Json::FindArray(jvariable,"refs"); refs )
					// 	variableRefs.emplace( variable.PK, move(*refs) );
				}
			}
			// if( variableRefs.size() )
			// 	CreateRefs( move(o), move(oType), move(variableRefs) );
			// else
			CreateChildren( move(o), move(oType) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ObjectTypeQLAwait::CreateRefs( jobject o, sp<ObjectType> oType, flat_map<VariablePK,jarray>&& refs )ι->VoidAwait::Task{
		try{
			for( auto& [pk, varRefs] : refs ){
				for( auto& jRef : varRefs )
					co_await ReferenceInsertAwait{ Reference{move(jRef.as_object()), pk}, _sl };
			}
			CreateChildren( move(o), move(oType) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectTypeQLAwait::CreateChildren( jobject o, sp<ObjectType> oType )ι->ObjectTypeQLAwait::Task{
		if( auto children = o.try_at("children"); children && children->is_array() ){
			try{
				for( auto&& child : children->get_array() )
					co_await ObjectTypeQLAwait{ move(child.as_object()), oType, _executer, _sl };
			}
			catch( exception& e ){
				ResumeExp( move(e) );
			}
		}
		Resume( jobject{{"complete", true}} );
	}

	α ObjectTypeQLAwait::await_resume()ε->jvalue{
		jvalue y;
		if( auto e = Json::FindBool(Args(),"$silent").value_or(false) && Promise() ? Promise()->MoveExp() : nullptr; e && !dynamic_cast<UAException*>(e.get()) ){
			e->SetLevel( ELogLevel::Trace );
			y = jobject{ {"complete", true} };
		}
		else
			y = TAwait<jvalue>::await_resume();
		return y;
	}

	α ObjectTypeHook::InsertBefore( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult{
		return m.TableName()=="object_types" ? mu<ObjectTypeQLAwait>( m, executer, sl ) : nullptr;
	}
}