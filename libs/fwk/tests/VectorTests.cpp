#include <jde/fwk/utils/Vector.h>

#define let const auto

namespace Jde::Tests{
	TEST( VectorTests, RerAseCallbackReentrant ){
		Vector<int> v;
		v.push_back( 1 ); v.push_back( 2 ); v.push_back( 3 );
		vector<int> order;
		v.rerase( [&](const int& x){ order.push_back(x); (void)v.empty(); } );//empty() takes a shared lock - would deadlock if rerase still held the exclusive lock.
		EXPECT_EQ( order, (vector<int>{3, 2, 1}) );//reverse order.
		EXPECT_TRUE( v.empty() );
	}
	TEST( VectorTests, EraseCallbackReentrant ){
		Vector<int> v;
		v.push_back( 1 ); v.push_back( 2 ); v.push_back( 3 );
		vector<int> order;
		v.erase( [&](const int& x){ order.push_back(x); (void)v.size(); } );
		EXPECT_EQ( order, (vector<int>{1, 2, 3}) );//forward order.
		EXPECT_TRUE( v.empty() );
	}
}
