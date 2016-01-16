// -*- c-basic-offset: 4 -*-

/** @file ParseExp.cpp
 *
 *  @brief functions to parse expressions from strings
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ParseExp.h"

#ifdef HAVE_CXX11
// parser using shunting yard algorithm using C++11 features

#include <exception>
#include <stack>
#include <queue>
#include <functional>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cstdlib>

namespace Parser
{
/** internal exception class for all errors */
class ParseException : public std::runtime_error
{
public:
    explicit ParseException(const char *message) : std::runtime_error(message) {};
};

/** classes for parsed tokens in rpn queue */
namespace RPNTokens
{
/** abstract base class */
class TokenBase
{
public:
    virtual void evaluate(std::stack<double>&) = 0;
};

/** single numeric token on rpn queue */
class NumericToken :public TokenBase
{
public:
    explicit NumericToken(double val) : m_value(val) {};
    void evaluate(std::stack<double>& rpnStack) { rpnStack.push(m_value); };
private:
    double m_value;
};

/** unary operator or function on rpn queue */
class FunctionToken : public TokenBase
{
public:
    explicit FunctionToken(std::function<double(double)> func) : TokenBase(), m_function(func) {};
    void evaluate(std::stack<double>& rpnStack)
    {
        if (rpnStack.empty())
        {
            throw ParseException("Unary operator expects one item on stack.");
        }
        const double val = rpnStack.top();
        rpnStack.pop();
        const double newVal = m_function(val);
        if (!isinf(newVal) && !isnan(newVal))
        {
            rpnStack.push(newVal);
        }
        else
        {
            throw ParseException("Invalid operation");
        };
    };
private:
    std::function<double(double)> m_function;
};

/** binary operator on rpn stack */
class BinaryToken : public TokenBase
{
public:
    explicit BinaryToken(std::function<double(double, double)> func) : TokenBase(), m_function(func) {};
    void evaluate(std::stack<double>& rpnStack)
    {
        if (rpnStack.size() < 2)
        {
            throw ParseException("BinaryOperator expects 2 items on stack.");
        };
        const double right = rpnStack.top();
        rpnStack.pop();
        const double left = rpnStack.top();
        rpnStack.pop();
        const double newVal = m_function(left, right);
        if (!isinf(newVal) && !isnan(newVal))
        {
            rpnStack.push(newVal);
        }
        else
        {
            throw ParseException("Invalid operation");
        };
    };
private:
    std::function<double(double, double)> m_function;
};

/** if operator on rpn stack */
class IfToken : public TokenBase
{
public:
    void evaluate(std::stack<double>& rpnStack)
    {
        if (rpnStack.size() < 3)
        {
            throw ParseException("IfOperator expects 3 items on stack.");
        };
        const double elseVal = rpnStack.top();
        rpnStack.pop();
        const double ifVal = rpnStack.top();
        rpnStack.pop();
        const double compareVal = rpnStack.top();
        rpnStack.pop();
        if (fabs(compareVal) > 1e-8)
        {
            rpnStack.push(ifVal);
        }
        else
        {
            rpnStack.push(elseVal);
        };
    };
};
} // namespace RPNTokens

  /** classes for operators on shunting yards operator stack */
