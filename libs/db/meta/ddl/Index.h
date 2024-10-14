#pragma once


namespace Jde::DB{
	struct Syntax; struct Table;
	struct Index final{
		Index( sv indexName, sv tableName, bool primaryKey, const vector<string>* pColumns=nullptr, bool unique=true, optional<bool> clustered=optional<bool>{} )ι;//, bool clustered=false
		Index( sv indexName, sv tableName, const Index& other )ι;

		Ω GetConfig( const Table& t )ι->vector<Index>;
		α Create( sv name, sv tableName, const Syntax& syntax )Ι->string;
		string Name;
		string TableName;
		vector<string> Columns;
		bool Clustered;
		bool Unique;
		bool PrimaryKey;
	};
}