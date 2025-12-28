#pragma once

namespace Jde::DB{
	struct IDataSource; struct AppSchema;
namespace CatalogDdl{
#ifndef PROD
namespace NonProd{
	α Drop( const AppSchema& schema )ε->void;
}
#endif
}}