#include "ape_ssl.h"
#if _HAVE_SSL_SUPPORT
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CIPHER_LIST "HIGH:!ADH:!MD5"

void ape_ssl_init()
{
    SSL_library_init();            
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

static void ape_ssl_info_callback(const SSL *s, int where, int ret)
{

}

ape_ssl_t *ape_ssl_init_ctx(const char *cert, const char *key)
{
    ape_ssl_t *ssl = NULL;
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
    
    if (ctx == NULL) {
        printf("Failed to init SSL ctx\n");
        return NULL;
    }
    
    ssl = malloc(sizeof(*ssl));
    ssl->ctx = ctx;
    ssl->con = NULL;
    SSL_CTX_set_info_callback(ssl->ctx, ape_ssl_info_callback);
    SSL_CTX_set_options(ssl->ctx, SSL_OP_ALL);
    SSL_CTX_set_default_read_ahead(ssl->ctx, 1);
    
    /* see APE_socket_write() ape_socket.c */
    SSL_CTX_set_mode(ssl->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    
    /* TODO: what for? */
    //SSL_CTX_set_read_ahead(ssl->ctx, 1);
    
    if (SSL_CTX_set_cipher_list(ssl->ctx, CIPHER_LIST) <= 0) {
        printf("Failed to set cipher\n");
        SSL_CTX_free(ctx);
        free(ssl);
        return NULL;
    }
    
    if (SSL_CTX_use_certificate_chain_file(ssl->ctx, cert) == 0) {
        printf("Failed to load cert\n");
        SSL_CTX_free(ctx);
        free(ssl);
        return NULL;
    }
    if (SSL_CTX_use_PrivateKey_file(ssl->ctx, (key != NULL ? key : cert), SSL_FILETYPE_PEM) == 0) {
        printf("Failed to load private key\n");
        SSL_CTX_free(ctx);
        free(ssl);
        return NULL;        
    }
    
    if (SSL_CTX_check_private_key(ssl->ctx) == 0) {
        printf("Private key does not match the certificate public key\n");
        SSL_CTX_free(ctx);
        free(ssl);
        return NULL;
    }
        
    printf("[SSL] New context\n");

    return ssl;
}

ape_ssl_t *ape_ssl_init_global_client_ctx()
{
    ape_ssl_t *ssl = NULL;
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());

    ssl = malloc(sizeof(*ssl));
    ssl->ctx = ctx;
    ssl->con = NULL;

    SSL_CTX_set_options(ssl->ctx, SSL_OP_ALL);
    SSL_CTX_set_default_read_ahead(ssl->ctx, 1);
    
    /* see APE_socket_write() ape_socket.c */
    SSL_CTX_set_mode(ssl->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    //SSL_MODE_AUTO_RETRY

    if (SSL_CTX_set_cipher_list(ssl->ctx, CIPHER_LIST) <= 0) {
        printf("Failed to set cipher\n");
        SSL_CTX_free(ctx);
        free(ssl);
        return NULL;
    }

    return ssl;
}

ape_ssl_t *ape_ssl_init_con(ape_ssl_t *parent, int fd, int accept)
{
    if (parent == NULL) {
        return NULL;
    }
    ape_ssl_t *ssl = NULL;
    SSL_CTX *ctx = parent->ctx;
    
    SSL *con = SSL_new(ctx);
    
    if (con == NULL) {
        return NULL;
    }
    
    if (accept) {
        SSL_set_accept_state(con);
    } else {
        SSL_set_connect_state(con);
    }

    if (SSL_set_fd(con, fd) != 1) {
        printf("Failed to set fd on ssl\n");
        SSL_free(con);
        return NULL;
    }
    
    //SSL_accept(con);
    
    ssl = malloc(sizeof(*ssl));
    ssl->ctx = NULL;
    ssl->con = con;
    
    /* TODO: SSL_set_connect_state for SSL connection */

    return ssl;
}

int ape_ssl_read(ape_ssl_t *ssl, void *buf, int num)
{
    if (ssl == NULL) {
        return 0;
    }
    return SSL_read(ssl->con, buf, num);
}

int ape_ssl_write(ape_ssl_t *ssl, void *buf, int num)
{
    if (ssl == NULL) {
        printf("SSL: Cant write : no ssl ctx\n");
        return 0;
    }
    return SSL_write(ssl->con, buf, num);
}

void ape_ssl_shutdown(ape_ssl_t *ssl)
{
    if (ssl == NULL) {
        printf("SSL: Cant read : no ssl ctx\n");
        return;
    }
    SSL_shutdown(ssl->con);
}

void ape_ssl_destroy(ape_ssl_t *ssl)
{
    if (ssl == NULL) return;
    
    if (ssl->ctx != NULL)
        SSL_CTX_free(ssl->ctx);
    if (ssl->con != NULL)
        SSL_free(ssl->con);
    
    free(ssl);
}
#endif

