﻿#pragma once
#include <string>
#include <functional>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <jde/Exception.h>
#include <jde/Exports.h>
#include "../../../Framework/source/threading/Worker.h"
#include "../../../Framework/source/io/DiskWatcher.h"
#include "../../../Framework/source/io/FileCo.h"

#define Φ Γ auto
namespace Jde{ struct Stopwatch; }
namespace Jde::IO
{
	Φ Native()noexcept->IO::IDrive&;
	Ξ Read( path path, bool vector=true, bool cache=false, SRCE )noexcept{ return DriveAwaitable{path, vector, cache, sl}; }
	Ξ Write( fs::path path, sp<vector<char>> data, SRCE )noexcept{ return DriveAwaitable{move(path), data, sl}; }
	Ξ Write( fs::path path, sp<string> data, SRCE )noexcept{ return DriveAwaitable{move(path), data, sl}; }
	Φ FileSize( path path )ε->uint;

	Φ Copy( fs::path from, fs::path to, SRCE )->PoolAwait;
	Φ CopySync( fs::path from, fs::path to, SRCE )->void;
	Φ CreateDirectories( fs::path path )ε->void;

	namespace FileUtilities
	{
		Φ LoadBinary( path path, SRCE )ε->up<std::vector<char>>;
		Φ Load( path path, SRCE )ε->string;
		Φ SaveBinary( path path, const std::vector<char>& values )ε->void;
		Φ Save( fs::path path, sp<string> value, SRCE )ε->void;
		Φ Save( path path, sv value, std::ios_base::openmode openMode = std::ios_base::out, SRCE )ε->void;
		Ξ SaveBinary( path path, sv value, SRCE )ε{ Save(path, value, std::ios::binary, sl); }
		Γ void ForEachItem( path directory, std::function<void(const fs::directory_entry&)> function )ε;//todo get rid of, 1 liner
		Γ up<std::set<fs::directory_entry>> GetDirectory( path directory );
		Γ up<std::set<fs::directory_entry>> GetDirectories( path directory, up<std::set<fs::directory_entry>> pItems=nullptr );
		Γ std::string ToString( path pszFilePath );
		Γ std::vector<std::string> LoadColumnNames( path csvFileName );
		Γ std::string DateFileName( uint16 year, uint8 month=0, uint8 day=0 )noexcept;
		Γ tuple<uint16,uint8,uint8> ExtractDate( path path )noexcept;

		Γ void Replace( path source, path destination, const flat_map<string,string>& replacements )ε;

		Γ void CombineFiles( path csvFileName );

		template<typename TCollection>
		void SaveColumnNames( path path, const TCollection& columns );

		struct Γ Compression //TODO See if this is used.
		{
			void Save( path path, const std::vector<char>& data )ε;
			virtual fs::path Compress( path path, bool deleteAfter=true )ε;
			virtual void Extract( path path )ε;
			up<std::vector<char>> LoadBinary( path uncompressed, path compressed=fs::path(), bool setPermissions=false, bool leaveUncompressed=false )ε;
			virtual const char* Extension()noexcept=0;
			virtual bool CompressAutoDeletes(){return true;}
		protected:
			virtual std::string CompressCommand( path path )noexcept=0;
			virtual std::string ExtractCommand( path compressedFile, fs::path destination )noexcept=0;
		};
		struct SevenZ : Compression
		{
			const char* Extension()noexcept final override{return ".7z";}
			std::string CompressCommand( path path )noexcept  override{ return fmt::format( "7z a -y -bsp0 -bso0 {0}.7z  {0}", path.string() ); };
			std::string ExtractCommand( path compressedFile, fs::path destination )noexcept override{ return fmt::format( "7z e {} -o{} -y -bsp0 -bso0", compressedFile.string(), destination.string() ); }
		};

