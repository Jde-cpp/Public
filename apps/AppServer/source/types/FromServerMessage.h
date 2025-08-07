#pragma once
/*
namespace Jde::ApplicationServer{
	α AddSessionResult( SessionPK sessionId, uint requestId )ι->Logging::Proto::FromServer{
		Logging::Proto::FromServer t;
		auto p = t.add_messages()->mutable_add_session_result();
		p->set_session_id( sessionId );
		p->set_request_id( requestId );
		return t;
	}

	α GraphQLResult( const json& j, uint requestId )ι->Logging::Proto::FromServer{
		Logging::Proto::FromServer t;
		auto p = t.add_messages()->mutable_graph_ql();
		p->set_request_id( requestId );
		p->set_result( j.dump() );
		return t;
	}

	α ProtoException( IException&& e, uint requestId )ι->Logging::Proto::FromServer{
		Logging::Proto::FromServer t;
		auto p = t.add_messages()->mutable_exception();
		p->set_request_id( requestId );
		p->set_what( e.what() );
		return t;
	}

	α Acknowledgement( SessionPK id )ι->Logging::Proto::FromServer{
		Logging::Proto::FromServer t;
		auto p = t.add_messages()->mutable_acknowledgement();
		p->set_instanceid( id );
		return t;
	}
}
*/