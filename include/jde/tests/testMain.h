#pragma once
#ifndef JDE_TEST_MAIN_H
#define JDE_TEST_MAIN_H

#include <cstdlib>
#include "jde/fwk/log/log.h"
#include "jde/fwk/log/logTags.h"
#define let const auto

namespace Jde{
	//A gtest filter that matches nothing is not an error to gtest - RUN_ALL_TESTS() runs no test and returns 0,
	//so a suite that was renamed out from under its '/testing/tests' setting (or never written) reports green
	//having tested nothing.  Wrap the RUN_ALL_TESTS() call:  result = CheckTestsRan( RUN_ALL_TESTS() );
	//Only meaningful afterwards - test_to_run_count() is populated when gtest applies the filter, inside Run().
	Ξ CheckTestsRan( int result )ι->int{
		let& unit = *::testing::UnitTest::GetInstance();
		if( unit.test_to_run_count() )
			return result;
		CRITICALT( ELogTags::Test, "gtest filter '{}' matched none of the {} tests - nothing ran.", ::testing::GTEST_FLAG(filter), unit.total_test_count() );
		return EXIT_FAILURE;
	}
}
#undef let
#endif
