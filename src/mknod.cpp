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

#include <fuse.h>

#include <string>
#include <vector>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "config.hpp"
#include "fs_clonepath.hpp"
#include "fs_path.hpp"
#include "rv.hpp"
#include "rwlock.hpp"
#include "ugid.hpp"

using std::string;
using std::vector;
using namespace mergerfs;

static
int
_mknod(Policy::Func::Search  searchFunc,
       Policy::Func::Create  createFunc,
       const vector<string> &srcmounts,
       const size_t          minfreespace,
       const string         &fusepath,
       const mode_t          mode,
       const dev_t           dev)
{
  int rv;
  int error;
  string dirname;
  vector<string> createpaths;
  vector<string> existingpath;

  dirname = fs::path::dirname(fusepath);
  rv = searchFunc(srcmounts,dirname,minfreespace,existingpath);
  if(rv == -1)
    return -errno;

  rv = createFunc(srcmounts,dirname,minfreespace,createpaths);
  if(rv == -1)
    return -errno;

  error = -1;
  for(size_t i = 0, ei = createpaths.size(); i != ei; i++)
    {
      string &createpath = createpaths[0];

      if(createpath != existingpath[0])
        {
          const ugid::SetRootGuard ugidGuard;
          fs::clonepath(existingpath[0],createpath,dirname);
        }

      fs::path::append(createpath,fusepath);

      rv = ::mknod(createpath.c_str(),mode,dev);

      error = calc_error(rv,error,errno);
    }

  return -error;
}

namespace mergerfs
{
  namespace fuse
  {
    int
    mknod(const char *fusepath,
          mode_t      mode,
          dev_t       rdev)
    {
      const fuse_context      *fc     = fuse_get_context();
      const Config            &config = Config::get(fc);
      const ugid::Set          ugid(fc->uid,fc->gid);
      const rwlock::ReadGuard  readlock(&config.srcmountslock);

      return _mknod(config.getattr,
                    config.mknod,
                    config.srcmounts,
                    config.minfreespace,
                    fusepath,
                    (mode & ~fc->umask),
                    rdev);
    }
  }
}
