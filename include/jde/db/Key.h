#pragma once

namespace Jde::DB{
	struct Key{
		Key( uint id )ι:_key{id}{}
		Key( string target )ι:_key{target}{}
		α IsPK()Ι{ return _key.index()==0; }
		α PK()Ι->uint{ return get<uint>(_key); }
		α NK()Ι->str{ return const_cast<Key*>(this)->NK(); }
		α NK()ι->string&{ return get<string>(_key); }
		α QLVariables()Ι->jobject{ return IsPK() ? jobject{ {"id", PK()} } : jobject{ {"target", NK()} }; }
		α QLInput()Ι->sv{ return IsPK() ? "id: $id" : "target: $target"; }
		private:
			variant<uint,string> _key;
	};
}