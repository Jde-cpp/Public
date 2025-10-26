#include "ObjectHook.h"
#include <jde/db/meta/AppSchema.h>
#include <jde/db/awaits/ScalerAwait.h>
#include <jde/opc/uatypes/Variant.h>
#include "../UAServer.h"
#include "../awaits/BrowseNameAwait.h"
#include "../awaits/VariableAwait.h"
#include "../uaTypes/Object.h"
#include "../uaTypes/VariableAttr.h"

#define let const auto
namespace Jde::Opc::Server{
	struct ObjectQLAwait final : TAwait<jvalue>{
		ObjectQLAwait( const QL::MutationQL& m, UserPK executer, SL sl )ι:
			TAwait<jvalue>{ sl },
			Mutation{ m },
			Executer{ executer }
		{}
		α Suspend()ι->void override{ GetBrowseName(); }
		α await_resume()ε->jvalue;

		QL::MutationQL Mutation;
		Jde::UserPK Executer;
	private:
		α Table()ε->sp<DB::View>{ return GetViewPtr( "nodes" ); }
		α GetBrowseName()ι->BrowseNameAwait::Task;
		α Create( BrowseName&& browse, NodePK parent )ι->DB::ScalerAwait<NodePK>::Task;
		α CreateVariables( const jarray& variables, NodePK parent )ι->VariableInsertAwait::Task;
	};
	α ObjectQLAwait::GetBrowseName()ι->BrowseNameAwait::Task{
		BrowseName browseName{ Mutation.As<jobject>("browseName", _sl) };
		try{
			co_await BrowseNameAwait{ &browseName };
			let& parent = GetUAServer().GetObject( NodeId{Mutation.As<jobject>("parent", _sl)} );
			if( GetUAServer().Find(parent.PK, browseName.PK) ){
				ResumeExp( Exception{_sl, "Object exists parent: {}, browseName: '{}'", Ƒ("{:x}", parent.PK), browseName.ToString()} );
				co_return;
			}
			Create( move(browseName), parent.PK );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectQLAwait::Create( BrowseName&& browse, NodePK parentPK )ι->DB::ScalerAwait<NodePK>::Task{
		auto& ua = GetUAServer();
		auto& m = Mutation;
		let& schema = GetSchema();
		try{
			Object object{ m.Args, parentPK, move(browse) };
			if( object.TypeDef )
				object.TypeDef = ua.GetTypeDef( *object.TypeDef );
			object = ua.AddObject( move(object), _sl );
			object.PK = co_await DS().InsertSeq<NodePK>( DB::InsertClause{
				schema.DBName( "object_insert" ),
				object.InsertParams()
			});
			ua._objects.try_emplace( object.PK, object );
			auto variables = m.FindPtr( "variables" );
			if( variables && variables->is_array() )
				CreateVariables( variables->get_array(), object.PK );
			else
				Resume( jobject{{"complete", true}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectQLAwait::CreateVariables( const jarray& variables, NodePK parentPK )ι->VariableInsertAwait::Task{
		try{
			for( auto&& vAttrValue : variables ){
				auto& o = vAttrValue.as_object();
				auto browse = BrowseNameAwait::GetOrInsert( Json::AsObject(o,"browseName") );
				co_await VariableInsertAwait{ Variable{o, parentPK, move(browse)}, _sl };
			}
			Resume( jobject{{"complete", true}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectQLAwait::await_resume()ε->jvalue{
		jvalue y;
		if( auto e = Mutation.Find<bool>("$silent").value_or(false) && Promise() ? Promise()->MoveExp() : nullptr; e ){
			e->SetLevel( ELogLevel::Trace );
			y = jobject{ {"complete", true} };
		}else
			y = TAwait<jvalue>::await_resume();
		return y;
	}

	α ObjectHook::InsertBefore( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult{
		return m.TableName()=="objects" ? mu<ObjectQLAwait>( m, executer, sl ) : nullptr;
	}
}