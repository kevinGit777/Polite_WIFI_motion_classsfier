#include "freertos/FreeRTOS.h"

#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "nvs_flash.h"
#include "string.h"


uint8_t beacon_packet[] = { 
				//null frame
                        0x48, 0x00, //Frame Control version=0 type=0(management)  subtype=1000 (beacon) fromDS toDS set to 0
                        0x00, 0x00, //Duration
						0xD4, 0xAD, 0xFC, 0x02, 0x7B, 0xAC, //humidifier
						
						//0x34, 0x2f, 0xBD, 0xDB, 0x26, 0x54, //switch 
						//0x58, 0x11, 0x22 ,0x27 ,0x55 ,0xA0,  //router
                /*4*/  //xiaomi phone 0x50, 0x8e, 0x49, 0x60, 0x5a, 0x02, //A1 Destination address  broadcast or NaN multicast (51-6f-9a-01-00-00) 
						//0xD4, 0xAD, 0xFC, 0x02, 0x7B, 0xaa,
                /*10*/  0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xbb, //A2 Source address - 
                /*16*/  0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xbb, //A3 BSSID 
                /*22*/  0x00, 0x00 //Seq-ctl
                
                // // Frame body starts here
                // /*24*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, //timestamp - the number of microseconds the AP has been active
                // /*32*/  0xE8, 0x03, //Beacon interval
                // /*34*/  0x20, 0x04, //Capability info

                // // test SSI (to remove)
                // 0x00, 0x04, 116, 101, 115, 116,

                // // NaN IE with their attributes
                // 0xdd, 0x19, 0x50, 0x6f, 0x9a,0x13,    // 0xdd=NaN IE, len, OUI of wifi alliance, 0x13=type and version of the Nan IE

                // // NaN attributes
                // 0x00, 0x02,0x00, 0x02, 0x7f,  // Master Indication Attribute  Master preference= 0x02, Random=0x7f
                // 0x01, 0x0d,0x00, 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08, 0x00, 0x00,0x00,0x00,0x00  // Cluster Attribute      
};

uint8_t action_packet[] = { 
                        0x0d, 0x00, //Frame Control version=0 type=0(management)  subtype=1101 (action) fromDS toDS set to 0
                        0x00, 0x00, //Duration
                /*4*/   //0x51, 0x6f, 0x9a, 0x10, 0x00, 0x00, //A1 router Destination address  broadcast or NaN multicast (51-6f-9a-01-00-00) 
						
                /*10*/  0x38, 0x10, 0x03, 0x04, 0x05, 0x06, //A2 Source address - 
                /*16*/  0x50, 0x6f, 0x9a, 0x01, 0x02, 0x03, //A3 BSSID 
                /*22*/  0xf0, 0x01, //Seq-ctl
                
                // Frame body starts here
                0x04,0x09,   0x50,0x6f,0x9a, 0x13,   //Public Action, Vendor specific, Uoi=Wifi Alliance, Oui type
                0x03,  0x09,0x00,  0x44,0xc6,0xf2,0x0e,0x42,0x22,  0x01, 0x00,  0x00  // Service Descriptor              
};

// swap these to attempt to send an action frame
#define PKT2SEND beacon_packet
//#define PKT2SEND action_packet

// esp_err_t event_handler(void *ctx, system_event_t *event) {
// 	return ESP_OK;
// }

void spam_task(void *pvParameter) {

	for (;;) {
		vTaskDelay(500 / portTICK_PERIOD_MS);
		printf("SPAM");
		esp_wifi_80211_tx(WIFI_IF_AP, PKT2SEND, sizeof(PKT2SEND), false);
	}
}

void app_main(void) {
	nvs_flash_init();
	esp_netif_init();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	//ESP_ERROR_CHECK(esp_event_loop_run(event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	// Init dummy AP to specify a channel and get WiFi hardware into
	// a mode where we can send the actual fake beacon frames.
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t ap_config = {
		.ap = {
			.ssid = "esp32-beaconspam",
			.ssid_len = 0,
			.password = "dummypassword",
			.channel = 1,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = 1,
			.max_connection = 4,
			.beacon_interval = 100000
		}
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	xTaskCreate(&spam_task, "spam_task", 2048, NULL, 5, NULL);
}