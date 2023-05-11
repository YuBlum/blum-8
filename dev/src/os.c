#include <os.h>

#define MAX_PATH 1028

#ifdef LINUX
#include <os_linux.h>
#else
#include <os_windows.h>
#endif
