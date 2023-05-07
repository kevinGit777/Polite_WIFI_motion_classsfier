#ifndef ESP32_CSI_CSI_COMPONENT_H
#define ESP32_CSI_CSI_COMPONENT_H

#include "math.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM (0)
#define ECHO_UART_BAUD_RATE (460800)


// char *project_type;

#define CSI_RAW 1
#define CSI_AMPLITUDE 1
#define CSI_PHASE 0

#define CSI_TYPE CSI_RAW

#define true 1
#define false 0
#define BUF_SIZE (1024)

// static const SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

int _mac_filter(char mac[])
{
    // const char recieveMac[] = "B0:67:B5:94:FA:C4";
    // const char recieveMac[] = "58:11:22:27:55:A0";
    // const char recieveMac[] = "0C:96:E6:8B:0A:0F";

    const char tarMac_1[] = "80:03:84:0A:EC:38";
    // const char tarMac_2[] = "B0:67:B5:94:FA:C4";
    // int res = strncmp(tarMac_1, mac, sizeof(tarMac_1));
    // printf("MAC: %s, tar: %s, res: %i%\n", mac, tarMac_1, res );

    return strncmp(tarMac_1, mac, sizeof(tarMac_1));
}

void _wifi_csi_cb(void *ctx, wifi_csi_info_t *data)
{

    //_print_csi_csv_header();
    // xSemaphoreTake(mutex, portMAX_DELAY);

    wifi_csi_info_t d = data[0];
    char src_mac[20] = {0};
    sprintf(src_mac, "%02X:%02X:%02X:%02X:%02X:%02X", d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    // char dest_mac[20] = {0};
    // sprintf(dest_mac, "%02X:%02X:%02X:%02X:%02X:%02X", d.dmac[0], d.dmac[1], d.dmac[2], d.dmac[3], d.dmac[4], d.dmac[5]);
    if (_mac_filter(src_mac) != 0)
    {
        // printf("Invalid\n");
    }
    else
    {

        int data_len = data->len;

        int8_t *my_ptr;

        // RAW
        //  my_ptr = data->buf;
        //  printf( "\nRAW: \n" );
        //  for (int i = 0; i < data_len; i++) {
        //      printf( "%i \t", my_ptr[i] );
        //  }
        //  printf( "\n" );

        // AMP
        my_ptr = data->buf;
        printf("MAC is %s\n", src_mac);
        printf("AMP: len is %d \n", data_len);
        
        
        char timestamp[50] = {0};
        sprintf(timestamp, "%d\t", d.rx_ctrl.timestamp);
        uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) timestamp, strlen(timestamp));
        
        char amp[50] = {0};
        uart_write_bytes(ECHO_UART_PORT_NUM, "[", 1);
        for (int i = 0; i < data_len / 2; i++)
        {
            sprintf(amp, "%f, ", sqrt(pow(my_ptr[i * 2], 2) + pow(my_ptr[(i * 2) + 1], 2)));
            uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) amp, strlen(amp));
        }
        uart_write_bytes(ECHO_UART_PORT_NUM, "]", 1);
        uart_write_bytes(ECHO_UART_PORT_NUM, "\n", 1);
        //printf("\n\n");

        // CSI_PHASE
        //  my_ptr = data->buf;
        //  printf("\nPHASE:\n");
        //  for (int i = 0; i < data_len / 2; i++) {
        //      printf( "%f \t",  atan2(my_ptr[i*2], my_ptr[(i*2)+1]));
        //  }
        //  printf( "\n");
        // vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void _print_csi_csv_header()
{
    char *header_str = (char *)"type,role,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,real_timestamp,len,CSI_DATA\n";
    printf(header_str);
}

void csi_init(char *type)
{
    // project_type = type;

    ESP_ERROR_CHECK(esp_wifi_set_csi(1));

    // @See: https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/include/esp_wifi_types.h#L401
    wifi_csi_config_t configuration_csi;
    configuration_csi.lltf_en = 1;
    configuration_csi.htltf_en = 1;
    configuration_csi.stbc_htltf2_en = 1;
    configuration_csi.ltf_merge_en = 1;
    configuration_csi.channel_filter_en = 0;
    configuration_csi.manu_scale = 0;

    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&configuration_csi));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(&_wifi_csi_cb, NULL));
}

#endif // ESP32_CSI_CSI_COMPONENT_H
void nvs_init()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void uart_init()
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
}