/*****************************************************************************
 * Author:   Remi Flament <remipouak at gmail dot com>
 *****************************************************************************
 * Copyright (c) 2005 - 2018, Remi Flament
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
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#define OVECCOUNT 30


Filter::Filter()
{
}

Filter::~Filter()
{
}

bool Filter::matches( const char* str,const char* pattern)
{
    pcre2_code *re;
    int error;
    pcre2_match_data *match_data;
    PCRE2_SIZE erroffset;


    re = pcre2_compile(
             (PCRE2_SPTR)pattern,
             PCRE2_ZERO_TERMINATED,
             0,
             &error,
             &erroffset,
             NULL);


    if (re == NULL)
    {
        printf("PCRE compilation failed at offset %zu: %d\n", erroffset, error);
        return false;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);

    int rc = pcre2_match(
                 re,                   /* the compiled pattern */
                 (PCRE2_SPTR)str,              /* the subject string */
                 strlen(str),       /* the length of the subject */
                 0,                    /* start at offset 0 in the subject */
                 0,                    /* default options */
                 match_data,
                 NULL);

    pcre2_code_free(re);
    return (rc >= 0);
}




bool Filter::matches(const char* path,int uid, const char *action, const char* retname)
{
bool a= (matches(path,this->extension) && (uid==this->uid || this->uid==-1) && matches(action,this->action) && matches(retname,this->retname));

return a;
}
