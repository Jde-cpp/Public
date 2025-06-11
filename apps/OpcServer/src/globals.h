namespace Jde::Opc::Server {
	struct UAServer;

	α DataType( uint i )ε->const UA_DataType&;
	α DS()ι->DB::IDataSource&;
	α GetView( str name )ε->const DB::View&;
	α GetViewPtr( str name )ε->sp<DB::View>;
	α ServerName()ι->str;
	α SetServerName( str serverName )ι->void;
	α SetSchema( sp<DB::AppSchema> schema )ι->void;
	α GetUAServer()ι->UAServer&;
}