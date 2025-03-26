/********************************************************

web.c

This module provides the web interface to loco

startWebserver and stopWebserver are the main functions
startWebserver registers a number of handlers each corresponding
to a different http request
The webserver serves the html page in locopage.cpp
in response to a request for / or /index.html

The web Ui has been used for development so web.c
supports endpoints reconnect, blob and connect that will be deprecated


*********************************************************/

#include "cJSON.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <esp_event.h>
#include <esp_heap_caps.h>
#include <esp_http_client.h>
#include <esp_http_server.h>
#include <esp_https_ota.h>
#include <esp_netif_sntp.h>
#include <esp_ota_ops.h>
#include <esp_sntp.h>
#include <esp_tls_crypto.h>
#include <esp_websocket_client.h>
#include <esp_wifi.h>
#include <mbedtls/base64.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "locoBoard.h"
#include <loco.h>

extern const char *mainPage;
extern cJSON *presetsJson;

httpd_handle_t server = NULL;
httpd_handle_t server2 = NULL;
static const char *TAG = "web.c";

char postBufferW[1000];

int min(int a, int b) { return a > b ? b : a; }

esp_err_t mainHandler(httpd_req_t *req) {

  httpd_resp_send(req, mainPage, HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriIndex = {.uri = "/index.html",
                        .method = HTTP_GET,
                        .handler = mainHandler,
                        .user_ctx = NULL};

httpd_uri_t uriMain = {
    .uri = "/", .method = HTTP_GET, .handler = mainHandler, .user_ctx = NULL};

esp_err_t notFoundHandler(httpd_req_t *req) {
  printf("No Handler for %s\n", req->uri);
  httpd_resp_send_404(req);
  return ESP_OK;
}

httpd_uri_t uri404 = {.uri = "/*",
                      .method = HTTP_GET,
                      .handler = notFoundHandler,
                      .user_ctx = NULL};

int hextoi(char c) {

  int r;

  if (isdigit(c))
    return c - '0';

  r = 10 + tolower(c) - 'a';

  if ((r < 16) && (r >= 10))
    return r;
  return 0;
}

// This variant doesnt stop at & (for search strings that could include &)

void stripPercentagesFromURLTail(char *d, char *s, int length) {

  int n;
  char c;

  *d = 0;
  for (n = 0; *s && (n < length - 1); n++) {
    if ((*s == '%') && isxdigit((int)s[1]) && isxdigit((int)s[2])) {
      s++;
      if (*s)
        c = 16 * hextoi(*s);
      else
        return;
      s++;
      if (*s)
        c += hextoi(*s);
      else
        return;
      *d++ = c;
      s++;
    } else
      *d++ = *s++;
    *d = 0;
  }
}

// len is length of value buffer

int getQueryParameter(const char *uri, char *key, char *value, int len) {

  *value = 0;
  char *s = strstr(uri, key);
  if (!s)
    return 0;
  while (*s && (*s != '='))
    s++; // skip the key
  if (*s)
    s++; // skip the equals
  char *d = value;
  len--;
  while (len && *s && (*s != '&')) {
    *d++ = *s++;
    len--;
  }
  *d = 0;
  return 1;
}


int isPlaying() {
  if (isSpotifySource())
    return getStateIsPlaying() && !getStateIsPaused();
  else if (isRadioSource())
    return isRadioPlaying();
  return 0;
}

// creates a cJSON object - caller must free

cJSON *getPlaylists() {

  int n;

  int pc = getMyPlaylistsCount();

  if (!pc)
    return NULL;

  //  printf("getPlaylists got count %d\n", pc);

  cJSON *playlists = cJSON_CreateArray();

  for (n = 0; n < pc; n++) {
    char *name = getLoadedPlaylistName(n);
    //    printf("got name %d %s\n", n, name);
    cJSON *playlist = cJSON_CreateString(name);

    cJSON_AddItemToArray(playlists, playlist);
  }
  return playlists;
}

esp_err_t getStatusHandler(httpd_req_t *req) {

  cJSON *status = cJSON_CreateObject();
  char version[30];
  sprintf(version, "%s %s", __DATE__, __TIME__);
  cJSON_AddStringToObject(status, "version", version);

  if (isSpotifySource()) {
    cJSON_AddStringToObject(status, "source", "Spotify");
    cJSON_AddStringToObject(status, "track", cleanName(getPlayingTrackName()));
    cJSON_AddStringToObject(status, "album", cleanName(getPlayingAlbumName()));
    cJSON_AddStringToObject(status, "artist",
                            cleanName(getPlayingArtistName()));
    cJSON_AddStringToObject(status, "art", getPlayingArtUrl());
  } else if (isRadioSource()) {
    cJSON_AddStringToObject(status, "source", "Radio");
    cJSON_AddStringToObject(status, "station", getCurrentStationName());
    cJSON_AddStringToObject(status, "art", getCurrentStationLogo());
  }

  cJSON_AddStringToObject(status, "playing", isPlaying() ? "true" : "false");
  cJSON_AddStringToObject(status, "breadcrumbs", getBreadcrumbs());

  cJSON *fav = getFavourites();
  if (fav)
    cJSON_AddItemToObject(status, "favourites", fav);

  cJSON *plists = getPlaylists();
  if (plists)
    cJSON_AddItemToObject(status, "playlists", plists);

  char *jsonString = cJSON_Print(status);
  httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);

  if (fav)
    cJSON_DetachItemFromObject(status, "favourites");
  /*
    if (plists) {
      cJSON_DetachItemFromObject(status, "playlists");
      cJSON_Delete(plists);
    }
  */
  cJSON_Delete(status);
  cJSON_free(jsonString);

  return ESP_OK;
}

httpd_uri_t uriGetStatus = {.uri = "/getStatus",
                            .method = HTTP_GET,
                            .handler = getStatusHandler,
                            .user_ctx = NULL};

esp_err_t setPresetsHandler(httpd_req_t *req) {

  int size = min(req->content_len, sizeof(postBufferW) - 1);

  int r = httpd_req_recv(req, postBufferW, size);

  if (size != req->content_len)
    printf("setPresetsHandler too much data\n");

  postBufferW[size] = 0;

  cJSON *new = cJSON_Parse(postBufferW);

  if ((r > 0) && new) {
    cJSON_Delete(presetsJson);
    presetsJson = new;
    savePresets();
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  } else
    httpd_resp_send(req, "Failed", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t playDirectHandler(httpd_req_t *req) {

  printf("playDirectHandler ()\n");

  int size = min(req->content_len, sizeof(postBufferW) - 1);

  int r = httpd_req_recv(req, postBufferW, size);

  if (size != req->content_len)
    printf("playDirectHandler too much data\n");

  postBufferW[size] = 0;

  cJSON *station = cJSON_Parse(postBufferW);

  if ((r > 0) && station) {

    cJSON *name = cJSON_GetObjectItemCaseSensitive(station, "name");
    cJSON *url = cJSON_GetObjectItemCaseSensitive(station, "url");

    if (name && url)
      printf("playDirectHandler ()\nName=%s\nurl=%s\n", name->valuestring,
             url->valuestring);

    startDirect(name->valuestring, url->valuestring, NULL);

    cJSON_Delete(station);

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  } else
    httpd_resp_send(req, "Failed", HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

esp_err_t vTunerSearchHandler(httpd_req_t *req) {

  int size = min(req->content_len, sizeof(postBufferW) - 1);

  int r = httpd_req_recv(req, postBufferW, size);

  if (size != req->content_len)
    printf("vTunerSearchHandler too much data\n");

  postBufferW[size] = 0;

  cJSON *new = cJSON_Parse(postBufferW);

  printf("vTunerSearchHandler got %s\n", postBufferW);

  // assume its like "jenny"

  char *text = postBufferW + 1; // strip quotes
  char *s = text;
  while (*s && *s != '\"')
    s++;
  if (*s)
    *s = 0;

  if ((r > 0) && new) {
    cJSON *stations = vTunerSearch(text);
    if (stations) {
      char *jsonString = cJSON_Print(stations);
      httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);
      free(jsonString);
    } else
      httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);
  } else
    httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t vTunerTopHandler(httpd_req_t *req) {

  printf("vTunerTopHandler\n");

  cJSON *stations = vTunerTop();
  if (stations) {
    char *jsonString = cJSON_Print(stations);
    httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);
    free(jsonString);
    cJSON_Delete(stations);
  } else
    httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t vTunerItemHandler(httpd_req_t *req) {

  char query[50];
  char index[50];
  int i = 0;

  httpd_req_get_url_query_str(req, query, 50);
  httpd_query_key_value(query, "index", index, 40);

  printf("vTunerItemHandler index = %s\n", index);
  sscanf(index, "%d", &i);

  cJSON *stations = vTunerItem(i);
  if (stations) {
    char *jsonString = cJSON_Print(stations);
    httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);
    free(jsonString);
    cJSON_Delete(stations);
  } else
    httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t vTunerBackHandler(httpd_req_t *req) {

  printf("vTunerBackHandler \n");

  cJSON *stations = vTunerBack();
  if (stations) {
    char *jsonString = cJSON_Print(stations);
    httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);
    free(jsonString);
    cJSON_Delete(stations);
  } else
    httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t playResultHandler(httpd_req_t *req) {

  char query[50];
  char station[50];
  int sn;

  httpd_req_get_url_query_str(req, query, 50);
  httpd_query_key_value(query, "station", station, 40);

  printf("playResult station = %s\n", station);
  sscanf(station, "%d", &sn);

  postStartSearchResult(sn);

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t playFavouriteHandler(httpd_req_t *req) {

  char query[50];
  char station[50];
  int sn;

  httpd_req_get_url_query_str(req, query, 50);
  httpd_query_key_value(query, "station", station, 40);

  printf("playFavourite station = %s\n", station);
  sscanf(station, "%d", &sn);

  postStartFavourite(sn);

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

esp_err_t playPlaylistHandler(httpd_req_t *req) {

  char query[50];
  char index[50];
  int i;

  httpd_req_get_url_query_str(req, query, 50);
  httpd_query_key_value(query, "index", index, 40);

  printf("playPlaylist index = %s\n", index);
  sscanf(index, "%d", &i);

  //  startSearchResult(sn);
  postStartPlaylist(i);

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriSetPresets = {.uri = "/setPresets",
                             .method = HTTP_POST,
                             .handler = setPresetsHandler,
                             .user_ctx = NULL};

httpd_uri_t uriPlayDirect = {.uri = "/playDirect",
                             .method = HTTP_POST,
                             .handler = playDirectHandler,
                             .user_ctx = NULL};

httpd_uri_t urivTunerSearch = {.uri = "/vTunerSearch",
                               .method = HTTP_POST,
                               .handler = vTunerSearchHandler,
                               .user_ctx = NULL};

httpd_uri_t urivTunerTop = {.uri = "/vTunerTop",
                            .method = HTTP_GET,
                            .handler = vTunerTopHandler,
                            .user_ctx = NULL};

httpd_uri_t urivTunerItem = {.uri = "/vTunerItem",
                             .method = HTTP_GET,
                             .handler = vTunerItemHandler,
                             .user_ctx = NULL};

httpd_uri_t urivTunerBack = {.uri = "/vTunerBack",
                             .method = HTTP_GET,
                             .handler = vTunerBackHandler,
                             .user_ctx = NULL};

httpd_uri_t uriPlayResult = {.uri = "/playResult",
                             .method = HTTP_GET,
                             .handler = playResultHandler,
                             .user_ctx = NULL};

httpd_uri_t uriPlayFavourite = {.uri = "/playFavourite",
                                .method = HTTP_GET,
                                .handler = playFavouriteHandler,
                                .user_ctx = NULL};

httpd_uri_t uriPlayPlaylist = {.uri = "/playPlaylist",
                               .method = HTTP_GET,
                               .handler = playPlaylistHandler,
                               .user_ctx = NULL};


esp_err_t stopHandler(httpd_req_t *req) {

  printf("stopHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  stopPlay();

  return ESP_OK;
}

httpd_uri_t uriStop = {.uri = "/stop",
                       .method = HTTP_GET,
                       .handler = stopHandler,
                       .user_ctx = NULL};

// WARNING it may be better to create a next/prev/pause event
// and let UI take care of this

esp_err_t nextHandler(httpd_req_t *req) {

  printf("nextHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  postNext();

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriNext = {.uri = "/next",
                       .method = HTTP_GET,
                       .handler = nextHandler,
                       .user_ctx = NULL};

esp_err_t prevHandler(httpd_req_t *req) {

  printf("prevHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  postPrev();

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriPrev = {.uri = "/prev",
                       .method = HTTP_GET,
                       .handler = prevHandler,
                       .user_ctx = NULL};

esp_err_t pauseHandler(httpd_req_t *req) {

  printf("pauseHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  postPlayPause();

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriPause = {.uri = "/pause",
                        .method = HTTP_GET,
                        .handler = pauseHandler,
                        .user_ctx = NULL};


esp_err_t reconnectHandler(httpd_req_t *req) {

  printf("reconnectHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  reconnectTest(0);

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriReconnect = {.uri = "/reconnect",
                            .method = HTTP_GET,
                            .handler = reconnectHandler,
                            .user_ctx = NULL};

esp_err_t blobHandler(httpd_req_t *req) {

  printf("blobHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  // WARNING should check if connected

  reAuth();

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriBlob = {.uri = "/blob",
                       .method = HTTP_GET,
                       .handler = blobHandler,
                       .user_ctx = NULL};

esp_err_t disconnectHandler(httpd_req_t *req) {

  printf("disconnectHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  doReset();

  keepAwake();

  return ESP_OK;
}

httpd_uri_t uriDisconnect = {.uri = "/disconnect",
                             .method = HTTP_GET,
                             .handler = disconnectHandler,
                             .user_ctx = NULL};

esp_err_t speedHandler(httpd_req_t *req) {

  char *buf = malloc(1000);
  memset(buf, 'A', 1000);

  httpd_resp_set_type(req, "text/plain");

  for (int n = 0; n < 1000; n++)
    httpd_resp_send_chunk(req, buf, 1000);

  httpd_resp_send_chunk(req, NULL, 0);

  keepAwake();

  free(buf);
  return ESP_OK;
}

httpd_uri_t uriSpeed = {.uri = "/speed",
                        .method = HTTP_GET,
                        .handler = speedHandler,
                        .user_ctx = NULL};

esp_err_t initPlaylistsHandler(httpd_req_t *req) {

  printf("initPlaylistsHandler ()\n");

  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);

  addEvent(INITPLAYLISTS);

  return ESP_OK;
}

httpd_uri_t initPlaylists = {.uri = "/initPlaylists",
                             .method = HTTP_GET,
                             .handler = initPlaylistsHandler,
                             .user_ctx = NULL};

void printTime() {

  struct timeval tvNow;

  gettimeofday(&tvNow, NULL);

  uint64_t t = (uint64_t)tvNow.tv_sec * 1000L + (uint64_t)tvNow.tv_usec / 1000;

  printf("printTime %lld\n", t);

  int r = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(3000));

  printf("esp_netif_sntp_sync_wait result %d %s\n", r, esp_err_to_name(r));
}

char *printBuffer;

static httpd_handle_t start_webserver2(void) {
  printf("start_webserver2 ()\n");
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;
  config.max_uri_handlers = 23;
  config.ctrl_port = 40000; // not sure what this does

  config.uri_match_fn = httpd_uri_match_wildcard;

  config.max_open_sockets = 4;

  // Start the httpd server

  printf("Starting server on port: %d\n", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &uriGetStatus);

    httpd_register_uri_handler(server, &urivTunerSearch);
    httpd_register_uri_handler(server, &urivTunerTop);
    httpd_register_uri_handler(server, &urivTunerItem);
    httpd_register_uri_handler(server, &urivTunerBack);
    httpd_register_uri_handler(server, &uriPlayResult);
    httpd_register_uri_handler(server, &uriPlayFavourite);
    httpd_register_uri_handler(server, &uriPlayPlaylist);

    httpd_register_uri_handler(server, &uriSpeed);

    httpd_register_uri_handler(server, &uriIndex);
    httpd_register_uri_handler(server, &uriMain);

    httpd_register_uri_handler(server, &uriStop);
    httpd_register_uri_handler(server, &uriNext);
    httpd_register_uri_handler(server, &uriPrev);
    httpd_register_uri_handler(server, &uriPause);

    httpd_register_uri_handler(server, &uriReconnect);
    httpd_register_uri_handler(server, &uriBlob);
    httpd_register_uri_handler(server, &uriDisconnect);

    httpd_register_uri_handler(server, &initPlaylists);

    httpd_register_uri_handler(server, &uri404);

    return server;
  }

  ESP_LOGI(TAG, "Error starting server2!");
  return NULL;
}

void stopWebserver() {
  unsigned long t = millis();
  printf("stopWebserver ()\n");
  httpd_stop(server2);
  printf("stopWebserver done after %lld\n", millis() - t);
  server2 = NULL;
}

void startWebserver() {
  unsigned long t = millis();
  printf("startWebserver ()\n");
  server2 = start_webserver2();
  printf("startWebserver result %d after %lld\n", (int)server2, millis() - t);
}