		struct XZ : Compression
		{
			const char* Extension()noexcept final override{return ".xz";}
			std::string CompressCommand( path path )noexcept final override{ return fmt::format( "xz -z -q {}", path.string() ); };
			std::string ExtractCommand( path compressedFile, fs::path destination )noexcept final override{ return fmt::format( "xz -d -q -k {}", compressedFile.string() ); }
		};
		struct Zip : Compression
		{
			const char* Extension()noexcept final override{return ".zip";}
			std::string CompressCommand( path path )noexcept final override{ return fmt::format( "zip -q -m -j {0}.zip {0}", path.string() ); };
#ifdef _MSC_VER
			std::string ExtractCommand( path compressedFile, fs::path destination )noexcept final override{ return fmt::format( "\"C:\\Program Files\\7-Zip\\7z\" e {0} -y -o{1} > NUL: & del {0}", compressedFile.string(), destination.string() ); }
#else
			std::string ExtractCommand( path compressedFile, fs::path destination )noexcept final override{ return fmt::format( "unzip -n -q -d{} {}", destination.string(), compressedFile.string() ); }
#endif
		};
	}
	namespace File
	{
		uint GetFileSize( std::ifstream& file );

		std::pair<std::vector<std::string>,std::set<uint>> LoadColumnNames( sv csvFileName, std::vector<std::string>& columnNamesToFetch, bool notColumns=false );

		template<typename T>
		void ForEachLine( path file, const std::function<void(const std::basic_string<T>&)>& function )noexcept;
		template<typename T>
		void ForEachLine( const std::basic_string<T>& file, const std::function<void(const std::basic_string<T>&)>& function, const uint lineCount );

		void ForEachLine( sv file, const std::function<void(sv)>& function, const uint lineCount );
		uint ForEachLine( sv pszFileName, const std::function<void(const std::vector<std::string>&, uint lineIndex)>& function, const std::set<uint>& columnIndexes, const uint maxLines=std::numeric_limits<uint>::max(), const uint startLine=0, const uint chunkSize=1073741824, uint maxColumnCount=1500 );

		uint ForEachLine2( path fileName, const std::function<void(const std::vector<std::string>&, uint lineIndex)>& function, const std::set<uint>& columnIndexes, const uint lineCount=std::numeric_limits<uint>::max(), const uint startLine=0, const uint chunkSize=1073741824, Stopwatch* sw=nullptr )ε;
		uint ForEachLine3( sv pszFileName, const std::function<void(const std::vector<double>&, uint lineIndex)>& function, const std::set<uint>& columnIndexes, const uint lineCount=std::numeric_limits<uint>::max(), const uint startLine=0, const uint chunkSize=1073741824, uint maxColumnCount=1500, Stopwatch* sw=nullptr );
		uint ForEachLine4( sv pszFileName, const std::function<void(const std::vector<double>&, uint lineIndex)>& function, const std::set<uint>& columnIndexes, const uint lineCount=std::numeric_limits<uint>::max(), const uint startLine=0, const uint chunkSize=1073741824, uint maxColumnCount=1500, Stopwatch* sw=nullptr, double emptyValue=0.0 );

		uint Merge( path file, const vector<char>& original, const vector<char>& newData )ε;
	};

	template<typename TCollection>
	void FileUtilities::SaveColumnNames( path path, const TCollection& columns )
	{
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
		os << endl;
	}


	template<typename T>
	void File::ForEachLine( path file, const std::function<void(const std::basic_string<T>&)>& function )noexcept
	{
		std::basic_ifstream<T> t( file );
		std::basic_string<T> line;
		while( getline(t, line) )
			function( line );
	}
	template<typename T>
	void File::ForEachLine( const std::basic_string<T>& file, const std::function<void(const std::basic_string<T>&)>& function, const uint lineCount )
	{
		CHECK_PATH( file, SRCE_CUR );
		std::basic_ifstream<T> t( file ); THROW_IFX( t.fail(), IOException(file, "Could not open file") );
		std::basic_string<T> line;
		uint lineIndex=0;
		while( std::getline<T>(t, line) )
		{
			function( line );
			if( ++lineIndex>=lineCount )
				break;
		}
	}
}
#undef Φ