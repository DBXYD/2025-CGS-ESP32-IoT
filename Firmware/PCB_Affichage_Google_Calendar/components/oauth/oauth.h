#ifndef OAUTH_H
#define OAUTH_H

#include <stdbool.h>
#include <stddef.h>

#define OAUTH_CLIENT_ID   GOOGLE_CLIENT_ID
#define OAUTH_SCOPE       GOOGLE_SCOPE

/**
 * Lance le Device Code Flow OAuth2.
 * @param device_code   buffer (>=256) pour recevoir le device_code
 * @param user_code     buffer (>=32)  pour recevoir le user_code
 * @param verify_url    buffer (>=128) pour recevoir l’URL de vérification
 * @param interval      pointeur vers l’intervalle de polling (secondes)
 * @return true si succès
 */
bool oauth_get_device_code(char *device_code, char *user_code, char *verify_url, int *interval);

/**
 * Poll pour récupérer access_token & refresh_token.
 */
bool oauth_poll_token(const char *device_code,
                      char *access_token, int *expires,
                      char *refresh_token);

/**
 * Rafraîchit l’access token.
 */
bool oauth_refresh(const char *refresh_token,
                   char *access_token, int *expires);

/**
 * Charge depuis NVS le refresh token précédemment enregistré.
 * @return true si un token non-vide a été lu
 */
bool load_refresh_token(char *buf, size_t len);

/**
 * Sauvegarde dans NVS le refresh token.
 * @return true si OK
 */
bool save_refresh_token(const char *tok);




#endif // OAUTH_H
