#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <esp_err.h>

/**
 * Effectue une requête HTTP GET ou POST.
 * @param url URL complète
 * @param method "GET" ou "POST"
 * @param bearer_token NULL ou token OAuth2
 * @param post_fields pour POST, sinon NULL
 * @param[out] out_status code HTTP
 * @return corps de réponse alloué, à free() par l'appelant
 */
char *http_fetch(const char *url,
                 const char *method,
                 const char *bearer_token,
                 const char *post_fields,
                 int *out_status);

#endif // HTTP_CLIENT_H