/* 
 * File:   php_script_server.h
 * Author: oasynnoum
 *
 * Created on 2012/02/06, 15:43
 */

#include "ext/pcre/php_pcre.h"

#ifndef PHP_SCRIPT_SERVER_H
#define	PHP_SCRIPT_SERVER_H

#ifndef PREG_PATTERN_ORDER
#define PREG_PATTERN_ORDER			1
#endif

#ifdef	__cplusplus
extern "C" {
#endif

int phpss_fileno;
extern int do_script_server(char *port);


#ifdef	__cplusplus
}
#endif

#endif	/* PHP_SCRIPT_SERVER_H */

