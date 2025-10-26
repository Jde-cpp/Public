#pragma once
#include <jde/fwk/io/FileAwait.h>
#include "../../../io/IDrive.h"
namespace Jde::IO{

	struct WindowsDrive final : IDrive{
		α Get( const fs::path& path )ε->up<IDirEntry> override;
		α Recursive( const fs::path& dir, SRCE )ε->flat_map<string,up<IDirEntry>> override;
		up<IDirEntry> Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )ε override;
		α CreateFolder( const fs::path& path, const IDirEntry& dirEntry )ε->up<IDirEntry> override;
		void Remove( const fs::path& /*path*/ )ε override{THROW( "Not Implemented" );}
		void Trash( const fs::path& /*path*/ )ε override{THROW( "Not Implemented" );}
		void TrashDisposal( TimePoint /*latestDate*/ )override{THROW( "Not Implemented" );}
		α Load( const IDirEntry& dirEntry )ε->vector<char> override;
		void Restore( sv )ε override{ THROW("Not Implemented"); }
		void SoftLink( const fs::path& from, const fs::path& to )ε override;
	};
}