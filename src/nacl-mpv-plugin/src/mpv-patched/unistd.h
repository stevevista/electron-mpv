/*
 * MSVC unistd.h compatibility header.
 * Copyright (c) 2015 Matthew Oliver
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SMP_UNISTD_H
#define SMP_UNISTD_H

#ifndef _MSC_VER
#   include_next <unistd.h>
#else

#include <stdlib.h>
#include <process.h>
// #include <direct.h>
#include <fcntl.h>

#define R_OK    4       /* Test for read permission.  */
#define W_OK    2       /* Test for write permission.  */
//#define   X_OK    1       /* execute permission - unsupported in windows*/
#define F_OK    0       /* Test for existence.  */

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#define srandom srand
#define random rand

#define inline __inline
typedef int mode_t;
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

#endif /* _MSC_VER */

#include <../ucrt/time.h>

//#include <wdm.h>
#ifdef _MSC_VER
#include <windows.h>
#include <winternl.h>
typedef enum
{
	FileFsVolumeInformation = 1,
	FileFsLabelInformation = 2,
	FileFsSizeInformation = 3,
	FileFsDeviceInformation = 4,
	FileFsAttributeInformation = 5,
	FileFsControlInformation = 6,
	FileFsFullSizeInformation = 7,
	FileFsObjectIdInformation = 8,
	FileFsDriverPathInformation = 9,
	FileFsVolumeFlagsInformation = 10,
	FileFsSectorSizeInformation = 11
} FS_INFORMATION_CLASS;
typedef struct _FILE_FS_DEVICE_INFORMATION
{
	DEVICE_TYPE DeviceType;
	ULONG       Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;
#endif

#endif /* SMP_UNISTD_H */