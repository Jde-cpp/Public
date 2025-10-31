#pragma once

#ifndef ASSERT
# define ASSERT( actual ){ if( !(actual) ){ CRITICALT( ELogTags::App, "Assert:  {} is false",  #actual ); } }
#endif

#define ASSERTSL( actual, xsl ){ if( !(actual) )LOGSL(ELogLevel::Critical, xsl, ELogTags::App, "Assert:  {} is false", #actual); }

#ifndef ASSERTX
# define	ASSERTX( actual ){ if( !(actual) ){ CRITICALT(ELogTags::App | ELogTags::ExternalLogger, "Assert:  {} is false", #actual); } }
#endif
#ifndef ASSERT_DESC
	#define ASSERT_DESC( actual, desc ) {if( !(actual) ){ CRITICALT(ELogTags::App, "Assert:  {} - {} is false", desc, #actual); }}
#endif