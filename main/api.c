/********************************************************
	apic.c
	
	This module contains examples of how to perform
	Spotify API functions with loco
	
*********************************************************/

#include "cJSON.h"
#include <esp_http_client.h>
#include "esp_crt_bundle.h"
#include "loco.h"

#define HTTPREPLYLEN 20000
#define AUTHHEADERLEN 350

int httpReplyLen = 0;
unsigned char *httpReply = NULL;
char *authHeader = NULL;

esp_err_t httpHandler(esp_http_client_event_handle_t evt) {

  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
//    locoLog ("httpHandler length %d\n", evt->data_len);
    int l = evt->data_len;
    if (l + httpReplyLen >= HTTPREPLYLEN){
		printf ("putStateHandler overflow\n");
	}
	else {	
		memmove (httpReply+httpReplyLen,evt->data,l);
	    httpReplyLen += l;	
	}
    break;
  default:
    break;
  }
  return ESP_OK;
}

// This variant of snprintf returns true on overflow

int snpfa (char *buf, int len, char *fmt, ...) {

  va_list ap;
  va_start(ap, fmt);

  int l = vsnprintf(buf, len, fmt, ap);
  va_end(ap);

  if ((l < 0) || (l >= len)) {
    printf ("ERROR snpfa overflow len=%d/%d %s\n", l, len, buf);
    return 1;
  }
  return 0;
}

// returns a cJSON - the caller must use cJSON_Delete afterwards 

cJSON *getMyPlaylists(int offset, int limit) {

  printf("Todo getMyPlaylists() allocate elsewhere\n");

  if (!httpReply)
    httpReply = heap_caps_malloc(HTTPREPLYLEN, MALLOC_CAP_SPIRAM);
  if (!authHeader)
    authHeader = heap_caps_malloc(AUTHHEADERLEN, MALLOC_CAP_SPIRAM);

  char url[200];

  sprintf(url, "https://api.spotify.com:443/v1/me/playlists?limit=%d&offset=%d",
          limit, offset);

  lockHttps();

  esp_http_client_config_t config = {.url = url,
                                     .timeout_ms = 2000,
                                     .buffer_size_tx = 2048,
                                     .buffer_size = 10000,
                                     .crt_bundle_attach = esp_crt_bundle_attach,
                                     .event_handler = httpHandler};

  httpReplyLen = 0;

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_GET);

	snpfa(authHeader, AUTHHEADERLEN, "Bearer %s", getAccessToken());
  
  esp_http_client_set_header(client, "Authorization", authHeader);

  esp_http_client_set_header(client, "User-agent", "okhttp/4.11.0");

  esp_http_client_set_header(client, "Host", "api.spotify.com");

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    printf("getMyPlaylists Status = %d, content_length = %lld\n",
           esp_http_client_get_status_code(client),
           esp_http_client_get_content_length(client));

    cJSON *response = cJSON_Parse((char *)httpReply);
    if (response) {
      esp_http_client_cleanup(client);
      unlockHttps();
      return response;
    }
    cJSON_Delete(response);

  } else {
    printf("getMyPlaylists Request failed: %s\n", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);

  unlockHttps();
  return NULL;
}

// returns a cJSON - the caller must use cJSON_Delete afterwards

cJSON *getMyShows(int offset, int limit) {

  printf("Todo getMySHows () allocate httpReply elsewhere\n");

  if (!httpReply)
    httpReply = heap_caps_malloc(HTTPREPLYLEN, MALLOC_CAP_SPIRAM);
  if (!authHeader)
    authHeader = heap_caps_malloc(AUTHHEADERLEN, MALLOC_CAP_SPIRAM);

  char url[200];
  sprintf(url, "https://api.spotify.com:443/v1/me/shows?limit=%d&offset=%d",
          limit, offset);

  lockHttps();

  esp_http_client_config_t config = {.url = url,
                                     .timeout_ms = 2000,
                                     .buffer_size_tx = 2048,
                                     .buffer_size = 10000,
                                     .crt_bundle_attach = esp_crt_bundle_attach,
                                     .event_handler = httpHandler};

  httpReplyLen = 0;

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_GET);

  snpfa(authHeader, AUTHHEADERLEN, "Bearer %s", getAccessToken());
    
  esp_http_client_set_header(client, "Authorization", authHeader);

  esp_http_client_set_header(client, "User-agent", "okhttp/4.11.0");

  esp_http_client_set_header(client, "Host", "api.spotify.com");

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    printf("getMyShows Status = %d, content_length = %lld\n",
           esp_http_client_get_status_code(client),
           esp_http_client_get_content_length(client));

    cJSON *response = cJSON_Parse((char *)httpReply);
    if (response) {
      esp_http_client_cleanup(client);
      unlockHttps();
      return response;
    }
    cJSON_Delete(response);

  } else {
    printf("getMyShows Request failed: %s\n", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  unlockHttps();
  return NULL;
}

