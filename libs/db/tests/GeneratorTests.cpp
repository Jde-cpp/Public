#include <gtest/gtest.h>
#include <jde/db/db.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Object.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include "../src/meta/ddl/ColumnDdl.h" //#39: the deleted-overload guard.

namespace Jde::DB::Tests{
	//#10: SchemaDdl::Sync now delegates to ObjectPrefix (was a swapped-ternary duplicate that yielded "" for a "db.um_" prefix). Lock in ObjectPrefix.
	TEST( AppSchemaTests, ObjectPrefix ){
		EXPECT_EQ( (AppSchema{ "s", {}, "db.um_" }).ObjectPrefix(), "um_" ); //MSSQL-style schema.prefix -> the object prefix after the dot.
		EXPECT_EQ( (AppSchema{ "s", {}, "um_" }).ObjectPrefix(), "um_" );    //no dot -> unchanged.
		EXPECT_EQ( (AppSchema{ "s", {}, "" }).ObjectPrefix(), "" );          //empty -> empty.
	}

	//#26: InsertClause::Proc placeholder count must match the params (was a leading "(?" -> zero Values gave `name(?)` with 0 params), and SequenceColumn must not deref a null column.
	TEST( InsertClauseTests, ProcParamCountMatches ){
		EXPECT_EQ( InsertClause( "p", vector<Value>{} ).Move().Text, "p()" );          //0 values -> no placeholder (was "p(?)").
		EXPECT_TRUE( InsertClause( "p", vector<Value>{} ).Move().Params.empty() );
		auto sql = InsertClause( "p", vector<Value>{ Value{1}, Value{2} } ).Move();
		EXPECT_EQ( sql.Text, "p(?,?)" );
		EXPECT_EQ( sql.Params.size(), 2u );
	}
	TEST( InsertClauseTests, SequenceColumnNullColumnSafe ){
		InsertClause ins{ "p", vector<Value>{ Value{1} } }; //proc ctor -> Values have null columns.
		EXPECT_EQ( ins.SequenceColumn(), nullptr );          //was a null-column ->Table deref (segfault).
	}

	//#33: getMap() runs from Table's member-init list and FindColumn's "id" alias reads SurrogateKeys, which used to be declared *after* Map - so the read was of a not-yet-constructed vector.
	//The surrogate key is deliberately not named "id": with the old order the garbage size() sends FindColumn to the Columns search, which has no "id", and GetColumnPtr throws.
	TEST( TableTests, MapIdAliasResolvesSurrogateKey ){
		const auto j = Json::Parse( R"({"columns":{"entity_id":{"sk":0,"i":0},"member_id":{"i":1}},"map":{"parentId":"id","childId":"member_id"}})" );
		const Table t{ "m", j };
		ASSERT_EQ( t.SurrogateKeys.size(), 1u );
		ASSERT_TRUE( t.Map.has_value() );
		EXPECT_EQ( t.Map->Parent, t.SurrogateKeys[0] ); //"id" resolved through the surrogate key, not by column name.
		EXPECT_EQ( t.Map->Parent->Name, "entity_id" );
		EXPECT_EQ( t.Map->Child->Name, "member_id" );
	}

	//#39: MySQL's loadTables passed `isId!=0` where ColumnDdl takes `optional<uint8> skIndex`, and a bool converts to an
	//*engaged* optional{0} - "column 0 of the primary key", not "no primary key" - so every MySQL-loaded column claimed
	//to be the first pk column.  It sat next to `isIdentity!=0`, which is spelled identically and is genuinely a bool.
	//The all-bool spelling is now deleted, so getting it wrong is a compile error rather than a wrong answer.
	TEST( ColumnDdlTests, SkIndexRejectsBool ){
		static_assert( std::is_constructible_v<ColumnDdl, sv, optional<Value>, bool, EType, optional<uint>, bool, optional<uint8>, optional<uint>, optional<uint>>,
			"the real signature has to stay callable." );
		static_assert( !std::is_constructible_v<ColumnDdl, sv, optional<Value>, bool, EType, optional<uint>, bool, bool, optional<uint>, optional<uint>>,
			"a bool in skIndex's place must not compile - it means optional{0}, which is a pk claim." );

		const ColumnDdl noKey{ "c", {}, true, EType::Int, {}, false, optional<uint8>{}, {}, {} };
		EXPECT_FALSE( noKey.SKIndex.has_value() ); //what a non-pk column must report.
		const ColumnDdl firstKey{ "c", {}, true, EType::Int, {}, false, optional<uint8>{0}, {}, {} };
		ASSERT_TRUE( firstKey.SKIndex.has_value() );
		EXPECT_EQ( (uint)*firstKey.SKIndex, 0u ); //(uint): uint8 formats as a character.
	}

	//db-refactor A8: one ctor for every loader, and it keeps everything the server reported - the precision/scale pair
	//used to be discarded on the way in, the length defaulted to an engaged 0 on one dialect, and the two bools the
	//configured column initialises were left indeterminate.
	TEST( ColumnDdlTests, KeepsWhatTheLoaderPassed ){
		const ColumnDdl c{ "amount", Value{true}, false, EType::Decimal, optional<uint>{}, false, optional<uint8>{}, optional<uint>{10}, optional<uint>{2} };
		ASSERT_TRUE( c.Default.has_value() ); EXPECT_TRUE( c.Default->get_bool() );
		EXPECT_FALSE( c.IsNullable );
		EXPECT_EQ( c.Type, EType::Decimal );
		EXPECT_FALSE( c.MaxLength.has_value() ); //no length stays no length, not 0.
		EXPECT_EQ( c.NumericPrecision.value_or(0), 10u );
		EXPECT_EQ( c.NumericScale.value_or(0), 2u );
		EXPECT_FALSE( c.IsSequence );
		EXPECT_TRUE( c.Insertable ); EXPECT_TRUE( c.Updateable ); //the configured column's defaults.
		const ColumnDdl seq{ "id", {}, false, EType::UInt, {}, true, optional<uint8>{0}, {}, {} };
		EXPECT_FALSE( seq.Insertable ); //a sequence is not inserted, as Column's json ctor decides.
		EXPECT_FALSE( seq.Default.has_value() );
	}

