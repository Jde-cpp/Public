#pragma once

namespace Jde::DB{
	struct IDataSource; struct Schema;
namespace CatalogDdl{
	α Create( IDataSource& ds, sv catalog )ε->void;

#ifndef PROD
namespace NonProd{
	α Drop( const Schema& schema )ε->void;
}
#endif
}}