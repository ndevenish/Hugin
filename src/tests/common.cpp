// -*- c-basic-offset: 4 -*-

/** @file common.cpp
 *
 *  @brief implementation of common Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <common/utils.h>
#include <common/platform.h>

using namespace boost::unit_test_framework;
using namespace std;
using namespace utils;

void test_quote()
{
    string a("normal,string");
    string a_(quoteString(a));
    BOOST_CHECK_EQUAL(a, a_);

    string b("test  space");
    string b_(quoteString(b));
    string bq("test\\ \\ space");
    BOOST_CHECK_EQUAL(b_, bq);

    string c("test\\space");
    string c_(quoteString(c));
    string cq("test\\\\space");
    BOOST_CHECK_EQUAL(c_, cq);

    string d(" test\\  space ");
    string d_(quoteString(d));
    string dq("\\ test\\\\\\ \\ space\\ ");
    BOOST_CHECK_EQUAL(d_, dq);

}

test_suite *
init_unit_test_suite( int, char** )
{
  test_suite* test= BOOST_TEST_SUITE( "common functions" );
  test->add(BOOST_TEST_CASE(&test_quote));

//  test->add(BOOST_TEST_CASE(&transforms_test));
  return test;
}

