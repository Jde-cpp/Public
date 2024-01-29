#pragma once
#include "App.h"
#ifndef ASSERT
# define ASSERT( actual ){ if( !(actual) ){ CRITICALT( AppTag(), "Assert:  {} is false"sv,  #actual ); } /*assert( actual );*/ }
#endif

#define ASSERTSL( actual, xsl ){ if( !(actual) )Log( ELogLevel::Critical, format("Assert:  {} is false"sv,  #actual), xsl ); }

#ifndef ASSERTX
# define	ASSERTX( actual ){ if( !(actual) ){ Logging::LogNoServer( MessageBase{ELogLevel::Critical, "Assert:  {} is false", MY_FILE, __func__, __LINE__}, _logTag,  #actual ); } assert( actual ); }
#endif
#ifndef VERIFY
# define VERIFY( actual ) if( !(actual) ){WARN( "VERIFY_TR - Expected {} to be false"sv, #actual ); }
#endif

#ifndef ASSERT_DESC
	#define ASSERT_DESC( actual, desc ) {if( !(actual) ){ CRITICAL("Assert:  {} - {} is false"sv, desc, #actual ); } assert( actual );}
#endif
