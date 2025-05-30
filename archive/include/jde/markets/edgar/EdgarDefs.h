#pragma once
#include <unordered_set>
#include <boost/container/flat_set.hpp>
#include <jde/framework/str.h>

namespace Jde::Markets
{
	using std::unordered_set;
	using boost::container::flat_set;
	constexpr uint8 CompressionAmountIndex{6};
	using Cik=uint32;
	using ClassPK=uint32;
	using FormPK=uint16;
	using FilingPK=uint32;
	using  Cusip=CIString;
}