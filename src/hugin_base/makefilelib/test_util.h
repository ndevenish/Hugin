/**
 * @file test_util.h
 * @brief
 *  Created on: Jul 27, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

int exec_make(const char* const argv[], std::stringbuf& makeoutbuf, std::stringbuf& makeerrbuf);

#endif /* TEST_UTIL_H_ */
