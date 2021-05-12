#pragma once
//#include "Exception.h"
//TODO Cleanup

#ifndef ASSERT
# define ASSERT( actual ){ if( !(actual) ){ CRITICAL("Assert:  {} is false"sv,  #actual ); } assert( actual ); }
#endif
#ifndef VERIFY
# define VERIFY( actual ) if( !(actual) ){WARN( "VERIFY_TR - Expected {} to be false"sv, #actual ); }
#endif

#ifndef ASSERT_DESC
	#define ASSERT_DESC( actual, desc ) {if( !(actual) ){ CRITICAL("Assert:  {} - {} is false"sv, desc, #actual ); } assert( actual );}
#endif
/*
#ifndef ASSRT_LT
	#define ASSRT_LT( expected, actual ) if( !(actual<expected) )CRITICAL("Assert: (expected:  {}) {} < {} (actual:  {})"sv, expected, #expected, #actual, actual ); assert( actual<expected );
#endif
#ifndef ASSRT_EQ
	# define ASSRT_EQ( expected, actual ) if( expected!=actual ){CRITICAL("Assert:  (expected:  {}) {}=={} (actual:  {})"sv, expected, #expected, #actual, actual ); } assert( expected==actual );
#endif
#ifndef ASSRT_NN
# define ASSRT_NN( actual ) if( !actual )CRITICAL("Expected {} to not be null."sv, #actual ); assert( actual );
#endif
#ifndef ASSRT_NN_DESC
# define ASSRT_NN_DESC( actual, desc ) if( !actual )CRITICAL("Expected {} to not be null - {}"sv, #actual, desc ); assert( actual );
#endif
#ifndef ASSERT_NULL
# define ASSERT_NULL( p ) if( p )CRITICAL("Expected '{}' to be null."sv, #p ); assert( !p );
#endif
//#ifndef ASSRT_BETWEEN
//# define ASSRT_BETWEEN( expected_low, expected_high, actual ) if( !(expected_low<=actual && expected_high>actual) )CRITICAL("Expected ({}) {} to be between ({}){} and {}({}) null.", #actual, actual, #expected_low, expected_low, expected_high, #expected_high ); assert( expected_low<=actual && expected_high>actual );
//#endif

*/