//BrowseName wraps UA_QualifiedName and owns the name buffer.  Two spellings feed it: the json {ns,name} object and the
//"2~Tag" fully-qualified form used in config.
#include <cstddef>
#include <new>
#include <gtest/gtest.h>
#include <jde/opc/uatypes/BrowseName.h>

#define let const auto

namespace Jde::Opc::Tests{
	TEST( BrowseNameTests, FromJson ){
		let j = parse( R"({"ns":2,"name":"Tag"})" ).as_object();
		BrowseName b{ j };
		EXPECT_EQ( b.namespaceIndex, 2 );
		EXPECT_EQ( ToSV(b.name), "Tag" );
		EXPECT_EQ( b.PK, 0u );

		let noNamespace = BrowseName{ parse(R"({"name":"Tag"})").as_object() };
		EXPECT_EQ( noNamespace.namespaceIndex, 0 );
		EXPECT_THROW( BrowseName{ parse(R"({"ns":2})").as_object() }, Exception ); //name is required.
	}

	TEST( BrowseNameTests, FullyQualifiedForm ){
		BrowseName qualified{ "2~Tag", 0 };
		EXPECT_EQ( qualified.namespaceIndex, 2 );
		EXPECT_EQ( ToSV(qualified.name), "Tag" );

		BrowseName defaulted{ "Tag", 3 }; //no '~': the default namespace applies.
		EXPECT_EQ( defaulted.namespaceIndex, 3 );
		EXPECT_EQ( ToSV(defaulted.name), "Tag" );
	}

	TEST( BrowseNameTests, ToJson ){
		BrowseName b{ 0u, 2, "Tag" };
		let j = b.ToJson();
		EXPECT_EQ( j.at("ns").to_number<int>(), 2 );
		EXPECT_EQ( j.at("name").as_string(), "Tag" );
		EXPECT_FALSE( j.contains("pk") ); //PK 0 is omitted.

		BrowseName withPk{ 7u, 2, "Tag" };
		EXPECT_EQ( withPk.ToJson().at("pk").to_number<int>(), 7 );
		EXPECT_EQ( withPk.ToString(), R"({"ns":2,"name":"Tag","pk":7})" );

		BrowseName nameless{ 0u, 0, "" };
		EXPECT_TRUE( nameless.ToJson().empty() ); //an empty name drops the whole object, ns included.
	}

	TEST( BrowseNameTests, CopyIsDeepMoveTransfers ){
		BrowseName src{ 7u, 2, "Tag" };
		BrowseName copy{ src };
		EXPECT_EQ( ToSV(copy.name), "Tag" );
		EXPECT_EQ( copy.PK, 7u );
		EXPECT_NE( copy.name.data, src.name.data );

		BrowseName moved{ move(src) };
		EXPECT_EQ( ToSV(moved.name), "Tag" );
		EXPECT_EQ( moved.PK, 7u );
		EXPECT_EQ( src.name.data, nullptr );
	}

	TEST( BrowseNameTests, Assignment ){
		BrowseName a{ 0u, 2, "a" };
		let b = BrowseName{ 5u, 2, "b" };
		a = b;
		EXPECT_EQ( ToSV(a.name), "b" );
		EXPECT_EQ( a.PK, 5u );
		EXPECT_NE( a.name.data, b.name.data );

		BrowseName c{ 0u, 2, "c" };
		a = move( c );
		EXPECT_EQ( ToSV(a.name), "c" );
		EXPECT_EQ( c.name.data, nullptr );

		auto& alias = a; //through a reference: `a = a` is -Wself-assign-overloaded, and the guard is what is under test.
		a = alias;
		EXPECT_EQ( ToSV(a.name), "c" );
	}

	//operator== takes the raw UA_QualifiedName base, so `wrapper==wrapper` is -Wambiguous-reversed-operator under C++20
	//(the reversed candidate is an equally good match) and does not compile under -Werror.  Comparing against the base is
	//the spelling that works; the library would need a free `operator==( const BrowseName&, const BrowseName& )` - as
	//NodeId and ExNodeId already have - for the wrapper-to-wrapper form.
	TEST( BrowseNameTests, EqualityIgnoresThePk ){
		BrowseName a{ 0u, 2, "Tag" };
		BrowseName samePk{ 5u, 2, "Tag" };
		EXPECT_TRUE( a==static_cast<const UA_QualifiedName&>(samePk) );

		BrowseName otherNamespace{ 0u, 3, "Tag" };
		EXPECT_FALSE( a==static_cast<const UA_QualifiedName&>(otherNamespace) );

		BrowseName otherName{ 0u, 2, "Other" };
		EXPECT_FALSE( a==static_cast<const UA_QualifiedName&>(otherName) );
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#5: `BrowseName()ι = default` gives PK an NSDMI but leaves the UA_QualifiedName base indeterminate under
	//default-initialization - every sibling wrapper (NodeId(), ExNodeId(), LocalizedText()) zeroes its base.  OpcServer
	//declares `BrowseName Browse;` with no initializer and five ctors omit it from their init lists, so on recycled heap
	//~BrowseName frees a garbage name.data (and assigning to it first is just as fatal - operator= clears the target).
	//Placement-new over poisoned storage is the only way to observe this deterministically; the ASSERTs come before the
	//explicit destructor call so a failing run does not free a wild pointer.  Fix: `BrowseName()ι:UA_QualifiedName{}{}`.
	TEST( BrowseNameTests, DISABLED_R2_5_DefaultConstructorZeroesTheBase ){
		alignas(BrowseName) std::byte storage[sizeof(BrowseName)];
		::memset( storage, 0xCD, sizeof(storage) );
		auto* p = ::new (static_cast<void*>(storage)) BrowseName;
		ASSERT_EQ( p->name.length, 0u );
		ASSERT_EQ( p->name.data, nullptr );
		EXPECT_EQ( p->namespaceIndex, 0 );
		EXPECT_EQ( p->PK, 0u );
		p->~BrowseName();
	}
}
