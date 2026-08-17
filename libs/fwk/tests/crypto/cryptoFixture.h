#pragma once
#include <jde/fwk/settings.h>

namespace Jde::Crypto::Tests{
	//Both crypto fixtures used to mint into fixed absolute paths - /tmp/public.pem, or ProgramData/Jde-Cpp/<product>
	//on windows - so every build and both test workflows wrote the same files, and with cryptoTests.clear:false a
	//leftover from one run fed the next (#26).  This is the dir FileTests already writes under, which is cwd-relative
	//and therefore per-workflow ($buildDir/Testing for ctest, $buildDir/runtime for a direct run), plus a per-suite
	//leaf so the two fixtures cannot collide with each other either.  Deliberately not process-scoped: clear:false
	//means to reuse the key pair across runs, and a pid in the path would silently defeat that.
	Ξ FixtureDir( sv suite )->fs::path{
		const auto configured = Settings::FindPath( "/testing/file" );//set by every test config; the fallback is for a settings-less run.
		const auto dir = ( configured ? configured->parent_path() : fs::temp_directory_path() )/"crypto"/suite;
		if( !fs::exists(dir) )
			fs::create_directories( dir );
		return dir;
	}
}
