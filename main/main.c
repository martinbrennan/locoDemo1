/********************************************************
	main.c
	
	app_main is the entry point
	setup initialises the hardware and starts various services including loco
	loop contains code in the main loop called every 100ms
		doUI is used to handle events - typically keys - UI and TFT code is in ui.c
		doCli is used for debug - and handles commands typed on the ESP-IDF monitor - could be used for headless operation
		sdPoll handles SD card detection and mounting
		doPostStart does work for the web UI - web server cannot do much inside the uri handler
	
	
*********************************************************/


#define DISPLAYENABLE 1

#include <loco.h>
#include "locoBoard.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include <ctype.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

void memDebug();

const int ledPin = 11;  // GPIO 11

void initLed() {
  // zero-initialize the config structure.
  gpio_config_t io_conf = {};
  // disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  // bit mask of the pins that you want to set, GPIO20
  io_conf.pin_bit_mask = 1ULL << ledPin;
  // disable pull-down mode
  io_conf.pull_down_en = 0;
  // disable pull-up mode
  io_conf.pull_up_en = 0;
  // configure GPIO with the given settings
  gpio_config(&io_conf);
  gpio_set_level(ledPin, 0);
}

void ledOn() { gpio_set_level(ledPin, 1); }

void ledOff() { gpio_set_level(ledPin, 0); }


char localIp[20];

	
char *getLocalIp() { return localIp; }

void memDebugB () {

  int l = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  int m = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  printf ("Largest block %d/%d\n", l, m);

}


cJSON *settingsJson = NULL;


void savePresets (){
	printf ("WARNING savePresets () NIY\n");
}	

void initSettings() {

  settingsJson = cJSON_CreateObject();

  char *vs = "50";

  cJSON *vol = cJSON_CreateString(vs);
  cJSON_AddItemToObject(settingsJson, "volume", vol);
}

int aFileExists(char *path) {
  FILE *tmp = fopen(path, "r");
  if (tmp) {
    fclose(tmp);
    return 1;
  }
  return 0;
}

int getSettings() {

  if (!aFileExists("/spiffs/Settings"))
    return 0;

  FILE *settingsFile = fopen("/spiffs/Settings", "r");
  if (!settingsFile)
    return 0;

  fseeko(settingsFile, 0, SEEK_END);
  int size = ftello(settingsFile);
  fseeko(settingsFile, 0, SEEK_SET);

  //  printf("Settings file length %d\n", size);
  char *buf = (char *) malloc(size + 1);
  if (!buf) {
    fclose(settingsFile);
    return 0;
  }
  memset(buf, 0, size + 1);
  fread(buf, 1, size, settingsFile);
  fclose(settingsFile);

  printf("getSettings = %s\n", buf);

  settingsJson = cJSON_Parse(buf);

  free(buf);
  return (int)settingsJson;
}

void saveSettings() {

  char *jsonString = cJSON_Print(settingsJson);

//  printf("saveSettings %s\n", jsonString);

  FILE *settingsFile = fopen("/spiffs/Settings", "wb");
  fprintf(settingsFile, "%s", jsonString);
  fclose(settingsFile);
  cJSON_free(jsonString);
}
	
int getSettingsVolume() {

  int r = 50;
  const cJSON *g = cJSON_GetObjectItemCaseSensitive(settingsJson, "volume");
  if (g && g->valuestring)
    sscanf(g->valuestring, "%d", &r);
  return r;
}

void setSettingsVolume(int volume) {

  char vs[10];
  sprintf(vs, "%d", volume);
  cJSON *newVol = cJSON_CreateString(vs);

  const cJSON *vj = cJSON_GetObjectItemCaseSensitive(settingsJson, "volume");

  if (vj)
    cJSON_ReplaceItemInObjectCaseSensitive(settingsJson, "volume", newVol);
  else
    cJSON_AddItemToObject(settingsJson, "volume", newVol);

  saveSettings();
}


void doText(unsigned char *s, int l) {

  int n;
  printf(" ");
  int m = l > 8 ? 8 : l;
  for (n = 0; n < m; n++) {
    if (isprint(*s))
      printf("%c", *s);
    else
      printf(".");
    s++;
  }
  printf("\n");
}

