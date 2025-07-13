idf_component_register(
    SRCS "calendar.c"
    INCLUDE_DIRS "."
    REQUIRES 
        config
        http_client     # pour http_fetch()
        time_utils      # pour les fonctions utilitaires temps
        json           # pour parser le JSON du calendrier
)
