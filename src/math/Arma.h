#pragma once
#include <armadillo>

#define var const auto

namespace Jde::Arma
{
	using namespace arma;
	template<class T=double> α From( const Eigen::Matrix<T,-1,-1> m )->Mat<T>
	{
		var rows = (uword)m.rows();
		var cols = (uword)m.cols();
		Mat<T> data{ rows, cols, fill::none };
		for( uint i=0; i<rows; ++i )
		{
			for( uint j=0; j<cols; ++j )
			{
				T v = m( i, j );
				if constexpr( std::is_same<T,double>() )
					CHECK( std::isfinite(v) );
				data( i, j ) = v;
			}
		}
		return data;
	}

	template<class T=double> α FromCsv( path path, bool transpose=false )->Mat<T>
	{
		auto m=EMatrix::LoadCsv<T>( path, nullptr, true, 0 );
		return From( transpose ? m->transpose() : *m );
	}
}
#undef var