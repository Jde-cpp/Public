#pragma once

namespace Jde::DB{ struct Column; struct View; } 
namespace Jde::QL{
  struct QLColumn final{
    QLColumn( sp<DB::Column> c )ι: Column{c}{}
    α Table()Ι->const DB::View&;
    α MemberName()Ι->string;
    sp<DB::Column> Column;
  };
}