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

#include <sys/statvfs.h>
#include <errno.h>

#include <climits>
#include <string>
#include <vector>
#include <map>

#include "ugid.hpp"
#include "config.hpp"
#include "rwlock.hpp"

using std::string;
using std::vector;
using std::map;
using std::pair;

static
void
_normalize_statvfs(struct statvfs      *fsstat,
                   const unsigned long  min_bsize,
                   const unsigned long  min_frsize,
                   const unsigned long  min_namemax)
{
  fsstat->f_blocks  = (fsblkcnt_t)((fsstat->f_blocks * fsstat->f_frsize) / min_frsize);
  fsstat->f_bfree   = (fsblkcnt_t)((fsstat->f_bfree * fsstat->f_frsize) / min_frsize);
  fsstat->f_bavail  = (fsblkcnt_t)((fsstat->f_bavail * fsstat->f_frsize) / min_frsize);
  fsstat->f_bsize   = min_bsize;
  fsstat->f_frsize  = min_frsize;
  fsstat->f_namemax = min_namemax;
}

static
void
_merge_statvfs(struct statvfs       * const out,
               const struct statvfs * const in)
{
  out->f_blocks += in->f_blocks;
  out->f_bfree  += in->f_bfree;
  out->f_bavail += in->f_bavail;

  out->f_files  += in->f_files;
  out->f_ffree  += in->f_ffree;
  out->f_favail += in->f_favail;
}

static
int
_statfs(const vector<string> &srcmounts,
        struct statvfs       &fsstat)
{
  unsigned long min_bsize   = ULONG_MAX;
  unsigned long min_frsize  = ULONG_MAX;
  unsigned long min_namemax = ULONG_MAX;
  map<unsigned long,struct statvfs> fsstats;
  vector<string>::const_iterator iter;
  vector<string>::const_iterator enditer;

  for(size_t i = 0, ei = srcmounts.size(); i != ei; i++)
    {
      int rv;
      struct statvfs fsstat;
      rv = ::statvfs(srcmounts[i].c_str(),&fsstat);
      if(rv != 0)
        continue;

      if(min_bsize > fsstat.f_bsize)
        min_bsize = fsstat.f_bsize;
      if(min_frsize > fsstat.f_frsize)
        min_frsize = fsstat.f_frsize;
      if(min_namemax > fsstat.f_namemax)
        min_namemax = fsstat.f_namemax;

      fsstats.insert(pair<unsigned long,struct statvfs>(fsstat.f_fsid,fsstat));
    }

  map<unsigned long,struct statvfs>::iterator fsstatiter    = fsstats.begin();
  map<unsigned long,struct statvfs>::iterator endfsstatiter = fsstats.end();
  if(fsstatiter != endfsstatiter)
    {
      fsstat = fsstatiter->second;
      _normalize_statvfs(&fsstat,min_bsize,min_frsize,min_namemax);
      for(++fsstatiter;fsstatiter != endfsstatiter;++fsstatiter)
        {
          _normalize_statvfs(&fsstatiter->second,min_bsize,min_frsize,min_namemax);
          _merge_statvfs(&fsstat,&fsstatiter->second);
        }
    }

  return 0;
}

namespace mergerfs
{
  namespace fuse
  {
    int
    statfs(const char     *fusepath,
           struct statvfs *stat)
    {
      const fuse_context      *fc     = fuse_get_context();
      const Config            &config = Config::get(fc);
      const ugid::Set          ugid(fc->uid,fc->gid);
      const rwlock::ReadGuard  readlock(&config.srcmountslock);

      return _statfs(config.srcmounts,
                     *stat);
    }
  }
}
