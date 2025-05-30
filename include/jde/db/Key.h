#pragma once

namespace Jde::DB{
	struct Key{
		Key( uint id )ι:_key{id}{}
		Key( string target )ι:_key{target}{}
		α IsPrimary()Ι{ return _key.index()==0; }
		α PK()Ι->uint{ return get<uint>(_key); }
		α NK()Ι->str{ return const_cast<Key*>(this)->NK(); }
		α NK()ι->string&{ return get<string>(_key); }
		private:
			variant<uint,string> _key;
	};
}