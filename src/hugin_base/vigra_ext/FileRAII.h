/** @file fileRAII.h
 *
 *  @author Lukas Jirkovsky <l.jirkovsky@gmail.com>
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <cstdio>
#include <cerrno>

#include <vigra/error.hxx>

namespace vigra_ext
{
/** Class used for opening files.
 * File class using RAII (Resource Acquisition Is Initialization) technique.
 * This ensures that the file is properly close even when an exception occurs.
 */
class FileRAII
{
public:
    /** Open the specified file.
     *
     * \param path path to a file
     * \param mode mode as defined by the fopen function
     */
    FileRAII(const char *path, const char *mode)
    {
        file = std::fopen(path, mode);
        if (file == 0) {
            std::string msg("Unable to open file '");
            msg += path;
            msg += "'.";
            vigra_precondition(0, msg.c_str());
        }
    }

    ~FileRAII()
    {
        if (file) {
            errno = 0;
            if (std::fclose(file)) {
                // there are several possible errors, handle some of them
                std::string msg;
                switch(errno)
                {
                    case EBADF:
                        msg = "Bad file descriptor.";
                        break;
                    case EIO:
                        msg = "An I/O error occurred while closing the file.";
                        break;
                    default:
                        msg = "An error ocured while closing the file.";
                        break;
                }
                vigra_postcondition(0, msg.c_str());
            }
        }
    }

    /** Get pointer to opened file.
     * \return FILE* pointer to the opened file.
     */
    FILE* get()
    {
        return file;
    }

private:
    FILE* file;

    // this class should never be copied or assigned
    FileRAII(const FileRAII &);
    FileRAII& operator=(const FileRAII &);
};

}