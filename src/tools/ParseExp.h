// -*- c-basic-offset: 4 -*-

/** @file ParseExp.h
 *
 *  @brief function to parse expressions from strings
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PARSEEXP_H
#define PARSEEXP_H
#include <map>
#include <string>

namespace Parser
{
typedef std::map<const char*, double> ConstantMap;

bool ParseExpression(const std::string expression, double& result);
bool ParseExpression(const std::string expression, double& result, const ConstantMap constants);

};

#endif