	//db-refactor B1: the placeholder ctors - Column::Count, a criteria column, the pkTable/extends/qlView stand-ins, every
	//loader-built TableDdl - left the bools and the EType indeterminate, and SelectClause::ToString reads Count()'s Type.
	//Default member initialisers answer the configured column's own defaults; this pins them (it cannot fail on a build
	//that happens to zero the frame, but it says what the answers are).
	TEST( ColumnTests, PlaceholdersAreDeterminate ){
		const auto count = Column::Count();
		EXPECT_EQ( count->Type, EType::None );
		EXPECT_FALSE( count->IsNullable ); EXPECT_FALSE( count->IsSequence );
		EXPECT_TRUE( count->Insertable ); EXPECT_TRUE( count->Updateable ); //the json ctor's value_or defaults.
		const Table v{ "placeholder" };
		EXPECT_FALSE( v.HasCustomInsertProc ); EXPECT_FALSE( v.IsFlags );
		EXPECT_TRUE( v.Operations==Access::ERights{} );
	}

	//ql-review3 #3: TryAdd read join.To->Table->Name unconditionally, but a null To is a legal Join - it is the single-table
	//form operator+= merges into, and Contains/GetColumnPtr both allow for it.  A sub-table whose fk could not be resolved
	//passed one in and segfaulted here rather than at the caller.
	TEST( FromClauseTests, TryAddNullToDoesNotDereference ){
		FromClause from;
		from.TryAdd( Join{ms<Column>("a"), nullptr, false} );
		ASSERT_EQ( from.Joins.size(), 1u );
		EXPECT_FALSE( from.HasJoin() ); //a single-table from clause, not a join.
	}

	TEST( ObjectTests, ValueEquality ){
		const DB::Object a = DB::Value{5}, b = DB::Value{5}, c = DB::Value{6};
		EXPECT_TRUE( a==b );
		EXPECT_FALSE( a==c );

		const DB::Object v1 = vector<DB::Value>{ DB::Value{1}, DB::Value{2} };
		const DB::Object v2 = vector<DB::Value>{ DB::Value{1}, DB::Value{2} };
		const DB::Object v3 = vector<DB::Value>{ DB::Value{1} };
		EXPECT_TRUE( v1==v2 );
		EXPECT_FALSE( v1==v3 );
		EXPECT_FALSE( a==v1 ); //different alternatives.
	}

	//#25: an Object holding Values must render placeholders (?,?), not the literal `[ 1, 2]` - GetParams appends the values, so the counts must match.
	//ql-review3 C9: the glob bracket-class grammar used to be written out three times - GlobToLike, GlobToRegex and
	//QL::matchClass.  It is one function now, so this is where its rules live.  `Body` excludes the '[', the negation and
	//the ']', and `Close` indexes the ']'.
	TEST( SyntaxTests, ParseGlobClass ){
		const auto parse = []( sv glob, uint open=0 ){ return ParseGlobClass( glob, open ); };
		auto c = parse( "[abc]" );          ASSERT_TRUE( c ); EXPECT_FALSE( c->Negate ); EXPECT_EQ( c->Body, "abc" ); EXPECT_EQ( c->Close, 4u );
		c = parse( "[a-c]" );               ASSERT_TRUE( c ); EXPECT_EQ( c->Body, "a-c" );   //ranges are body text - only the ql matcher interprets them.
		c = parse( "[!a-c]" );              ASSERT_TRUE( c ); EXPECT_TRUE( c->Negate ); EXPECT_EQ( c->Body, "a-c" ); EXPECT_EQ( c->Close, 5u );
		c = parse( "[^a-c]" );              ASSERT_TRUE( c ); EXPECT_TRUE( c->Negate ); EXPECT_EQ( c->Body, "a-c" ); //both spellings of negation.
		c = parse( "[]x]" );                ASSERT_TRUE( c ); EXPECT_EQ( c->Body, "]x" ); EXPECT_EQ( c->Close, 3u ); //a ']' first is a member, not the terminator.
		c = parse( "[!]x]" );               ASSERT_TRUE( c ); EXPECT_TRUE( c->Negate ); EXPECT_EQ( c->Body, "]x" ); //…including right after the negation.
		c = parse( "[a-]" );                ASSERT_TRUE( c ); EXPECT_EQ( c->Body, "a-" );    //a trailing '-' spans nothing, so it is a member.
		c = parse( "x[ab]y", 1 );           ASSERT_TRUE( c ); EXPECT_EQ( c->Body, "ab" ); EXPECT_EQ( c->Close, 4u ); //not only at index 0.
		EXPECT_FALSE( parse("[abc") );      //unterminated: the caller treats the '[' as a literal, as sqlite does.
		EXPECT_FALSE( parse("[") );
		EXPECT_FALSE( parse("[]") );        //that ']' is a member, so the class is still unterminated.
	}

	TEST( ObjectTests, ValuesRendersPlaceholders ){
		DB::Object o = vector<DB::Value>{ DB::Value{1}, DB::Value{2}, DB::Value{3} };
		EXPECT_EQ( DB::ToString(o), "(?,?,?)" );
		EXPECT_EQ( DB::GetParams(o).size(), 3u ); //placeholder count now matches param count.
	}
}