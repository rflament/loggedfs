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
#ifndef LOGGEDFS_FILTER_H
#define LOGGEDFS_FILTER_H

#include <stdio.h>
#include <string.h>


class Filter
{
public:
	Filter();
	~Filter();
	const char* getExtension() {return extension;};
	int getUID() {return uid;};
	const char* getAction() {return action;};
	const char* getRetname() {return retname;};
	
	void setExtension(const char* e) {this->extension=strdup(e);};
	void setUID(int u) {this->uid=u;};
	void setAction(const char* a) {this->action=strdup(a);};
	void setRetname(const char* r) {this->retname=strdup(r);};
	bool matches(const char* path, int uid, const char *action, const char* retname);
	
private:
	const char* extension;
	int uid;
	const char* action;
	const char* retname;
	bool matches( const char* str,const char* pattern);
};


#endif
