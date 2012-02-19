

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "php.h"
#include "php_script_server.h"


static void service(php_stream *in, php_stream *out) {
    printf("service(in, out)\n");
    size_t block_size = 1024;
    char *exec_direct = malloc(block_size);
    memset(exec_direct, 0, block_size);
    char *_exec_direct = malloc(block_size);
    char *tmp;
    memset(_exec_direct, 0, block_size);
    size_t didread = 0;
    
    pcre_cache_entry *pce;
    pce = pcre_get_compiled_regex_cache("#//DIESS#", (int)strlen("#//DIESS#" TSRMLS_CC));
    if (pce == NULL) {
        printf("failed to pcre_get_compiled_regex_cache()");
        return;
    }
    
    zval *return_value;
    zval *subpats;
    char *haystack;
    int haystack_len;
    
    MAKE_STD_ZVAL(return_value);
    ALLOC_INIT_ZVAL(subpats);
    
    while((tmp = _php_stream_get_line(in, _exec_direct, block_size, &didread TSRMLS_CC)) != NULL)
    {
        size_t org_size = strlen(exec_direct) + 1;
        size_t new_exec_direct_size = org_size + (size_t)didread;
        char *new_exec_direct = malloc(new_exec_direct_size);
        memset(new_exec_direct, 0, new_exec_direct_size);
        strncpy(new_exec_direct, exec_direct, org_size);
        free(exec_direct);
        exec_direct = new_exec_direct;
        printf("_exec_direct: %s\ndidread: %d\n", _exec_direct, didread);

        haystack_len = (int)strlen(_exec_direct);
        haystack = estrndup(_exec_direct, haystack_len);
        php_pcre_match_impl(pce, haystack, haystack_len, return_value, subpats, 1, 1, PREG_PATTERN_ORDER, 0 TSRMLS_CC);
        
        
        strncat(exec_direct, _exec_direct, didread);
        memset(_exec_direct, 0, block_size);
        efree(haystack);
        haystack_len = 0;

        if ((Z_LVAL_P(return_value) > 0)) {
            printf("break read loop");
            break;
        }
    }
    
    FREE_ZVAL(return_value);
    zval_ptr_dtor(&subpats);
    
    if (zend_eval_string_ex(exec_direct, NULL, "Command line code", 1 TSRMLS_CC) == FAILURE) {
            //exit_status=254;
    }
    free(exec_direct);
    free(_exec_direct);
    
}

extern int phpss_fileno;

static int script_server(int server) {
    printf("script_server(%d)\n", server);
    int exit_status = 0;
    for (;;) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof addr;
        int sock;
        int pid;
        sock = accept(server, (struct sockaddr *)&addr, &addrlen);
        
        if (sock < 0) {
            PUTS("accept failed\n");
            printf("accept(2) failed: %s", strerror(errno));
            exit_status = -1;
            break;
        }
        
        pid = fork();
        if (pid < 0) {
            PUTS("fork failed\n");
            exit_status = -3;
            break;
        } else if (pid == 0) {
            
            char sock_url[256];
            memset(&sock_url, 0, sizeof(sock_url));
            sprintf(&sock_url, "php://fd/%d", sock);
            php_stream *stream_in = php_stream_open_wrapper(&sock_url, "rb", 0, NULL);
            //php_stream *stream_out = php_stream_open_wrapper(&sock_url, "wb", 0, NULL);
            php_stream *stream_out = NULL;
            phpss_fileno = sock;
            service(stream_in, stream_out);
            php_stream_close(stream_in);
            php_stream_close(stream_out);
            exit(0);
        }
        close(sock);
    }
    return exit_status;
}

static int listen_socket(char *port) {
    printf("listen_socket(%d)\n", port);
    struct addrinfo hints, *res, *ai;
    int sock = -1;
    int backlog = 5;
    int err;
    
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((err = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        return -1;
    }
    
    for (ai = res; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock < 0) continue;
        if (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            close(sock);
            continue;
        }
        if (listen(sock, backlog) < 0) {
            close(sock);
            continue;
        }
        freeaddrinfo(res);
        break;
    }
    
    return sock;
}

int do_script_server(char *port) {
    printf("do_script_server(%s)\n", port);
    int server = listen_socket(port);
    return script_server(server);
}
