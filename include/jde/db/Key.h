#pragma once

namespace Jde::DB{
	struct Key{
		Key( uint id )ι:_key{id}{}
		Key( string slug )ι:_key{slug}{}
		α IsPK()Ι{ return _key.index()==0; }
		α PK()Ι->uint{ return get<uint>(_key); }
		α NK()Ι->str{ return get<string>(_key); }
		α NK()ι->string&{ return const_cast<string&>( std::as_const(*this).NK() ); }
		α QLVariables()Ι->jobject{ return IsPK() ? jobject{ {"id", PK()} } : jobject{ {"slug", NK()} }; }
		α QLInput()Ι->sv{ return IsPK() ? "id: $id" : "slug: $slug"; }
		private:
			variant<uint,string> _key;
	};
}