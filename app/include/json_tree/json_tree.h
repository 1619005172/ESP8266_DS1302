/*
 * json_tree.h
 *
 *  Created on: 2021年3月16日
 *      Author: Memory/Kongbai
 */

#ifndef APP_JSON_TREE_H_
#define APP_JSON_TREE_H_
//平台上报数据格式
#define JSON_Tree	"{\n"						\
						"\"mac\":\"%d\",\n"		\
						"\"data\":\n"			\
						"{\n"					\
							"\"temp\":\"%s\",\n"\
							"\"humi\":\"%s\"\n"	\
						"}\n"					\
					"}\n"						\
//平台下发指令格式
#define JSON_Ctrl   "{\n"						\
						"\"mac\":\"%d\",\n"		\
						"\"data\":\"%s\"\n"		\
					"}\n"						\


#endif
