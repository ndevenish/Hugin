/**
 * @file test_util.h
 * @brief Prototype only.
 *  Created on: Jul 27, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

namespace makefile { namespace tester {
int exec_make(const char* const argv[], std::stringbuf& makeoutbuf, std::stringbuf& makeerrbuf);

/**
 * Base class for tests;
 */
class Test
{
	/// store makes output
	std::stringbuf makeoutbuf, makeerrbuf;
	/// Test's name
	const char* name;
	/// Output on stdout if test passes.
	const char* goodout;
	bool result;
public:
	/// Prepare the MakefileItems
	Test(const char* name_, const char* goodout_)
	:name(name_), goodout(goodout_), result(false) {}
	/// Eventual cleanup
	virtual ~Test() {}
	/// Execute the test
	virtual bool run();
	/// Precondition, additional to comparing stdout, overload if necessary.
	virtual bool precond()
	{
		return true;
	}
	/// Checks the result, overload if necessary.
	virtual bool eval()
	{
		result = precond() && (goodout == makeoutbuf.str());
		return result;
	}

};
}}
#endif /* TEST_UTIL_H_ */
