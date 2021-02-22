#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/apps/sntp.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include <arpa/inet.h>
#include <sys/socket.h>

#include "wifi_set.h"
#include "ota.h"
#include "eth_main.h"
#include "sock_client_init.h"

#define TRUE 1
#define FALSE 0

#define OTA_WIFI_SSID    CONFIG_OTA_WIFI_SSID
#define OTA_WIFI_PASS    CONFIG_OTA_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

static const char *TAG = "wifi set";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
const int OTA_WIFI_CONNECTED_BIT = BIT0;
static int s_retry_num = 0;
ip4_addr_t ip;
ip4_addr_t gw;
ip4_addr_t msk;

int isGotIp = 0;
int isStaOn = 0;
int isHandlerOn = 0;

static uint8_t mac_des[6];




void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    
    if(isHandlerOn == 0)
    {        
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );
        isHandlerOn = 1;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            .ssid = OTA_WIFI_SSID,
            .password = OTA_WIFI_PASS,
            .channel = 0
        },
    };


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start() );
    
    
    printf("connect to ap SSID:%s password:%s",OTA_WIFI_SSID, OTA_WIFI_PASS);

    isStaOn = 1;
    s_retry_num = 0;
}

int isWifiGotIp()
{
    return isGotIp;
}


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) 
    {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ip = event->event_info.got_ip.ip_info.ip;
            gw = event->event_info.got_ip.ip_info.gw;
            msk = event->event_info.got_ip.ip_info.netmask;
            ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&ip));
            s_retry_num = 0;
            isGotIp = 1;                    
            xEventGroupSetBits(s_wifi_event_group, OTA_WIFI_CONNECTED_BIT);
            ota_main();
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            isGotIp = 0;                   
            if (isStaOn != 0 && s_retry_num < ESP_MAXIMUM_RETRY) 
            {
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, OTA_WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP, s_retry_num = %d!", s_retry_num);
            }
            else if(s_retry_num >= ESP_MAXIMUM_RETRY) 
            {    
				ESP_LOGI(TAG, "Change to Sniffer");
				wifi_stop();
                eth_main();
                init_socket_clinet();
                ckSniffAlive();   
                wifi_init_sniffer();  
            }
            break;
            
     
        default:
            break;
    }


    return ESP_OK;
}



void wifi_resume()
{
    isStaOn = 1;
    s_retry_num = 0;
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_stop()
{
    isStaOn = 0;
    ESP_ERROR_CHECK(esp_wifi_stop()); 
}


