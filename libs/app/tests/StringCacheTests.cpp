//The process-wide id->text maps the app server fills from client log traffic and the `logs` query reads back.  The maps
//are globals, so every test here uses text of its own rather than resetting them.
#include <gtest/gtest.h>
#include <jde/app/StringCache.h>

#define let const auto

namespace Jde::App::Tests{
	using Log::Proto::EFields;
	Ω id( sv text )ι->StringMd5{ return Logging::Entry::GenerateId( text ); }

	TEST( StringCacheTests, RoundTripsEachKind ){
		EXPECT_TRUE( StringCache::AddFile(id("cache/file.cpp"), "cache/file.cpp") );
		EXPECT_TRUE( StringCache::AddFunction(id("CacheFunction"), "CacheFunction") );
		EXPECT_TRUE( StringCache::AddMessage(id("cache message"), "cache message") );

		EXPECT_EQ( StringCache::GetFile(id("cache/file.cpp")), "cache/file.cpp" );
		EXPECT_EQ( StringCache::GetFunction(id("CacheFunction")), "CacheFunction" );
		EXPECT_EQ( StringCache::GetMessage(id("cache message")), "cache message" );
	}

	//The three maps are independent - the same id in one says nothing about the others.
	TEST( StringCacheTests, KindsAreSeparate ){
		StringCache::AddFile( id("separate"), "separate" );
		EXPECT_EQ( StringCache::GetFile(id("separate")), "separate" );
		EXPECT_EQ( StringCache::GetFunction(id("separate")), "" );
		EXPECT_EQ( StringCache::GetMessage(id("separate")), "" );
	}

	TEST( StringCacheTests, UnknownIdIsEmpty ){
		EXPECT_EQ( StringCache::GetMessage(id("never added to the cache")), "" );
	}

	//The return is "the caller should persist this" - false once the string is already known.
	TEST( StringCacheTests, SecondAddIsNotASave ){
		EXPECT_TRUE( StringCache::AddMessage(id("added twice"), "added twice") );
		EXPECT_FALSE( StringCache::AddMessage(id("added twice"), "added twice") );
	}

	//A client may send the text without the id;  the cache generates it, and it has to be the one the senders compute.
	TEST( StringCacheTests, EmptyIdIsGenerated ){
		EXPECT_TRUE( StringCache::AddMessage(uuid{}, "generated id") );
		EXPECT_EQ( StringCache::GetMessage(id("generated id")), "generated id" );
	}

	TEST( StringCacheTests, AddRoutesOnTheField ){
		EXPECT_TRUE( StringCache::Add(EFields::Template, id("routed template"), "routed template", ELogTags::Test) );
		EXPECT_TRUE( StringCache::Add(EFields::File, id("routed/file.cpp"), "routed/file.cpp", ELogTags::Test) );
		EXPECT_TRUE( StringCache::Add(EFields::Function, id("RoutedFunction"), "RoutedFunction", ELogTags::Test) );

		EXPECT_EQ( StringCache::GetMessage(id("routed template")), "routed template" );
		EXPECT_EQ( StringCache::GetFile(id("routed/file.cpp")), "routed/file.cpp" );
		EXPECT_EQ( StringCache::GetFunction(id("RoutedFunction")), "RoutedFunction" );
	}

	//Nothing to save, and nothing cached, for input the cache can't place.
	TEST( StringCacheTests, AddRejectsEmptyValuesAndUnknownFields ){
		EXPECT_FALSE( StringCache::Add(EFields::Template, id("rejected"), "", ELogTags::Test) );
		EXPECT_FALSE( StringCache::Add(EFields::Args, id("rejected"), "rejected", ELogTags::Test) ); //args are per-entry, never cached here.
		EXPECT_EQ( StringCache::GetMessage(id("rejected")), "" );
	}

	//Merge is the bulk load the app server does at startup;  it must not overwrite what is already cached.
	TEST( StringCacheTests, MergeAddsWithoutOverwriting ){
		StringCache::AddMessage( id("merge existing"), "merge existing" );

		concurrent_flat_map<StringMd5,string> files, functions, messages;
		messages.emplace( id("merge existing"), "clobbered" );
		messages.emplace( id("merge new"), "merge new" );
		files.emplace( id("merge/file.cpp"), "merge/file.cpp" );
		functions.emplace( id("MergeFunction"), "MergeFunction" );
		StringCache::Merge( std::move(files), std::move(functions), std::move(messages) ); //qualified: boost::move is an ADL match for the boost map.

		EXPECT_EQ( StringCache::GetMessage(id("merge existing")), "merge existing" );
		EXPECT_EQ( StringCache::GetMessage(id("merge new")), "merge new" );
		EXPECT_EQ( StringCache::GetFile(id("merge/file.cpp")), "merge/file.cpp" );
		EXPECT_EQ( StringCache::GetFunction(id("MergeFunction")), "MergeFunction" );
	}
}
