#include "ConstructorHook.h"
#include "../UAServer.h"
#include "../awaits/VariantAwait.h"
#include "../uaTypes/BrowseName.h"
#include "../uaTypes/Variant.h"

namespace Jde::Opc::Server{
	struct ConstructorQLAwait final : TAwait<jvalue>{
		ConstructorQLAwait( const QL::MutationQL& m, UserPK executer, SL sl )ι: TAwait<jvalue>{ sl }, _mutation{ m }, _executer{ executer } {}
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<VariantPK>::Task;
		QL::MutationQL _mutation;
		Jde::UserPK _executer;
	};

	α ConstructorQLAwait::Execute()ι->TAwait<VariantPK>::Task{
		auto& ua = GetUAServer();
		try{
			auto& m = _mutation;
			auto node = ua.GetTypeDef( NodeId{m.GetRef<jobject>("node", _sl)} );
			jarray y;
			flat_map<BrowseNamePK, Variant> values;
			for( auto&& v : m.GetRef<jarray>("values", _sl) ){
				auto& o = v.as_object();
				BrowseName browse{ o.at("browseName").as_object() };
				ua.GetBrowse( browse );
				Variant variant{ o.at("value"), Json::FindSV(o, "dataType").value_or("") };
				let variantPK = co_await VariantInsertAwait{ variant };
				values.try_emplace( browse.PK, move(variant) );
				y.emplace_back( jobject{
					{"browse", {"id", browse.PK}},
					{"value", {"id", variantPK}}
				} );
				co_await DS().Execute( DB::InsertClause{
					GetSchema().GetView("constructors").InsertProcName(),
					{{node->PK}, {browse.PK}, {variantPK}}
				}.Move(), _sl );
			}
			ua.AddConstructor( *node, move(values) );
			Resume( y );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ConstructorHook::InsertBefore( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult{
		return m.TableName()=="constructors" ? mu<ConstructorQLAwait>( m, executer, sl ) : nullptr;
	}
}