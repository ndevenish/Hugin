// -*- c-basic-offset: 4 -*-
/** @file utils.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.h 1952 2007-04-15 20:57:55Z dangelo $
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _UTILS_H
#define _UTILS_H

#include <string>
#include <iostream>

namespace AppBase
{
    
    
// === [TODO:  to make string class generic with template ] ===
    
    class ProgressReporter
    {
    public:
        virtual ~ProgressReporter() {};
        virtual bool increaseProgress(double delta) = 0;
        virtual bool increaseProgress(double delta, const std::string & msg) = 0;
        virtual void setMessage(const std::string & msg) = 0;
    };

    
    
    class StreamProgressReporter : public ProgressReporter
    {
    public:
        StreamProgressReporter(double maxProgress, std::ostream & out=std::cout);

        virtual ~StreamProgressReporter();

        virtual bool increaseProgress(double delta);
        virtual bool increaseProgress(double delta, const std::string & msg);
        virtual void setMessage(const std::string & msg);

        void print();
    private:
        double m_progress;
        double m_maxProgress;
        std::string m_message;
        std::ostream & m_stream;
    };


} // namespace


#endif // _UTILS_H
