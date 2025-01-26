/* *****************************************************************
*
* Copyright 2016 Microsoft
*
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************/

#ifndef GETOPT_H__
#define GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>

extern int     opterr,          /* if error message should be printed */
		optind,					/* index into parent argv vector */
		optopt,                 /* character checked for validity */
		optreset;               /* reset getopt */
extern char* optarg;            /* argument associated with option */

int getopt(int nargc, char* const nargv[], const char* ostr);

#ifdef __cplusplus
}
#endif

#endif