namespace Operators
{
/** base class for operator on shunting yards operator stack */
class OperatorBase
{
public:
    OperatorBase(int prec, bool rightAssoc = false) : m_precedence(prec), m_rightAssoc(rightAssoc) {};
    const int GetPrecedence() const { return m_precedence; };
    const bool IsRightAssociative() const { return m_rightAssoc; };
    bool ComparePrecedence(const OperatorBase* other)
    {
        if (IsRightAssociative())
        {
            return GetPrecedence() < other->GetPrecedence();
        }
        else
        {
            return GetPrecedence() <= other->GetPrecedence();
        };
    };
    virtual RPNTokens::TokenBase* GetTokenBase() { return nullptr; };
private:
    int m_precedence;
    bool m_rightAssoc;
};

/** function or unary operator on shunting yards operator stack */
class FunctionOperator : public OperatorBase
{
public:
    FunctionOperator(std::function<double(double)> func, int prec = -2, bool rightAssoc = false) : OperatorBase(prec, rightAssoc), m_function(func) {};
    RPNTokens::TokenBase* GetTokenBase() { return new RPNTokens::FunctionToken(m_function); };
private:
    std::function<double(double)> m_function;
};

/** binary operator on stack on shunting yards operator stack */
class BinaryOperator : public OperatorBase
{
public:
    BinaryOperator(std::function<double(double, double)> func, int prec, bool rightAssoc = false) : OperatorBase(prec, rightAssoc), m_function(func) {};
    RPNTokens::TokenBase* GetTokenBase() { return new RPNTokens::BinaryToken(m_function); };
private:
    std::function<double(double, double)> m_function;
};

/** if operator on operator stack, used only for ':' token */
class IfOperator : public OperatorBase
{
public:
    IfOperator() : OperatorBase(0, true) {};
    RPNTokens::TokenBase* GetTokenBase() { return new RPNTokens::IfToken(); }
};
}; // namespace Operators

   /** clear the queue */
void ClearQueue(std::queue<RPNTokens::TokenBase*>& input)
{
    while (!input.empty())
    {
        delete input.front();
        input.pop();
    }
}

/** remove all whitespaces and convert to lowercase */
std::string RemoveWhiteSpaces(const std::string& text)
{
    std::string output;
    output.reserve(text.size());
    for (auto c : text)
    {
        if (!isspace(c))
        {
            output.push_back(tolower(c));
        };
    };
    return output;
};

static std::map<std::string, Operators::OperatorBase*> supportedBinaryOperations;
static Operators::OperatorBase* parenthesesOperator = nullptr;
static Operators::OperatorBase* ifOperator = nullptr;
static Operators::OperatorBase* ifOperatorClose = nullptr;
static std::map<std::string, Operators::FunctionOperator*> supportedFunctions;

/** initialize some internal variables */
void InitParser()
{
    if (supportedBinaryOperations.empty())
    {
        supportedBinaryOperations["||"] = new Operators::BinaryOperator(std::logical_or<double>(), 2);
        supportedBinaryOperations["&&"] = new Operators::BinaryOperator(std::logical_and<double>(), 3);
        supportedBinaryOperations["=="] = new Operators::BinaryOperator(std::equal_to<double>(), 4);
        supportedBinaryOperations["!="] = new Operators::BinaryOperator(std::not_equal_to<double>(), 4);
        supportedBinaryOperations["<"] = new Operators::BinaryOperator(std::less<double>(), 5);
        supportedBinaryOperations["<="] = new Operators::BinaryOperator(std::less_equal<double>(), 5);
        supportedBinaryOperations[">"] = new Operators::BinaryOperator(std::greater<double>(), 5);
        supportedBinaryOperations[">="] = new Operators::BinaryOperator(std::greater_equal<double>(), 5);
        supportedBinaryOperations["+"] = new Operators::BinaryOperator(std::plus<double>(), 6);
        supportedBinaryOperations["-"] = new Operators::BinaryOperator(std::minus<double>(), 6);
        supportedBinaryOperations["*"] = new Operators::BinaryOperator(std::multiplies<double>(), 7);
        supportedBinaryOperations["/"] = new Operators::BinaryOperator(std::divides<double>(), 7);
        supportedBinaryOperations["%"] = new Operators::BinaryOperator((double(*)(double, double))fmod, 7);
        supportedBinaryOperations["^"] = new Operators::BinaryOperator((double(*)(double, double))pow, 8, true);
        supportedBinaryOperations["UNARY_MINUS"] = new Operators::FunctionOperator([](double val)->double {return -1 * val; }, 9, true);
    }
    if (!parenthesesOperator)
    {
        parenthesesOperator = new Operators::OperatorBase(-1);
    };
    if (!ifOperator)
    {
        ifOperator = new Operators::OperatorBase(1);
    }
    if (!ifOperatorClose)
    {
        ifOperatorClose = new Operators::IfOperator();
    }
    if (supportedFunctions.empty())
    {
        supportedFunctions["abs"] = new Operators::FunctionOperator((double(*)(double))abs);
        supportedFunctions["sin"] = new Operators::FunctionOperator((double(*)(double))sin);
        supportedFunctions["cos"] = new Operators::FunctionOperator((double(*)(double))cos);
        supportedFunctions["tan"] = new Operators::FunctionOperator((double(*)(double))tan);
        supportedFunctions["asin"] = new Operators::FunctionOperator((double(*)(double))asin);
        supportedFunctions["acos"] = new Operators::FunctionOperator((double(*)(double))acos);
        supportedFunctions["atan"] = new Operators::FunctionOperator((double(*)(double))atan);
        supportedFunctions["exp"] = new Operators::FunctionOperator((double(*)(double))exp);
        supportedFunctions["log"] = new Operators::FunctionOperator((double(*)(double))log);
        supportedFunctions["ceil"] = new Operators::FunctionOperator((double(*)(double))ceil);
        supportedFunctions["floor"] = new Operators::FunctionOperator((double(*)(double))floor);
        supportedFunctions["sqrt"] = new Operators::FunctionOperator((double(*)(double))sqrt);
        supportedFunctions["deg"] = new Operators::FunctionOperator([](double val)->double { return val * 180.0f / M_PI; });
        supportedFunctions["rad"] = new Operators::FunctionOperator([](double val)->double { return val * M_PI / 180.0f; });
    }
};

