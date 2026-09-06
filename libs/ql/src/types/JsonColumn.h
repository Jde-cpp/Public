#pragma once

namespace Jde::DB{ struct Column; struct View; }
namespace Jde::QL{
	//A db column as json spells it:  the member name a client's mutation uses for it, and the view its value lives in - the enum or
	//flags table for a lookup column, else its own.  The write side's mirror of ColumnQL (include/jde/ql/types/TableQL.h).
	struct JsonColumn final{
		JsonColumn( sp<DB::Column> c )ι: Column{c}{}
		α Table()Ι->const DB::View&;
		α MemberName()Ι->string;
		sp<DB::Column> Column;
	};
}