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
#include <regex>

#define OVECCOUNT 30


Filter::Filter()
{
}

Filter::~Filter()
{
}

bool Filter::matches( const char* str,const char* pattern)
{
    std::regex re(pattern);

    std::smatch match;

    std::string strstr(str);

    return std::regex_match(strstr, match, re);
}

bool Filter::matches(const char* path,int uid, const char *action, const char* retname)
{
bool a= (matches(path,this->extension) && (uid==this->uid || this->uid==-1) && matches(action,this->action) && matches(retname,this->retname));

return a;
}
