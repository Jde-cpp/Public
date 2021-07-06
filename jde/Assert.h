#pragma once

#ifndef ASSERT
# define ASSERT( actual ){ if( !(actual) ){ CRITICAL("Assert:  {} is false"sv,  #actual ); } assert( actual ); }
#endif
#ifndef VERIFY
# define VERIFY( actual ) if( !(actual) ){WARN( "VERIFY_TR - Expected {} to be false"sv, #actual ); }
#endif

#ifndef ASSERT_DESC
	#define ASSERT_DESC( actual, desc ) {if( !(actual) ){ CRITICAL("Assert:  {} - {} is false"sv, desc, #actual ); } assert( actual );}
#endif
