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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "fs_path.hpp"
#include "rv.hpp"
#include "rwlock.hpp"
#include "ugid.hpp"

using std::string;
using std::vector;
using mergerfs::Policy;

static
int
_utimens(Policy::Func::Action  actionFunc,
         const vector<string> &srcmounts,
         const size_t          minfreespace,
         const string         &fusepath,
         const timespec        ts[2])
{
  int rv;
  int error;
  vector<string> paths;

  rv = actionFunc(srcmounts,fusepath,minfreespace,paths);
  if(rv == -1)
    return -errno;

  error = -1;
  for(size_t i = 0, ei = paths.size(); i != ei; i++)
    {
      fs::path::append(paths[i],fusepath);

      rv = ::utimensat(0,paths[i].c_str(),ts,AT_SYMLINK_NOFOLLOW);

      error = calc_error(rv,error,errno);
    }

  return -error;
}

namespace mergerfs
{
  namespace fuse
  {
    int
    utimens(const char     *fusepath,
            const timespec  ts[2])
    {
      const fuse_context      *fc     = fuse_get_context();
      const Config            &config = Config::get(fc);
      const ugid::Set          ugid(fc->uid,fc->gid);
      const rwlock::ReadGuard  readlock(&config.srcmountslock);

      return _utimens(config.utimens,
                      config.srcmounts,
                      config.minfreespace,
                      fusepath,
                      ts);
    }
  }
}
