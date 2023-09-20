/* hack to use old libbfd.a w/ new C libs (as in RedHat 6.x) */

#include <sys/stat.h>

void _fxstat(int ver, int fildes, struct stat* sbuf);
void _xstat(int ver, const char* fname, struct stat* sbuf);

void _fxstat(int ver, int fildes, struct stat* sbuf)
{
  return;
}

void _xstat(int ver, const char* fname, struct stat* sbuf)
{
  return;
}
