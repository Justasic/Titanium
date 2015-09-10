#error "This platform is not yet supported."
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <Windows.h>


#include "misc.h"


///////////////////////////////////////////////////
// Function: GetSystemInformation (Linux variant)
//
// description:
// Collects the system information for the platform
// and fills out the platform-independent information_t
// structure. This function is called in main.c
information_t *GetSystemInformation()
{
    return NULL;
}
