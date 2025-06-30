#pragma once
#include <jde/db/exports.h>

namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	struct Catalog; struct DBSchema; struct IDataSource;  struct Syntax; struct Table; struct View;
	struct ΓDB AppSchema{
		AppSchema( sv name, const jobject& appSchema, sp<Access::IAcl> authorizer )ε;
		AppSchema( sv name, flat_map<string,sp<Table>> tables, sv prefix )ι:Name{name},Prefix{prefix},Tables{tables}{}

		Ω Initialize( sp<DB::DBSchema> db, sp<AppSchema> self )ε->void;
		α ConfigPath()Ι->string;
		α DS()Ε->sp<IDataSource>;
		α ResetDS()Ι->void; //testing schema doesn't exist at startup.
		α Syntax()Ι->const Syntax&;

		α FindTable( str name )Ι->sp<Table>;
		α GetTable( str name, SRCE )Ε->const Table&;
		α GetTablePtr( str name, SRCE )Ε->sp<Table>;

		α FindView( str name )Ι->sp<View>;
		α GetView( str name, SRCE )ε->const View&;
		α GetViewPtr( str name, SRCE )ε->sp<View>;

		α FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>;
		α ObjectPrefix()Ι->string;
		α DBName( str objectName )Ι->string;

		string Name;
		sp<DB::DBSchema> DBSchema;
		sp<Access::IAcl> Authorizer;
		string Prefix;
		flat_map<string,sp<Table>> Tables;
		flat_map<string,sp<View>> Views;
	private:
		AppSchema( sv name, sv prefix, const jobject& meta, sp<Access::IAcl> authorizer )ε;
	};
}