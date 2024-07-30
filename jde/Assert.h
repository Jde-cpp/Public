#pragma once
//#include "App.h"
#ifndef ASSERT
# define ASSERT( actual ){ if( !(actual) ){ Critical{ ELogTags::App, "Assert:  {} is false"sv,  #actual }; } /*assert( actual );*/ }
#endif

#define ASSERTSL( actual, xsl ){ if( !(actual) )Critical{xsl, ELogTags::App, "Assert:  {} is false", #actual}; }

#ifndef ASSERTX
# define	ASSERTX( actual ){ if( !(actual) ){ Critical{ELogTags::App | ELogTags::ExternalLogger, "Assert:  {} is false", #actual }; } }
#endif
#ifndef ASSERT_DESC
	#define ASSERT_DESC( actual, desc ) {if( !(actual) ){ Critical{ELogTags::App, "Assert:  {} - {} is false", desc, #actual }; }}
#endif
