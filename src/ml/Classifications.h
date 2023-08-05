#include "../math/Arma.h"
//#include "/home/duffyj/code/jde/Framework/source/math/MathUtilities.h"
//#include <mlpack/methods/kmeans/kmeans.hpp>

#define var const auto
namespace Jde
{
	using Point=Math::Point<double>;
	using namespace arma;
	//(b - a)/max(a, b)
	//a=mean distance to the other instances in the same cluster.
	//b=is the _mean distance to the other instances in the nearest-cluster.  cluster that mimizes b.
	template<class T=double> α SilhouetteScore( const arma::Mat<T>& x, const arma::Col<uint>& assignments, const mat& centroids )->double
	{
		var assignmentSize = assignments.n_rows;
		CHECK( x.n_cols==assignmentSize );
		var centroidSize = centroids.n_rows;
		vector<uint> counts( centroidSize );
		for( var a : assignments )
		{
			CHECK( a<centroidSize );
			++counts[a];
		}
		double sum{};
		for( uint i=0; i<assignmentSize; ++i )
		{
			var a = assignments[i];
			vector<double> sums( centroidSize ); //[centriod][count in centroid]
			const Point pt{ x(0,i), x(1,i) };
			for( uint i2=0; i2<assignmentSize; ++i2 )
			{
				var a2 = assignments[i2];
				var d = pt.Distance( Point{x(0,i2), x(1,i2)} );
				sums[a2]+=d;
			}
			vector<double> avg( centroidSize );
			for( uint c=0; c<centroidSize; ++c )
				avg[c] = c==a ? limits<double>::max() : sums[c]/counts[c];

			var aScore = sums[a]/(counts[a]-1);
			var bScore = *std::min_element( avg.begin(), avg.end() );

			double iScore = (bScore-aScore)/std::max(aScore,bScore);
			sum += iScore;
		}

		return sum/assignmentSize;
	}
}