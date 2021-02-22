int isWifiGotIp();
void wifi_init_sta();
esp_err_t event_handler(void *ctx, system_event_t *event);

void wifi_init_sniffer();
static void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
static void periodic_timer_callback(void* arg);
void ckSniffAlive( void );

void wifi_resume();
void wifi_stop();
