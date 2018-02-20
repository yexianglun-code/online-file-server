#ifndef __USER_H__
#define __USER_H__
#include "head.h"
#include "database.h"
#include "transfile.h"

int get_user_info(MYSQL *conn, char *user_name, MYSQL_RES **res);
int user_verify(MYSQL *conn, int sfd, int *user_id);
int user_signup(MYSQL *conn, int sfd);

#endif
