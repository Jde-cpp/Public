
namespace Jde::DB{
	struct WhereClause final{
		WhereClause()ι=default;
		WhereClause( sv init )ι{ Add(init); }
		friend α operator<<( WhereClause &self, sv clause )ι->WhereClause&{ self.Add(clause); return self; }
		α Add( sv x )ι->void{ if( x.size() )_clauses.push_back(string{x}); }
		α Add( str columnName, DB::object param )ι->void{ if( columnName.size() ){Add( columnName+"=?" ); _parameters.push_back(move(param));} }
		α Move()ι->string;

		α Parameters()ι->vector<DB::object>&{ return _parameters; }
		α Parameters()Ι->const vector<DB::object>&{ return _parameters; }
		α Size()Ι->uint{ return _clauses.size(); }
	private:
		vector<string> _clauses;
		vector<DB::object> _parameters;
	};

	α WhereClause::Move()ι->string{
		string prefix = _clauses.size() ? "where " : "";
		return prefix + Str::AddSeparators( move(_clauses), " and " );
	}	
}