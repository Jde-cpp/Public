//The transmission builders both ends of the app socket speak through.  Each one sets one oneof member on one message,
//so what these hold is the request id it is answered on, the oneof case the receiver switches on, and the payload.
#include <gtest/gtest.h>
#include <jde/fwk/process/process.h>
#include <jde/app/proto/App.FromServer.pb.h> //app.FromServer.h declares against the generated types without including them.
#include <jde/app/proto/app.FromClient.h>
#include <jde/app/proto/app.FromServer.h>
#include <jde/app/proto/common.h>
#include "helpers.h"

#define let const auto

namespace Jde::App::Tests{
	using CMessage = Proto::FromClient::Message;
	using SMessage = Proto::FromServer::Message;
	Ω fromClient( string&& transmission )ε->Proto::FromClient::Transmission{ return Protobuf::Deserialize<Proto::FromClient::Transmission>( move(transmission) ); }

	TEST( FromClientTests, Query ){
		let variables = jobject{ {"id", 42} };
		let t = fromClient( FromClient::Query("{users{id}}", variables, 9, false) );
		ASSERT_EQ( t.messages_size(), 1 );
		let& m = t.messages( 0 );
		EXPECT_EQ( m.request_id(), 9u );
		ASSERT_EQ( m.value_case(), CMessage::kQuery );
		EXPECT_EQ( m.query().text(), "{users{id}}" );
		EXPECT_EQ( m.query().variables(), serialize(variables) );
		EXPECT_FALSE( m.query().return_raw() );
	}

	//Same payload as a query, a different oneof member - that is all that tells the server to keep the query live.
	TEST( FromClientTests, Subscription ){
		let variables = jobject{ {"id", 42} };
		let t = fromClient( FromClient::Subscription(string{"{users{id}}"}, variables, 10) );
		ASSERT_EQ( t.messages_size(), 1 );
		let& m = t.messages( 0 );
		EXPECT_EQ( m.request_id(), 10u );
		ASSERT_EQ( m.value_case(), CMessage::kSubscription );
		EXPECT_EQ( m.subscription().text(), "{users{id}}" );
		EXPECT_EQ( m.subscription().variables(), serialize(variables) );
	}

	TEST( FromClientTests, AddSession ){
		let t = fromClient( FromClient::AddSession("jde.com", "bob", 3, "127.0.0.1", true, 11) );
		ASSERT_EQ( t.messages_size(), 1 );
		let& m = t.messages( 0 );
		EXPECT_EQ( m.request_id(), 11u );
		ASSERT_EQ( m.value_case(), CMessage::kAddSession );
		EXPECT_EQ( m.add_session().domain(), "jde.com" );
		EXPECT_EQ( m.add_session().login_name(), "bob" );
		EXPECT_EQ( m.add_session().provider_pk(), 3u );
		EXPECT_EQ( m.add_session().user_endpoint(), "127.0.0.1" );
		EXPECT_TRUE( m.add_session().is_socket() );
	}

	TEST( FromClientTests, SessionAndJwtAreBareRequests ){
		let session = fromClient( FromClient::Session(12, 13) );
		ASSERT_EQ( session.messages(0).value_case(), CMessage::kSessionInfo );
		EXPECT_EQ( session.messages(0).session_info(), 12u );
		EXPECT_EQ( session.messages(0).request_id(), 13u );

		let jwt = fromClient( FromClient::Jwt(14) );
		ASSERT_EQ( jwt.messages(0).value_case(), CMessage::kRequestType );
		EXPECT_EQ( jwt.messages(0).request_type(), Proto::FromClient::ERequestType::Jwt );
		EXPECT_EQ( jwt.messages(0).request_id(), 14u );
	}