/** clean up some internal static variables */
void CleanUpParser()
{
    for (auto it : supportedBinaryOperations)
    {
        delete it.second;
    };
    supportedBinaryOperations.clear();
    for (auto it : supportedFunctions)
    {
        delete it.second;
    };
    supportedFunctions.clear();
    if (parenthesesOperator)
    {
        delete parenthesesOperator;
        parenthesesOperator = nullptr;
    };
    if (ifOperator)
    {
        delete ifOperator;
        ifOperator = nullptr;
    };
    if (ifOperatorClose)
    {
        delete ifOperatorClose;
        ifOperatorClose = nullptr;
    };
};

/** compare the first characters in string with supportedBinaryOperations, return longest match */
std::string FindOperator(const std::string& searchString)
{
    std::string foundOperator;
    for (auto it : supportedBinaryOperations)
    {
        const std::string op(it.first);
        if (searchString.compare(0, op.length(), op) == 0)
        {
            if (op.length() > foundOperator.length())
            {
                foundOperator = op;
            }
        };
    };
    return foundOperator;
};

/** convert expression to RPN *
*  @return true, if expression could be converted to Reverse Polish notation (RPN), otherwise false */
bool ConvertToRPN(const std::string& expression, const ConstantMap& constants, std::queue<RPNTokens::TokenBase*>& rpn)
{
    // initialize some internal variables, don't forget to call CleanUpParser() at the end
    InitParser();
    size_t pos = 0;
    std::stack<Operators::OperatorBase*> operatorStack;
    bool lastTokenWasOperator = true;
    const size_t expressionLength = expression.length();
    while (pos < expressionLength)
    {
        const char currentChar = expression[pos];
        if (lastTokenWasOperator && (currentChar == '+' || currentChar == '-'))
        {
            // special handling of +/- prefix
            if (currentChar == '-')
            {
                operatorStack.push(supportedBinaryOperations["UNARY_MINUS"]);
            };
            pos++;
            lastTokenWasOperator = false;
            continue;
        }
        if (isdigit(currentChar) || currentChar == '.')
        {
            // parse numbers
            size_t index;
            double val;
            try
            {
                val = std::stod(expression.substr(pos), &index);
            }
            catch (std::invalid_argument)
            {
                throw ParseException("Invalid number");
            }
            catch (std::out_of_range)
            {
                throw ParseException("Out of range");
            }
            index += pos;
            rpn.push(new RPNTokens::NumericToken(val));
            pos = index;
            lastTokenWasOperator = false;
            continue;
        }
        if (isalpha(currentChar))
        {
            // parse variable or function names
            const size_t found = expression.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_", pos);
            std::string subString;
            if (found != std::string::npos)
            {
                subString = expression.substr(pos, found - pos);
                // if next character is '(' we found a function name, otherwise we treat it as variable name
                // all white space have been filtered out before
                if (expression[found] == '(')
                {
                    const auto foundFunction = supportedFunctions.find(subString);
                    if (foundFunction == supportedFunctions.end())
                    {
                        throw ParseException("Unknown function");
                    };
                    operatorStack.push(foundFunction->second);
                    operatorStack.push(parenthesesOperator);
                    pos = found + 1;
                    lastTokenWasOperator = true;
                    continue;
                };
            }
            else
            {
                // no further character in string, it can only be a variable name
                subString = expression.substr(pos);
            };
            const auto foundVar = constants.find(subString);
            if (foundVar == constants.end())
            {
                throw ParseException("Unknown variable");
            };
            rpn.push(new RPNTokens::NumericToken(foundVar->second));
            pos = found;
            lastTokenWasOperator = false;
            continue;
        };
        if (currentChar == '(')
        {
            // parse left parenthesis
            operatorStack.push(parenthesesOperator);
            pos++;
            lastTokenWasOperator = true;
            continue;
        };
        if (currentChar == ')')
        {
            // closing parenthesis
            bool matchingParenthesis = false;
            while (!operatorStack.empty())
            {
                const int topPrecedenece = operatorStack.top()->GetPrecedence();
                if (topPrecedenece == -2)
                {
                    throw ParseException("Function without matching parenthesis");
                }
                if (topPrecedenece == -1)
                {
                    // we found a matching parenthesis
                    matchingParenthesis = true;
                    operatorStack.pop();
                    break;
                }
                rpn.push(operatorStack.top()->GetTokenBase());
                operatorStack.pop();
            }
            if (!matchingParenthesis)
            {
                throw ParseException("Mismatched parentheses");
            };
            if (!operatorStack.empty() && operatorStack.top()->GetPrecedence() == -2)
            {
                // we found a function on the operator stack
                rpn.push(operatorStack.top()->GetTokenBase());
                operatorStack.pop();
            };
            pos++;
            lastTokenWasOperator = false;
            continue;
        };
        if (currentChar == '?')
        {
            // we found an start of an ?: expression
            while (!operatorStack.empty() && operatorStack.top()->GetPrecedence() > 1)
            {
                rpn.push(operatorStack.top()->GetTokenBase());
                operatorStack.pop();
            };
            operatorStack.push(ifOperator);
            pos++;
            lastTokenWasOperator = true;
            continue;
        }
        if (currentChar == ':')
        {
            bool matchingIf = false;
            while (!operatorStack.empty())
            {
                if (operatorStack.top()->GetPrecedence() == 1)
                {
                    matchingIf = true;
                    operatorStack.pop();
                    break;
                };
                rpn.push(operatorStack.top()->GetTokenBase());
                operatorStack.pop();
            }
            if (!matchingIf)
            {
                throw ParseException("Mismatched ternary operator ?:");
            };
            operatorStack.push(ifOperatorClose);
            pos++;
            lastTokenWasOperator = true;
            continue;
        };
        // and finally the operators
        const std::string foundOperatorString = FindOperator(expression.substr(pos));
        if (foundOperatorString.empty())
        {
            throw ParseException("Invalid operator or unknown character");
        }
        Operators::OperatorBase* foundOperator = supportedBinaryOperations[foundOperatorString];
        while (!operatorStack.empty() && foundOperator->ComparePrecedence(operatorStack.top()))
        {
            rpn.push(operatorStack.top()->GetTokenBase());
            operatorStack.pop();
        };
        operatorStack.push(foundOperator);
        pos += foundOperatorString.length();
        lastTokenWasOperator = true;
    };
    while (!operatorStack.empty())
    {
        switch (operatorStack.top()->GetPrecedence())
        {
            case -2:
                throw ParseException("Function with unbalanced parenthenses");
                break;
            case -1:
                throw ParseException("Unbalanced left and right parenthenses");
                break;
            case 1:
                throw ParseException("If without else");
                break;
        }
        rpn.push(operatorStack.top()->GetTokenBase());
        operatorStack.pop();
    };
    return true;
};