void cdump(unsigned char *buf, int len) {

  int n = 1;
  unsigned char *t = buf;
  unsigned tl = len;
  int a = 0;
  while (len--) {
    if (n % 8 == 1) {
      printf("%04X ", a);
      a += 8;
    }
    printf("%02x ", *buf++);
    if (!(n++ % 8)) {
      doText(t, tl);
      t = buf;
      tl -= 8;
    }
  }
  if ((n - 1) % 8)
    doText(t, tl);
}


void setup() {

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
  printf("CONFIG_MBEDTLS_CERTIFICATE_BUNDLE\n");
#endif

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  printf("NVS done %lld\n", millis());

  printf("Flashing LED setup\n");

  initLed();

  printf("about to call locoAudioInit()\n");

  locoAudioInit();

  printf("about to call startAudioThread()\n");

  startAudioThread();

  printf("startAudioThread() done\n");

  initSDDetect();

  initSpiffs();

  if (!getSettings()) {
    initSettings();
    saveSettings();
  }

  int vi = getSettingsVolume();
  setVolume(vi);

  memDebugB();

  initialiseWifi();

  initArtPipeline();

  memDebugB();

  initLoco((PFVN)locoCallback, "Beauty");

  printf("initButtons()\n");
  initButtons();

#if DISPLAYENABLE
  printf("lcdInit()\n");
  lcdInit();

  memDebugB();

  printf("uiInit()\n");
  uiInit();
#endif

  printf("Setup done\n");

  memDebugB();
}



char lbuf[100];
int lblen = 0;

int isSeparator(char c) {
  if (isspace(c))
	return 1;
  if (c == ',')
	return 1;
  return 0;
}

void parseCli(char *b, char **arg0, char **arg1, char **arg2) {

  *arg0 = "";
  *arg1 = "";
  *arg2 = "";

  while (*b && isSeparator(*b))
	b++; // skip leftover CRLF
  if (!*b)
	return;
  *arg0 = b;

  while (*b && !isSeparator(*b))
	b++; // skip arg0
  if (!*b)
	return;
  *b++ = 0; // terminate with a zero

  while (*b && isSeparator(*b))
	b++; // skip first separator
  if (!*b)
	return;
  *arg1 = b;

  while (*b && !isSeparator(*b))
	b++; // skip arg1
  if (!*b)
	return;
  *b++ = 0; // terminate with a zero

  while (*b && isSeparator(*b))
	b++; // skip second separator
  if (!*b)
	return;
  *arg2 = b;
}

void memDebug() {

	
  int l = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  printf("Heap size %d\n", l);

  l = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  printf("SPIRAM size %d\n", l);

  l = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  printf("Largest block %d\n", l);

  l = heap_caps_get_total_size(MALLOC_CAP_DMA);
  printf("DMA size %d\n", l);
  l = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  printf("Largest block %d\n", l);

  l = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  printf("SPIRAM size %d\n", l);
  l = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  printf("Largest block %d\n", l);

  l = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  printf("Internal size %d\n", l);
  l = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  int m = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  printf("Largest block %d/%d\n", l, m);  
  
//  printf ("PSRAM size %d\n",ESP.getPsramSize());
  
}