	TEST( FromClientTests, Instance ){
		let t = FromClient::Instance( "Jde.App.Tests", "instance", 12, 5 );
		ASSERT_EQ( t.messages_size(), 1 );
		EXPECT_EQ( t.messages(0).request_id(), 5u );
		let& i = t.messages( 0 ).instance();
		EXPECT_EQ( i.application(), "Jde.App.Tests" );
		EXPECT_EQ( i.instance_name(), "instance" );
		EXPECT_EQ( i.session_id(), 12u );
		EXPECT_EQ( i.host(), Process::HostName() );
		EXPECT_EQ( i.pid(), (uint32_t)Process::ProcessId() );
		EXPECT_EQ( i.web_port(), 5010u ); ///http/port from the test settings.
		EXPECT_EQ( Protobuf::ToTimePoint(i.start_time()), Process::StartTime() );
	}

	TEST( FromClientTests, Exception ){
		let text = FromClient::Exception( string{"boom"}, 3 );
		ASSERT_EQ( text.messages_size(), 1 );
		EXPECT_EQ( text.messages(0).request_id(), 3u );
		EXPECT_EQ( text.messages(0).exception().what(), "boom" );
		EXPECT_EQ( text.messages(0).exception().code(), 0u );

		let thrown = FromClient::Exception( std::runtime_error{"plain"}, 4 );
		EXPECT_EQ( thrown.messages(0).exception().what(), "plain" );
		EXPECT_EQ( thrown.messages(0).exception().code(), 0u ); //not a Jde::Exception - there is no code to carry.
	}

	//Log traffic is unsolicited:  one message per entry, no request to answer.
	TEST( FromClientTests, LogEntries ){
		vector<Logging::Entry> entries{ entry(tp(0), ELogLevel::Information, 1, "a"), entry(tp(1), ELogLevel::Error, 2, "b") };
		let t = FromClient::LogEntries( move(entries) );
		ASSERT_EQ( t.messages_size(), 2 );
		EXPECT_EQ( t.messages(0).request_id(), 0u );
		ASSERT_EQ( t.messages(0).value_case(), CMessage::kLogEntry );
		EXPECT_EQ( t.messages(0).log_entry().text(), "a" );
		EXPECT_EQ( t.messages(1).log_entry().line(), 2u );
	}

	TEST( FromServerTests, Ack ){
		let t = FromServer::Ack( 42 );
		ASSERT_EQ( t.messages_size(), 1 );
		ASSERT_EQ( t.messages(0).value_case(), SMessage::kAck );
		EXPECT_EQ( t.messages(0).ack(), 42u );
		EXPECT_EQ( t.messages(0).request_id(), 0u ); //the handshake answers no request.
	}

	//"your request is done, there is nothing else coming" - a request id and no payload.
	TEST( FromServerTests, Complete ){
		let t = FromServer::Complete( 7 );
		ASSERT_EQ( t.messages_size(), 1 );
		EXPECT_EQ( t.messages(0).request_id(), 7u );
		EXPECT_EQ( t.messages(0).value_case(), SMessage::VALUE_NOT_SET );
	}

	TEST( FromServerTests, Exception ){
		let t = FromServer::Exception( std::runtime_error{"plain"}, RequestId{8} );
		ASSERT_EQ( t.messages_size(), 1 );
		EXPECT_EQ( t.messages(0).request_id(), 8u );
		EXPECT_EQ( t.messages(0).exception().what(), "plain" );
		EXPECT_EQ( t.messages(0).exception().code(), 0u );
		EXPECT_EQ( t.messages(0).exception().db_error(), 0u ); //not a DBException - "unclassified" and "not one" are the same to the receiver.

		let unsolicited = FromServer::Exception( std::runtime_error{"plain"}, nullopt );
		EXPECT_EQ( unsolicited.messages(0).request_id(), 0u );

		let text = FromServer::Exception( string{"boom"}, nullopt );
		EXPECT_EQ( text.messages(0).exception().what(), "boom" );
	}