/** evaluate RPN in input
*  @return true, if input could be evaluated, result has the final value; otherwise false and result is unmodified */
bool EvaluateRPN(std::queue<RPNTokens::TokenBase*>& input, double& result)
{
    std::stack<double> stack;
    try
    {
        while (!input.empty())
        {
            RPNTokens::TokenBase* token = input.front();
            token->evaluate(stack);
            delete token;
            input.pop();
        }
    }
    catch (ParseException)
    {
        ClearQueue(input);
        return false;
    };
    ClearQueue(input);
    if (stack.size() == 1)
    {
        result = stack.top();
        return true;
    }
    return false;
};

/** parse complete expression in 2 steps */
bool ParseExpression(const std::string& expression, double& result, const ConstantMap& constants)
{
    std::queue<RPNTokens::TokenBase*> rpn;
    try
    {
        // remove all white spaces
        const std::string inputExpression = RemoveWhiteSpaces(expression);
        if (inputExpression.empty())
        {
            // if expression is now empty, return false
            return false;
        };
        ConstantMap inputConstants(constants);
        // add pi
        inputConstants["pi"] = M_PI;
        // convert expression to reverse polish notation rpn
        if (ConvertToRPN(inputExpression, inputConstants, rpn))
        {
            // if successful, evaluate queue
            return EvaluateRPN(rpn, result);
        }
        else
        {
            // could not convert to RPN
            ClearQueue(rpn);
            return false;
        };
    }
    catch (ParseException)
    {
        //something went wrong, delete queue and return false
        ClearQueue(rpn);
        return false;
    };
};

} //namespace Parser

