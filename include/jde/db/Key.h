#pragma once

namespace Jde::DB{
	struct Key{
		Key( uint id )ι:_key{id}{}
		Key( string name )ι:_key{name}{}
		α IsPrimary()Ι{ return _key.index()==0; }
		α PK()Ι->uint{ return get<uint>(_key); }
		α NK()Ι->str{ return get<string>(_key); }
		private:
			variant<uint,string> _key;
	};
}