void exCli() {

  char *arg0, *arg1, *arg2;
  lbuf[lblen] = 0;

  parseCli(lbuf, &arg0, &arg1, &arg2);

  if (!strcasecmp(arg0, "mem")) {
    lmemDebug();
  }
  if (!strcasecmp(arg0, "stop")) {
    stopPlay();
  }
  if (!strcasecmp(arg0, "next")) {
    startNext();
  }
  if (!strcasecmp(arg0, "prev")) {
    startPrev();
  }
  if (!strcasecmp(arg0, "resume")) {
    doResume();
  }
  if (!strcasecmp(arg0, "pause")) {
    doPause();
  }
  if (!strcasecmp(arg0, "sd")) {
    printf("sdDetect = %d\n", getSDDetect());
  }
  if (!strcasecmp(arg0, "mount")) {
    mountSd(0);
  } else if (!strcasecmp(arg0, "tt")) {
    toggleTestTone();
  } else if (!strcasecmp(arg0, "back")) {
    int percent = 50;
    sscanf(arg1, "%d", &percent);
    set_backlight_brightness(percent);
  } else if (!strcasecmp(arg0, "verb")) {
    int level = 0;
    sscanf(arg1, "%d", &level);
    locoVerbosity(level);
  } else if (!strcasecmp(arg0, "verify")) {
    verify();
  } else if (!strcasecmp(arg0, "restart")) {
    reconnectTest(0);
  } else if (!strcasecmp(arg0, "deb")) {
    int n = 0;
    sscanf(arg1, "%d", &n);
    locoDebug(n);
  } else if (!strcasecmp(arg0, "ver")) {
    printf("getVersion () = %s\n", getVersion(0));
  } else if (!strcasecmp(arg0, "format")) {
//    format_spiffs();
  } else if (!strcasecmp(arg0, "web")) {
    if (server2) {
      stopWebserver();
      printf("stopped\n");
    } else {
      startWebserver();
      printf("started\n");
    }
  } else if (!strcasecmp(arg0, "ssr")) {

    int index;
    sscanf(arg1, "%d", &index);
    startSearchResult(index);
  } else if (!strcasecmp(arg0, "sock")) {
    stopWebsocket();
  } else if (!strcasecmp(arg0, "life")) {
    printfTokenLife();
  } else if (!strcasecmp(arg0, "newt")) {
    refreshTokenNow();
  } else if (!strcasecmp(arg0, "togw")) {
    toggleWebsocket();
  } else if (!strcasecmp(arg0, "codec")) {
    codecRestart(getSettingsVolume());
  } else if (!strcasecmp(arg0, "wifi")) {
    startWifiFromCredentials();
  } else if (!strcasecmp(arg0, "track")) {
    int track = 0;
    sscanf(arg1, "%d", &track);
	startTrackN (track);
  } else if (!strcasecmp(arg0, "status")) {
    printf ("isActive %d StateIsPlaying %d StateIsPaused %d\n",getIsActive(),getStateIsPlaying(),getStateIsPaused());
  } 
  

  lblen = 0;
}

int postStartSearchResultFlag = 0;
int postStartFavouriteFlag = 0;
int postStartPlaylistFlag = 0;
int postNextFlag = 0;
int postPrevFlag = 0;
int postPlayPauseFlag = 0;

void postStartSearchResult (int index){
	postStartSearchResultFlag = index+1;
}

void postStartFavourite (int index){
	postStartFavouriteFlag = index+1;
}

void postNext (){
	postNextFlag = 1;
}

void postPrev (){
	postPrevFlag = 1;
}

void postPlayPause (){
	postPlayPauseFlag = 1;
}

void postStartPlaylist (int index){
	postStartPlaylistFlag = index+1;
}


void doPostStart() {
  if (postStartSearchResultFlag) {
    startSearchResult(postStartSearchResultFlag - 1);
    postStartSearchResultFlag = 0;
  }
  if (postStartFavouriteFlag) {
    playFavourite(postStartFavouriteFlag);
    postStartFavouriteFlag = 0;
  }
  if (postNextFlag) {
    postNextFlag = 0;
    if (isSpotifySource() && getIsActive()) {
      startNext();
    }
  }
  if (postPrevFlag) {
    postPrevFlag = 0;
    if (isSpotifySource() && getIsActive()) {
      startPrev();
    }
  }
  if (postPlayPauseFlag) {
    postPlayPauseFlag = 0;
    if (isSpotifySource() && getIsActive()) {
      playPause();
    }
    else if (isRadioSource()) {
      if (isRadioPlaying())
        stopPlay();
      else
		restartCurrentRadio();
    }    
  }
  if (postStartPlaylistFlag) {    
    char *uri = getPlaylistUri(postStartPlaylistFlag-1);
	playUri (uri);
    postStartPlaylistFlag = 0;
  }
}




void doCli() {

  int c = getchar();
  if (c != -1) {
	putchar(c);
	if (c == '\n')
  	putchar('\n');
	//      	printf ("%x\n",c);
  if (c == '\n')
  	exCli();
	else
  	lbuf[lblen++] = c;
	if (lblen >= 100)
  	lblen = 0;
  }
}


int n = 0;

void loop() {
  int c;
  int e;

  if (getStateIsPlaying() && !getStateIsPaused()) {
    if (n & 0x1)			// fast
      ledOn();
    else
      ledOff();
  } else {
    if ((n & 0xF) == 0xF)	// slow
      ledOn();
    else
      ledOff();
  }

#if DISPLAYENABLE
  while ((e = getEvent())) // while allows multiple volume changes per main loop
    doUI(e);
#endif

  doCli();
  sdPoll();
  doPostStart();

  n = n + 1;
}


void app_main () {
	setup ();
	while (1){
		loop();
		vTaskDelay (pdMS_TO_TICKS(100));
	}
}


		
