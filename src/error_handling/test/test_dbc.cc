/*
  Copyright 2010-201x held jointly by participating institutions.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:

*/

//!

#include <UnitTest++.h>

#include "../dbc.hh"

#ifdef ENABLE_DBC

SUITE(DBC)
{
  TEST(Assert) { CHECK_THROW(AMANZI_ASSERT(1 == 2), DBC::Assertion); }
}

#endif
