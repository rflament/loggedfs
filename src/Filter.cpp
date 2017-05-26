/*****************************************************************************
 * Author:   Remi Flament <remipouak at gmail dot com>
 *****************************************************************************
 * Copyright (c) 2005 - 2016, Remi Flament
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */
#include "Filter.h"
#include <pcre.h>

#define OVECCOUNT 30


Filter::Filter()
{
}

Filter::~Filter()
{
}

bool Filter::matches( const char* str,const char* pattern)
{
    pcre *re;
    const char *error;
    int ovector[OVECCOUNT];
    int erroffset;


    re = pcre_compile(
             pattern,
             0,
             &error,
             &erroffset,
             NULL);


    if (re == NULL)
    {
        printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        return false;
    }

    int rc = pcre_exec(
                 re,                   /* the compiled pattern */
                 NULL,                 /* no extra data - we didn't study the pattern */
                 str,              /* the subject string */
                 strlen(str),       /* the length of the subject */
                 0,                    /* start at offset 0 in the subject */
                 0,                    /* default options */
                 ovector,              /* output vector for substring information */
                 OVECCOUNT);           /* number of elements in the output vector */

    pcre_free(re);
    return (rc >= 0);
}




bool Filter::matches(const char* path,int uid, const char *action, const char* retname)
{
bool a= (matches(path,this->extension) && (uid==this->uid || this->uid==-1) && matches(action,this->action) && matches(retname,this->retname));

return a;
}
