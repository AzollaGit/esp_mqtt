idf_component_register(SRCS "app_main.c" 
                            "app_smartconfig.c"
                            "app_https_ota.c"
                            "app_mqtt.c"
                            "app_touch.c"
                            "app_user.c"
                            "app_sntp.c"
                            "app_uart.c"
                    INCLUDE_DIRS "."
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
