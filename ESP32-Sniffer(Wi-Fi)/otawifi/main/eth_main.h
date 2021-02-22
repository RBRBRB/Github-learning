
//static void phy_device_power_enable_via_gpio(bool enable);
//static void eth_gpio_config_rmii(void);
void eth_task(void *pvParameter);
uint8_t initEthernet();
void eth_main();
char isEthGotIp();

