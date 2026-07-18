#include <jde/fwk/utils/paramPack.h>

#define let const auto

namespace Jde::Tests{
	TEST( ParamPackTests, RvalueString ){
		vector<string> values;
		ParamPack::Append( values, string{"first"}, 2, string{"third"} );
		ASSERT_EQ( values, (vector<string>{"first", "2", "third"}) );
	}
}
