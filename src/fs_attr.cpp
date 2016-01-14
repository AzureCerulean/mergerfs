/*
  Copyright (c) 2016, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

using std::string;

namespace fs
{
  namespace attr
  {
    static
    int
    get_fs_ioc_flags(const int  fd,
                     int       &flags)
    {
      return ::ioctl(fd,FS_IOC_GETFLAGS,&flags);
    }

    static
    int
    get_fs_ioc_flags(const string &file,
                     int          &flags)
    {
      int fd;
      int rv;
      const int openflags = O_RDONLY|O_NONBLOCK;

      fd = ::open(file.c_str(),openflags);
      if(fd == -1)
        return -1;

      rv = get_fs_ioc_flags(fd,flags);
      if(rv == -1)
        {
          int error = errno;
          ::close(fd);
          errno = error;
          return -1;
        }

      return ::close(fd);
    }

    static
    int
    set_fs_ioc_flags(const int fd,
                     const int flags)
    {
      return ::ioctl(fd,FS_IOC_SETFLAGS,&flags);
    }

    static
    int
    set_fs_ioc_flags(const string &file,
                     const int     flags)
    {
      int fd;
      int rv;
      const int openflags = O_RDONLY|O_NONBLOCK;

      fd = ::open(file.c_str(),openflags);
      if(fd == -1)
        return -1;

      rv = set_fs_ioc_flags(fd,flags);
      if(rv == -1)
        {
          int error = errno;
          ::close(fd);
          errno = error;
          return -1;
        }

      return ::close(fd);
    }

    int
    copy(const int fdin,
         const int fdout)
    {
      int rv;
      int flags;

      rv = get_fs_ioc_flags(fdin,flags);
      if(rv == -1)
        return -1;

      return set_fs_ioc_flags(fdout,flags);
    }

    int
    copy(const string &from,
         const string &to)
    {
      int rv;
      int flags;

      rv = get_fs_ioc_flags(from,flags);
      if(rv == -1)
        return -1;

      return set_fs_ioc_flags(to,flags);
    }
  }
}
