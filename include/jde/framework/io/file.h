#pragma once
#ifndef FILE_H
#define FILE_H
#include <string>
#include <functional>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <vector>
#include <boost/container/flat_map.hpp>
#include "../exports.h"
#include "../../../../../Framework/source/threading/Worker.h"
#include "../../../../../Framework/source/io/DiskWatcher.h"
#include "../../../../../Framework/source/io/FileCo.h"

#define Φ Γ auto
namespace Jde{ struct Stopwatch; }
namespace Jde::IO{
	Φ Native()ι->IO::IDrive&;
	Ξ Read( const fs::path& path, bool vector=true, bool cache=false, SRCE )ι{ return DriveAwaitable{path, vector, cache, sl}; }
	Ξ Write( fs::path path, sp<vector<char>> data, SRCE )ι{ return DriveAwaitable{move(path), data, sl}; }
	Ξ Write( fs::path path, sp<string> data, SRCE )ι{ return DriveAwaitable{move(path), data, sl}; }
	Φ Remove( const fs::path& path, SRCE )ε->void;

	Φ FileSize( const fs::path& path )ε->uint;

	Φ Copy( fs::path from, fs::path to, SRCE )->PoolAwait;
	Φ CopySync( fs::path from, fs::path to, SRCE )->void;
	Φ CreateDirectories( fs::path path )ε->void;
	Φ ForEachLine( const fs::path& p, function<void(const vector<double>&, uint lineIndex)> f, const flat_set<uint>& columnIndexes, uint maxLines=std::numeric_limits<uint>::max(), size_t startLine=0, size_t chunkSize=1073741824, size_t maxColumnCount=1500, Stopwatch* sw=nullptr, double emptyValue=0.0 )ε->uint;

	Φ LoadColumnNames( const fs::path& csvFileName, const vector<string>& columnNamesToFetch, bool notColumns=false )->tuple<vector<string>,flat_set<uint>>;

	namespace FileUtilities{
		Φ LoadBinary( const fs::path& path, SRCE )ε->up<std::vector<char>>;
		Φ Load( const fs::path& path, SRCE )ε->string;
		Φ SaveBinary( const fs::path& path, const std::vector<char>& values )ε->void;
		Φ Save( fs::path path, sp<string> value, SRCE )ε->void;
		Φ Save( const fs::path& path, sv value, std::ios_base::openmode openMode = std::ios_base::out, SRCE )ε->void;
		Ξ SaveBinary( const fs::path& path, sv value, SRCE )ε{ Save(path, value, std::ios::binary, sl); }
		Γ void ForEachItem( const fs::path& directory, std::function<void(const fs::directory_entry&)> function )ε;//todo get rid of, 1 liner
		Γ up<std::set<fs::directory_entry>> GetDirectory( const fs::path& directory );
		Γ up<std::set<fs::directory_entry>> GetDirectories( const fs::path& directory, up<std::set<fs::directory_entry>> pItems=nullptr );
		Γ std::string ToString( const fs::path& pszFilePath );
		Φ LoadColumnNames( const fs::path& csvFileName )->vector<string>;
		Γ std::string DateFileName( uint16 year, uint8 month=0, uint8 day=0 )ι;
		Γ tuple<uint16,uint8,uint8> ExtractDate( const fs::path& path )ι;

		Γ void Replace( const fs::path& source, const fs::path& destination, const flat_map<string,string>& replacements )ε;

		Γ void CombineFiles( const fs::path& csvFileName );

		template<typename TCollection>
		void SaveColumnNames( const fs::path& path, const TCollection& columns );

/*		struct Γ Compression //TODO See if this is used.
		{
			void Save( const fs::path& path, const std::vector<char>& data )ε;
			virtual fs::path Compress( const fs::path& path, bool deleteAfter=true )ε;
			virtual void Extract( const fs::path& path )ε;
			up<std::vector<char>> LoadBinary( const fs::path& uncompressed, const fs::path& compressed=fs::path(), bool setPermissions=false, bool leaveUncompressed=false )ε;
			virtual const char* Extension()ι=0;
			virtual bool CompressAutoDeletes(){return true;}
		protected:
			virtual std::string CompressCommand( const fs::path& path )ι=0;
			virtual std::string ExtractCommand( const fs::path& compressedFile, fs::path destination )ι=0;
		};
		struct SevenZ : Compression
		{
			const char* Extension()ι final override{return ".7z";}
			std::string CompressCommand( path path )ι  override{ return Jde::format( "7z a -y -bsp0 -bso0 {0}.7z  {0}", path.string() ); };
			std::string ExtractCommand( path compressedFile, fs::path destination )ι override{ return Jde::format( "7z e {} -o{} -y -bsp0 -bso0", compressedFile.string(), destination.string() ); }
		};

		struct XZ : Compression
		{
			const char* Extension()ι final override{return ".xz";}
			std::string CompressCommand( path path )ι final override{ return Jde::format( "xz -z -q {}", path.string() ); };
			std::string ExtractCommand( path compressedFile, fs::path destination )ι final override{ return Jde::format( "xz -d -q -k {}", compressedFile.string() ); }
		};
		struct Zip : Compression
		{
			const char* Extension()ι final override{return ".zip";}
			std::string CompressCommand( path path )ι final override{ return Jde::format( "zip -q -m -j {0}.zip {0}", path.string() ); };
#ifdef _MSC_VER
			std::string ExtractCommand( path compressedFile, fs::path destination )ι final override{ return Jde::format( "\"C:\\Program Files\\7-Zip\\7z\" e {0} -y -o{1} > NUL: & del {0}", compressedFile.string(), destination.string() ); }
#else
			std::string ExtractCommand( path compressedFile, fs::path destination )ι final override{ return Jde::format( "unzip -n -q -d{} {}", destination.string(), compressedFile.string() ); }
#endif
		};
*/
	}
	namespace File{
		uint GetFileSize( std::ifstream& file );
		uint Merge( const fs::path& file, const vector<char>& original, const vector<char>& newData )ε;
	};

	template<typename TCollection>
	void FileUtilities::SaveColumnNames( const fs::path& path, const TCollection& columns ){
		std::ofstream os( path );
		auto first = true;
		for( const auto& column : columns )
		{
			if( first )
				first = false;
			else
				os << ",";
			os << column;
		}
		os << std::endl;
	}
}
#undef Φ
#endif