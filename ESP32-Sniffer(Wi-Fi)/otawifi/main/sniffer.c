#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "ping/ping.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_ping.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/uart.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "rom/uart.h"
#include <esp_sleep.h>

#include "def_vars.h"
#include "eth_main.h"
#include "wifi_set.h"
#include "sock_client_init.h"

#define TRUE 1
#define FALSE 0

#define SNIFFER_CHANNEL CONFIG_SNIFFER_CHANNEL


static void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
inline static void socket_data_send(char *Buf);
inline static void socket_data_send_ck(char *Buf);
static void console();
void wifi_init_sniffer();
void ckSniffAlive( void );
static void periodic_timer_callback(void* arg);

static const char *TAG = "wifi station";
static EventGroupHandle_t s_wifi_event_group;
static uint8_t mac_des[6];

extern int ser_sock;
extern struct sockaddr_in client;
extern struct sockaddr_in server;

extern int ser_sock_ck;
extern struct sockaddr_in client_ck;
extern struct sockaddr_in server_ck;

extern char got_ip;

void app_main()
{
    console();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init_sta();
	//eth_main();
    //init_socket_clinet();
    //ckSniffAlive();   
    //wifi_init_sniffer();  
}

inline static void socket_data_send(char *Buf)
{
    if( sendto(ser_sock, Buf, sizeof(esp_snif_packet_t), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        puts("Send failed");
        //exit(0);
    }
}



inline static void socket_data_send_ck(char *Buf)
{
    if( sendto(ser_sock_ck, Buf, 6, 0, (struct sockaddr*)&server_ck, sizeof(server_ck)) < 0)
    {
        puts("Send failed");
        //exit(0);
    }
}


static void console()
{
    const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_EVEN,
    .stop_bits = UART_STOP_BITS_1,
    //.use_ref_tick = true,
    //.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
    //.rx_flow_ctrl_thresh = 122,            

    };

    ESP_ERROR_CHECK( uart_driver_delete(UART_NUM_0) );

    ESP_ERROR_CHECK( uart_param_config(UART_NUM_0, &uart_config) );
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
   // Setup UART buffered IO with event queue
    const int uart_buffer_size = 168;
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, uart_buffer_size,uart_buffer_size, 168, &uart_queue, 0));



}


void wifi_init_sniffer()
{
    bool snif_mode;
    wifi_ps_type_t wc_ps;
    esp_read_mac(mac_des, 1);
    while( 1 ){
        if( got_ip != 0 ){
        printf("init_sniffer\n");
        s_wifi_event_group = xEventGroupCreate();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL) );
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(1));
        ESP_ERROR_CHECK(esp_wifi_get_ps(&wc_ps));
        ESP_ERROR_CHECK(esp_wifi_get_promiscuous(&snif_mode));
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
        ESP_ERROR_CHECK(esp_wifi_set_channel(1,0));
        break;
    }
    sleep(1);
    }


}


static void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
	static esp_snif_packet_t send_pkt;

	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
        
	send_pkt.timestamp = ppkt->rx_ctrl.timestamp;
	send_pkt.channel = ppkt->rx_ctrl.channel;
	send_pkt.rssi = ppkt->rx_ctrl.rssi;
	memcpy(send_pkt.addr1, hdr->addr1,6);
	memcpy(send_pkt.addr2, hdr->addr2,6);
	memcpy(send_pkt.mac_des, mac_des,6);
    printf("AP MAC = \"%02X-%02X-%02X-%02X-%02X-%02X\", ", hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
    printf("UE MAC = \"%02X-%02X-%02X-%02X-%02X-%02X\", ", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
    printf("Sniffer MAC = \"%02X-%02X-%02X-%02X-%02X-%02X\"\n", mac_des[0], mac_des[1], mac_des[2], mac_des[3], mac_des[4], mac_des[5]);
	//uart_write_bytes(UART_NUM_0, (char *)&send_pkt, sizeof(esp_snif_packet_t));
	socket_data_send((char *)&send_pkt);
}

void ckSniffAlive( void )
{
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 3000000));



}

static void periodic_timer_callback(void* arg)
{    
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
    init_socket_clinet_ck();
    socket_data_send_ck((char *)mac_des);
    close(ser_sock_ck);
}

