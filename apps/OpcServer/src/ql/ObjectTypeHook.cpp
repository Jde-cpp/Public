#include "ObjectTypeHook.h"
#include <jde/db/meta/AppSchema.h>
#include "../UAServer.h"
#include "../awaits/BrowseNameAwait.h"
#include "../awaits/VariableAwait.h"
#include "../uaTypes/Object.h"
#include "../uaTypes/VariableAttr.h"
#include "../uaTypes/Variant.h"
 #include <jde/db/awaits/ScalerAwait.h>

#define let const auto
namespace Jde::Opc::Server{
	struct ObjectTypeQLAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		ObjectTypeQLAwait( QL::MutationQL m, UserPK executer, SL sl )ι:base{ sl }, _input{ move(m) }, _executer{ executer } {}
		ObjectTypeQLAwait( jobject o, sp<ObjectType> oType, UserPK executer, SL sl )ι:base{ sl }, _root{oType}, _input{ move(o) }, _executer{ executer } {}

		α Suspend()ι->void override;
		α await_resume()ε->jvalue;

	private:
		α Args()ι->jobject&{ return _input.index()==0 ? get<QL::MutationQL>(_input).Args : get<jobject>(_input); }
		α GetBrowseName( jobject o, sp<ObjectType> parent )ι->BrowseNameAwait::Task;
		α Create( jobject o, BrowseName&& browse, sp<ObjectType> parent )ι->DB::ScalerAwait<NodePK>::Task;
		α CreateVariables( jobject o, sp<ObjectType> objectType )ι->VariableInsertAwait::Task;
		α CreateChildren( jobject o, sp<ObjectType> parent )ι->ObjectTypeQLAwait::Task;
		sp<ObjectType> _root;
		variant<QL::MutationQL,jobject> _input;
		Jde::UserPK _executer;

	};

	α ObjectTypeQLAwait::Suspend()ι->void{
		try{
			if( !_root )
				_root = GetUAServer().GetTypeDef( ExNodeId{Args().at("parent").as_object()} );
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
		let& schema = GetSchema();
		try{
			auto oType = ms<ObjectType>( o, parent->PK, move(browse) );
			ua.AddObjectType( oType );
			oType->PK = co_await DS().InsertSeq<NodePK>( DB::InsertClause{
				schema.DBName( "object_type_insert" ),
				oType->InsertParams()
			});
			ua._typeDefs.try_emplace( oType->PK, oType );

			CreateVariables( move(o), oType );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectTypeQLAwait::CreateVariables( jobject o, sp<ObjectType> oType )ι->VariableInsertAwait::Task{
		if( auto variables = o.try_at("variables"); variables && variables->is_array() ){
			try{
				for( auto&& value : variables->get_array() ){
					auto& variable = value.as_object();
					BrowseName browse{ BrowseNameAwait::GetOrInsert(variable.at("browseName").as_object()) };
					Trace{ ELogTags::Test, "v={}", serialize(variable) };
					co_await VariableInsertAwait{ Variable{move(variable), oType->PK, move(browse)}, _sl };
				}
			}
			catch( exception& e ){
				ResumeExp( move(e) );
			}
		}
		CreateChildren( move(o), oType );
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