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

#include "Config.h"
#include <fstream>
#include <iostream>




xmlChar* INCLUDE=xmlCharStrdup("include");
xmlChar* EXCLUDE=xmlCharStrdup("exclude");
xmlChar* USER=xmlCharStrdup("uid");
xmlChar* EXTENSION=xmlCharStrdup("extension");
xmlChar* ACTION=xmlCharStrdup("action");
xmlChar* RETNAME=xmlCharStrdup("retname");
xmlChar* ROOT=xmlCharStrdup("loggedFS");
xmlChar* LOG_ENABLED=xmlCharStrdup("logEnabled");
xmlChar* PNAME_ENABLED=xmlCharStrdup("printProcessName");


Config::Config()
{
    // default values
    enabled=true; 
    pNameEnabled=true;
}

Config::~Config()
{
    includes.clear();
    excludes.clear();
}

void Config::parse(xmlNode * a_node)
{

xmlNode *cur_node = NULL;

for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
	{
	if (cur_node->type == XML_ELEMENT_NODE) 
		{
		xmlAttr *attr=cur_node->properties;
		if (xmlStrcmp(cur_node->name,ROOT)==0)
			{
			while (attr!=NULL)
				{
				if (xmlStrcmp(attr->name,LOG_ENABLED)==0)
					{
					//enable or disable loggedfs
					if (xmlStrcmp(attr->children->content,xmlCharStrdup("true"))!=0)
						{
						enabled=false;
						printf("Log disabled\n");
						}
					else	{
						printf("Log enabled\n");
						}
					}
				else if (xmlStrcmp(attr->name,PNAME_ENABLED)==0)
					{
					//enable or disable process name prints in loggedfs
					if (xmlStrcmp(attr->children->content,xmlCharStrdup("true"))!=0)
						{
						pNameEnabled=false;
						printf("print process name disabled\n");
						}
					else	{
						printf("print process name enabled\n");
						}
					}
				else printf("unknown attribute : %s\n",attr->name);
				attr=attr->next;
				}
			}
		if (xmlStrcmp(cur_node->name,INCLUDE)==0 || xmlStrcmp(cur_node->name,EXCLUDE)==0)
			{
			Filter* filter=new Filter();
			char* buffer=new char[100];
			while (attr!=NULL)
				{
				
				sprintf(buffer,"%s",attr->children->content); // I guess there's another way to do that
				if (xmlStrcmp(attr->name,EXTENSION)==0)
					{
					filter->setExtension(buffer);
					}
				else if (xmlStrcmp(attr->name,USER)==0)
					{
					if (strcmp(buffer,"*"))
						filter->setUID(atoi(buffer));
					else filter->setUID(-1); // every users

					}
				else if (xmlStrcmp(attr->name,ACTION)==0)
					{
					filter->setAction(buffer);
					}
				else if (xmlStrcmp(attr->name,RETNAME)==0)
					{
					filter->setRetname(buffer);
					}
				else printf("unknown attribute : %s\n",attr->name);
				attr=attr->next;
				}
			
			if (xmlStrcmp(cur_node->name,INCLUDE)==0)
				{
				includes.push_back(*filter);
				}
			else excludes.push_back(*filter);
			delete buffer;
			}		
    		}
		

   	parse(cur_node->children);
	}

}

bool Config::loadFromXml(xmlDoc* doc)
{
	
	xmlNode *root_element = NULL;
	root_element = xmlDocGetRootElement(doc);
	
	parse(root_element);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return true;
}

bool Config::loadFromXmlBuffer(const char* buffer)
{
	xmlDoc *doc = NULL;

	LIBXML_TEST_VERSION
	
	doc=xmlReadMemory(buffer,strlen(buffer),"",NULL, XML_PARSE_NONET);
	return loadFromXml(doc);
}

bool Config::loadFromXmlFile(const char* filename)
{
	xmlDoc *doc = NULL;

	LIBXML_TEST_VERSION
	
	doc = xmlReadFile(filename, NULL, 0);
	return loadFromXml(doc);
}

bool Config::shouldLog(const char* filename, int uid, const char* action, const char* retname)
{
    bool should=false;
    if (enabled)
    {
    	if (includes.size()>0)
	{
		for (unsigned int i=0;i<includes.size() && !should;i++)
		{
		Filter f=includes[i];
		if (f.matches(filename,uid,action,retname))
			should=true;
		}
		for (unsigned int i=0;i<excludes.size() && should;i++)
		{
		Filter f=excludes[i];
		if (f.matches(filename,uid,action,retname))
			should=false;
		}
	}
	else
	{
		should=true;
	}

    }
    
    return should;
}

