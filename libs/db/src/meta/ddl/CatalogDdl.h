#pragma once

namespace Jde::DB{
	struct IDataSource; struct AppSchema;
namespace CatalogDdl{
	α Create( IDataSource& ds, sv catalog )ε->void;

#ifndef PROD
namespace NonProd{
	α Drop( const AppSchema& schema )ε->void;
}
#endif
}}