	//Forwarded execution carries the user only when there is one;  otherwise it is the anonymous member, not user 0.
	TEST( FromServerTests, ExecuteRequest ){
		let anonymous = FromServer::ExecuteRequest( 3, UserPK{}, string{"payload"} );
		ASSERT_EQ( anonymous.messages_size(), 1 );
		EXPECT_EQ( anonymous.messages(0).request_id(), 3u );
		ASSERT_EQ( anonymous.messages(0).value_case(), SMessage::kExecuteAnonymous );
		EXPECT_EQ( anonymous.messages(0).execute_anonymous(), "payload" );

		let identified = FromServer::ExecuteRequest( 4, UserPK{9}, string{"payload"} );
		EXPECT_EQ( identified.messages(0).request_id(), 4u );
		ASSERT_EQ( identified.messages(0).value_case(), SMessage::kExecute );
		EXPECT_EQ( identified.messages(0).execute().user_pk(), 9u );
		EXPECT_EQ( identified.messages(0).execute().transmission(), "payload" );
	}

	TEST( FromServerTests, ExecuteResponse ){
		let t = FromServer::Execute( string{"result"}, 5 );
		ASSERT_EQ( t.messages(0).value_case(), SMessage::kExecuteResponse );
		EXPECT_EQ( t.messages(0).execute_response(), "result" );
		EXPECT_EQ( t.messages(0).request_id(), 5u );
	}

	TEST( FromServerTests, GraphQLAndSubscription ){
		let query = FromServer::GraphQL( string{R"({"users":[]})"}, 6 );
		ASSERT_EQ( query.messages(0).value_case(), SMessage::kQueryResult );
		EXPECT_EQ( query.messages(0).query_result(), R"({"users":[]})" );
		EXPECT_EQ( query.messages(0).request_id(), 6u );

		let sub = FromServer::Subscription( string{R"({"users":[]})"}, 7 );
		ASSERT_EQ( sub.messages(0).value_case(), SMessage::kSubscription );
		EXPECT_EQ( sub.messages(0).subscription(), R"({"users":[]})" );
	}

	TEST( FromServerTests, SubscriptionAck ){
		let t = FromServer::SubscriptionAck( flat_set<QL::SubscriptionId>{3,1,2}, 8 );
		ASSERT_EQ( t.messages(0).value_case(), SMessage::kSubscriptionAck );
		EXPECT_EQ( t.messages(0).request_id(), 8u );
		let& ids = t.messages( 0 ).subscription_ack().server_ids();
		EXPECT_EQ( Protobuf::ToVector(ids), (vector<uint32_t>{1,2,3}) ); //flat_set - sorted, deduped.
	}

	TEST( FromServerTests, QueryClient ){
		let variables = ms<jobject>( jobject{{"id",42}} );
		let t = FromServer::QueryClient( string{"{users{id}}"}, variables, UserPK{9}, true, 6 );
		ASSERT_EQ( t.messages(0).value_case(), SMessage::kClientQuery );
		EXPECT_EQ( t.messages(0).request_id(), 6u );
		EXPECT_EQ( t.messages(0).client_query().query(), "{users{id}}" );
		EXPECT_EQ( t.messages(0).client_query().variables(), serialize(*variables) );
		EXPECT_TRUE( t.messages(0).client_query().raw() );
		EXPECT_EQ( t.messages(0).client_query().executer_pk(), 9u );

		let none = FromServer::QueryClient( string{"{users{id}}"}, nullptr, UserPK{}, false, 7 );
		EXPECT_TRUE( none.messages(0).client_query().variables().empty() ); //no variables object -> the field is left unset, not "null".
		EXPECT_FALSE( none.messages(0).client_query().raw() );
	}

	TEST( ProtoUtilsTests, ToQuery ){
		let query = ProtoUtils::ToQuery( string{"{users{id}}"}, jobject{{"id",42}}, false );
		EXPECT_EQ( query.text(), "{users{id}}" );
		EXPECT_EQ( query.variables(), serialize(jobject{{"id",42}}) );
		EXPECT_FALSE( query.return_raw() );
	}
}