#else

// implementation using boost::spirit is based on blog at
// http://agentzlerich.blogspot.de/2011/06/using-boost-spirit-21-to-evaluate.html
// modified to Hugins need
// added if statement

#include <limits>
#include <iterator>

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#include <boost/spirit/version.hpp>
#if !defined(SPIRIT_VERSION) || SPIRIT_VERSION < 0x2010
#error "At least Spirit version 2.1 required"
#endif
#include <boost/math/constants/constants.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>

namespace Parser
{

// helper classes to implement operators

//power function
struct lazy_pow_
{
    typedef double result_type;

    double operator()(double x, double y) const
    {
        return std::pow(x, y);
    }
};

// modulus for double values
struct lazy_mod_
{
    typedef double result_type;

    double operator()(double x, double y) const
    {
        return std::fmod(x,y);
    }
};

// if statement
struct lazy_if_
{
    typedef double result_type;

    double operator()(double x, double y, double z) const
    {
        return (std::fabs(x)>1e-5) ? y : z;
    }
};

// wrapper for unary function
struct lazy_ufunc_
{
    typedef double result_type;

    double operator()(double (*f)(double), double a1) const
    {
        return f(a1);
    }
};

// convert rad into deg
const double deg(const double d)
{
    return d*180.0/boost::math::constants::pi<double>();
};

// convert deg into rad
const double rad(const double d)
{
    return d*boost::math::constants::pi<double>()/180;
};

// the main grammar class
struct grammar:boost::spirit::qi::grammar<std::string::const_iterator, double(), boost::spirit::ascii::space_type>
{

    // symbol table for constants like "pi", e.g. image number and value
    struct constant_ : boost::spirit::qi::symbols<char, double>
    {
        constant_(const ConstantMap constMap)
        {
            this->add("pi", boost::math::constants::pi<double>());
            if (constMap.size()>0)
            {
                for (ConstantMap::const_iterator it = constMap.begin(); it != constMap.end(); it++)
                {
                    this->add(it->first, it->second);
                };
            };
        };
    };

