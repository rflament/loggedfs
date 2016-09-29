/*****************************************************************************
 * Author:   Remi Flament <rflament at laposte dot net>
 *****************************************************************************
 * Copyright (c) 2005 - 2007, Remi Flament
 *
 * This library is free software; you can distribute it and/or modify it under
 * the terms of the GNU General Public License (GPL), as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GPL in the file COPYING for more
 * details.
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
