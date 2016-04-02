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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PARSEEXP_H
#define PARSEEXP_H
#include <map>
#include <string>
#include "hugin_config.h"

namespace Parser
{
typedef std::map<std::string, double> ConstantMap;

bool ParseExpression(const std::string& expression, double& result, const ConstantMap& constants = ConstantMap());
void CleanUpParser();
};

#endif