    // symbol table for unary functions like "abs"
    struct ufunc_ : boost::spirit::qi::symbols<char, double(*)(double) >
    {
        ufunc_()
        {
            this->add
                ("abs", (double(*)(double)) std::abs)
                ("acos", (double(*)(double)) std::acos)
                ("asin", (double(*)(double)) std::asin)
                ("atan", (double(*)(double)) std::atan)
                ("ceil", (double(*)(double)) std::ceil)
                ("sin", (double(*)(double)) std::sin)
                ("cos", (double(*)(double)) std::cos)
                ("tan", (double(*)(double)) std::tan)
                ("exp", (double(*)(double)) std::exp)
                ("floor", (double(*)(double)) std::floor)
                ("sqrt", (double(*)(double)) std::sqrt)
                ("deg", (double(*)(double)) deg)
                ("rad", (double(*)(double)) rad)
                ;
        }
    } ufunc;

    boost::spirit::qi::rule<std::string::const_iterator, double(), boost::spirit::ascii::space_type> expression, term, factor, primary, compExpression, compTerm, numExpression;

    grammar(const ConstantMap constMap) : grammar::base_type(expression)
    {
        using boost::spirit::qi::real_parser;
        using boost::spirit::qi::real_policies;
        real_parser<double, real_policies<double> > real;

        using boost::spirit::qi::_1;
        using boost::spirit::qi::_2;
        using boost::spirit::qi::_3;
        using boost::spirit::qi::no_case;
        using boost::spirit::qi::_val;
        struct constant_ constant(constMap);

        boost::phoenix::function<lazy_pow_>   lazy_pow;
        boost::phoenix::function<lazy_mod_>   lazy_mod;
        boost::phoenix::function<lazy_if_>    lazy_if;
        boost::phoenix::function<lazy_ufunc_> lazy_ufunc;

        expression =
            (compExpression >> '\?' >> compExpression >> ':' >> compExpression)[_val = lazy_if(_1, _2, _3)]
            | compExpression[_val = _1]
            ;

        compExpression =
            compTerm[_val = _1]
            >> *(("&&" >> compTerm[_val = _val && _1])
            | ("||" >> compTerm[_val = _val || _1])
            )
            ;

        compTerm =
            numExpression[_val = _1]
            >> *(('<' >> numExpression[_val = _val <  _1])
            | ('>' >> numExpression[_val = _val >  _1])
            | ("<=" >> numExpression[_val = _val <= _1])
            | (">=" >> numExpression[_val = _val >= _1])
            | ("==" >> numExpression[_val = _val == _1])
            | ("!=" >> numExpression[_val = _val != _1])
            )
            ;

        numExpression =
            term[_val = _1]
            >> *(('+' >> term[_val += _1])
            | ('-' >> term[_val -= _1])
            )
            ;

        term =
            factor[_val = _1]
            >> *(('*' >> factor[_val *= _1])
            | ('/' >> factor[_val /= _1])
            | ('%' >> factor[_val = lazy_mod(_val, _1)])
            )
            ;

        factor =
            primary[_val = _1]
            >> *(('^' >> factor[_val = lazy_pow(_val, _1)]))
            ;

        primary =
            real[_val = _1]
            | '(' >> expression[_val = _1] >> ')'
            | ('-' >> primary[_val = -_1])
            | ('+' >> primary[_val = _1])
            | no_case[constant][_val = _1]
            | (no_case[ufunc] >> '(' >> expression >> ')')[_val = lazy_ufunc(_1, _2)]
            ;

    };
};

bool parse(std::string::const_iterator& iter,
           std::string::const_iterator end,
           const grammar& g,
           double& result)
{
    if(!boost::spirit::qi::phrase_parse(iter, end, g, boost::spirit::ascii::space, result))
    {
        return false;
    };
    // we check if the full string could parsed
    return iter==end;
}

// version with pre-defined constants
bool ParseExpression(const std::string& expression, double& result, const ConstantMap& constants)
{
    grammar g(constants);
    std::string::const_iterator it=expression.begin();
    return parse(it, expression.end(), g, result);
};

void CleanUpParser()
{
};

} // namespace
#endif