
#ifndef _tst_h_
#define _tst_h_

#ifndef MESSAGE
#define MESSAGE(foo) fprintf(stderr,"line %d: %s", __LINE__, foo)
#endif

#ifndef NULL
#define NULL 0
#endif

#include <unistd.h>

#endif