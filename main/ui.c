/********************************************************
	ui.c
	
	This module provides the user interface for the loco demo program
	It handles user input and other events using an event queue
	It displays whats playing and menus on the TFT by assembling lvgl screens
	It will help to have a little understanding of LVGL to make sense of it
	The current state of the UI is uiState. This usually corresponds to a menu
	UiState directs events to a handler for that state
	The idle state is used to display what is currently playing
	This module also contains code for Wifi because wifi setup is part of the UI
	
	uiInit - initialises the display and UI 
	initialiseWifi - initialises Wifi - may use credentials stored in spiffs
	doUI - handles events from the event Queue
	
	
*********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <locale.h>
#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "time.h"

#include "lvgl.h"
#include <loco.h>
#include "locoBoard.h"


char *getLocalIp ();
int fileExists(char *path);
void waitlock_init (waitlock_t *wl);
int loadFavourites();
void displayFavourites ();

int stress = 0;
int artToggle = 0;

#define LONGMODE LV_LABEL_LONG_DOT

char ssidBuffer[33];
char passwordBuffer[70];
uint8_t bssidBuffer[6];
int reconnectFlag = 0;
int scanning = 0;
int scanComplete = 0;
int connected = 0;
int newIpFlag = 0;
int wifiDisconnectedFlag = 0;
int wifiDisconnectWhileActive = 0;
int wifiDisconnectWhilePlaying = 0;
int newCredentials = 0;
cJSON *wifiCredentialsJson = NULL;
int locoStarted = 0;

// Timezone and NTP server
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // Adjust for your timezone
const int daylightOffset_sec = 0; // Adjust for daylight savings if applicable

int loadWifiCredentials();


void setLocalIp(unsigned long ipAddr) {

  sprintf(getLocalIp(), "%d.%d.%d.%d", (int)ipAddr & 0xFF, (int)(ipAddr >> 8) & 0xFF,
          (int)(ipAddr >> 16) & 0xFF, (int)(ipAddr >> 24) & 0xFF);
}

static void wifiEventHandler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {

	printf ("wifiEventHandler\n");

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
	
	if (connected){			 
		connected = 0;
		getLocalIp()[0] = 0;
		wifiDisconnectedFlag = 1;
		printf ("\nWIFI DISCONNECTED\n\n");
	}
	else printf ("Superfluous WIFI_EVENT_STA_DISCONNECTED event\n");	
    
    esp_wifi_connect();
    

    
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    setLocalIp((unsigned long)event->ip_info.ip.addr);

    printf ("WIFI CONNECTED localIp = %s\n", getLocalIp());
    
    connected = 1;
    newIpFlag = 1;
    refreshUI();

  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
    printf ("Scan Complete\n");
    wifi_event_sta_scan_done_t *r = (wifi_event_sta_scan_done_t *) event_data;
    printf ("status %ld number %d\n",r->status,r->number);
    scanComplete = 1;
    scanning = 0;
    refreshUI();
  }
}

// start Wifi from ssidBuffer & passwordBuffer
// using esp_wifi_set_ps() and set a timer so that it reverts to lower power
// after a while

wifi_config_t wifiConfig = {
    .sta =
        {
            .listen_interval = 6,
            .channel = 2,
        },
};

void startWifiSta() {
  printf ("startWIfi () %s %s\n", ssidBuffer, passwordBuffer);
  esp_wifi_stop();
  esp_wifi_set_mode(WIFI_MODE_STA);
  memset(wifiConfig.sta.ssid, 0, sizeof(wifiConfig.sta.ssid));
  memset(wifiConfig.sta.password, 0, sizeof(wifiConfig.sta.password));
  strcpy((char *)wifiConfig.sta.ssid, ssidBuffer);
  strcpy((char *)wifiConfig.sta.password, passwordBuffer);
  esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
  esp_wifi_start();
}

void startWifiSta2() {
  printf("startWIfiSta2 () %s %s\n", ssidBuffer, passwordBuffer);
  uint8_t *mac = bssidBuffer;
  printf("BSSID %02x,%02x,%02x,%02x,%02x,%02x\n", mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);
  esp_wifi_stop();
  esp_wifi_set_mode(WIFI_MODE_STA);
  memset(wifiConfig.sta.ssid, 0, sizeof(wifiConfig.sta.ssid));
  memset(wifiConfig.sta.password, 0, sizeof(wifiConfig.sta.password));
  strcpy((char *)wifiConfig.sta.ssid, ssidBuffer);
  strcpy((char *)wifiConfig.sta.password, passwordBuffer);
  memcpy(wifiConfig.sta.bssid, bssidBuffer, 6);
  wifiConfig.sta.bssid_set = true;
  esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
  esp_wifi_start();
}


void wifi_init_softap(void) {

  wifi_config_t dummy_config = {};

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = "BrennanVB",
              .ssid_len = strlen("BrennanVB"),
              .channel = 6,
              .password = "brennanvb",
              .max_connection = 4,
              .authmode = WIFI_AUTH_WPA_WPA2_PSK,
              .pmf_cfg =
                  {
                      .required = false,
                  },
          },
  };
  if (strlen("brennanvb") == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  //    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(
      esp_wifi_set_mode(WIFI_MODE_APSTA)); // USE APSTA or Scan does not work
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &dummy_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  printf ("wifi_init_softap finished. SSID:%s password:%s channel:%d",
           "BrennanVB", "brennanvb", 6);
}

void startWifiAp() {

  printf ("startWIfiAp () redirected to wifi_init_softap()\n");
  esp_wifi_stop();
//  stopWebserver();
  wifi_init_softap();
  return;
}  



void initialiseWifi() {

  getLocalIp()[0] = 0;

  /*init wifi as sta and set power save mode*/
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
  assert(ap_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifiEventHandler, NULL, NULL));

  if (loadWifiCredentials() && wifiCredentialsJson) {

    const cJSON *ssid;
    ssid = cJSON_GetObjectItemCaseSensitive(wifiCredentialsJson, "ssid");
    strcpy(ssidBuffer, ssid->valuestring);
    const cJSON *password;
    password =
        cJSON_GetObjectItemCaseSensitive(wifiCredentialsJson, "password");
    strcpy(passwordBuffer, password->valuestring);

    const cJSON *bssid;
    bssid = cJSON_GetObjectItemCaseSensitive(wifiCredentialsJson, "bssid");
    if (!bssid)
      return;
    int n;
    for (n = 0; n < 6; n++) {
      cJSON *item = cJSON_GetArrayItem(bssid, n);
      bssidBuffer[n] = (uint8_t)cJSON_GetNumberValue(item);
    }
    startWifiSta2();
    
  } else
    startWifiAp();
}

#define DEFAULT_SCAN_LIST_SIZE 20


uint16_t apCount = 0;
//wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
wifi_ap_record_t *ap_info;

static void parseWifiResults(int echo) {


	apCount = DEFAULT_SCAN_LIST_SIZE;
//  memset(ap_info, 0, sizeof(ap_info));
  memset(ap_info, 0,DEFAULT_SCAN_LIST_SIZE*sizeof(wifi_ap_record_t));
 

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_info));
  
  printf ("Number = %d\n", apCount);

  for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < apCount); i++) {
      printf ("SSID %s (%d)\n", ap_info[i].ssid, ap_info[i].rssi);
  }
 
}


int getSSIDCount() {
  if (apCount) return apCount;
  return 1;
}

char SSIDNameBuf [40];

char *getSSIDTitle(int index) {

  if (apCount) {
    sprintf(SSIDNameBuf, "%s (%d)", ap_info[index].ssid, ap_info[index].rssi);
    return SSIDNameBuf;

  } else
    return "Scanning";
}

char *getSSIDName(int index) {

  if (apCount) {
    return (char *)ap_info[index].ssid;
  } else
    return "";
}

char *getBSSID(int index) {

  if (apCount) {

    return (char *)ap_info[index].bssid;    
    
  } else
    return NULL;
}


// WARNING NIY
		

int isWifiConnected (){
	return 1;
}


int isScanning() { return scanning; }

void startWifiScan (){

	printf ("WARNING startWifiScan () must check if connecting\n");

  int r = esp_wifi_scan_start(NULL, false);
  printf ("esp_wifi_scan_start result %d\n",r);

  scanning = 1;
  scanComplete = 0;
  apCount = 0;
}


void reconnect(char *ssid, char *password, uint8_t *bssid) {
  strcpy(ssidBuffer, ssid);
  strcpy(passwordBuffer, password);
  memcpy (bssidBuffer, bssid, 6);
  reconnectFlag = 1;
  refreshUI ();
}


void saveWifiCredentials() {

  if (wifiCredentialsJson)
    cJSON_Delete(wifiCredentialsJson);
  wifiCredentialsJson = cJSON_CreateObject();
  cJSON_AddStringToObject(wifiCredentialsJson, "ssid", ssidBuffer);
  cJSON_AddStringToObject(wifiCredentialsJson, "password", passwordBuffer);

  cJSON *mac = cJSON_CreateArray();
  int n;
  for (n = 0; n < 6; n++) {
    cJSON_AddItemToArray(mac, cJSON_CreateNumber(bssidBuffer[n]));
  }
  cJSON_AddItemToObject(wifiCredentialsJson, "bssid", mac);

  char *jsonString = cJSON_Print(wifiCredentialsJson);

  printf ("wifiCredentials %s\n", jsonString);

  FILE *settingsFile = fopen("/spiffs/wifiCredentials", "wb");
  fprintf(settingsFile, "%s", jsonString);
  fclose(settingsFile);
  cJSON_free(jsonString);
}


int loadWifiCredentials() {
	printf ("loadWifiCredentials ()\n");
  char *name = "/spiffs/wifiCredentials";
  if (!fileExists(name))
    return 0;
  FILE *iFile = fopen(name, "r");
  if (!iFile)
    return 0;

	printf ("loadWifiCredentials () got file\n");

  fseeko(iFile, 0, SEEK_END);
  int size = ftello(iFile);
  fseeko(iFile, 0, SEEK_SET);

  char *buf = malloc(size + 1);
  if (!buf) {
    fclose(iFile);
    return 0;
  }
  memset(buf, 0, size + 1);
  fread(buf, 1, size, iFile);
  fclose(iFile);

    printf ("loadWifiCredentials = %s\n", buf);

  if (wifiCredentialsJson)
    cJSON_Delete(wifiCredentialsJson);

  wifiCredentialsJson = cJSON_Parse(buf);

  free(buf);
  
  printf ("loadWifiCredentials () wifiCredentialsJson = %d\n",(int)wifiCredentialsJson);
  
  return (int)wifiCredentialsJson;
}


int startWifiFromCredentials() {

  if (!loadWifiCredentials())
    return 0;

  if (!wifiCredentialsJson)
    return 0;
    
  const cJSON *ssid;
  ssid = cJSON_GetObjectItemCaseSensitive(wifiCredentialsJson, "ssid");
  strcpy(ssidBuffer, ssid->valuestring);
  const cJSON *password;
  password = cJSON_GetObjectItemCaseSensitive(wifiCredentialsJson, "password");
  strcpy(passwordBuffer, password->valuestring);
  
  const cJSON *bssid;
  bssid = cJSON_GetObjectItemCaseSensitive(wifiCredentialsJson, "bssid");
  if (!bssid)
    return 0;
  int n;
  for (n = 0; n < 6; n++) {
    cJSON *item = cJSON_GetArrayItem(bssid, n);
    bssidBuffer[n] = (uint8_t)cJSON_GetNumberValue(item);
  }
  startWifiSta2();
  return 1;
}

void locoCallback(int e) {
//  printf("locoCallback () got event %d\n", e);
  if (e == AUTHORISED) {

    printf("New Credentials %s\n", getCredentials());
    FILE *cFile = fopen("/spiffs/credentials", "wb");
    fprintf(cFile, "%s", getCredentials());
    fclose(cFile);

    printf("New User Name %s\n", getUsername());
    cFile = fopen("/spiffs/username", "wb");
    fprintf(cFile, "%s", getUsername());
    fclose(cFile);

    printf("Saved\n");
  }
  else if (e == NEWART){
		newArt ();
  }	  
  else if (e == REFRESH){
	  refreshUI ();
	}
}


#define STATETRACKURILEN 50
#define MAXPLAYLISTS 50
#define MAXNAMELEN 100
int myPlaylistsCount = 0;
int myPlaylistsTotal = 0;

char (*myPlaylistUri)[STATETRACKURILEN];
char (*myPlaylistName)[MAXNAMELEN];	

#define MAXSHOWS 50
int myShowsCount = 0;
int myShowsTotal = 0;

char (*myShowUri)[STATETRACKURILEN];
char (*myShowName)[MAXNAMELEN];
	

#define MENUHEIGHT 120
#define MENUPITCH 40
#define MENUYPOS 25
#define MENUITEMSFULLYVISIBLE 3
#define MENUITEMSTODISPLAY 4

#define LETTERSPERROW 10
#define LETTERWIDTH 32

#define MENUITEMY 7
#define MENUITEMX 7

#define ARTSIZE 120

#define METAMARGIN 30

int idleHandler (int e);
int mainMenuHandler (int e);
int myPlaylistsHandler (int e);
void gotoMyPlaylists ();	
void displayMyPlaylists ();
int myShowsHandler (int e);
void gotoMyShows ();	
void displayMyShows ();

void displayPickSSID();
int pickSSIDHandlerU (int e);
void gotoPickSSID ();

void displaySetPassword();
int setPasswordHandler (int e);
void gotoSetPassword ();

void gotoConnecting ();
int connectingHandler (int e);

void gotoLogin (PFV cb);
int loginHandler (int e);

void gotoFavourites ();	
int favouritesHandler (int e);

#define UIIDLE 0
#define MAINMENU 1
#define MYPLAYLISTS 2
#define MYSHOWS 3
#define PICKSSID 4
#define SETPASSWORD 5
#define CONNECTING 6
#define LOGIN 7
#define FAVOURITES 8

int unusedHandler (){return 1;}
	

PFI uiHandlers [] = {
	idleHandler,
	mainMenuHandler,
	myPlaylistsHandler,
	myShowsHandler,
	pickSSIDHandlerU,
	setPasswordHandler,
	connectingHandler,	
	loginHandler,
	favouritesHandler,	
};


int alertTimer = 0;


lv_obj_t *menuScreen;
lv_obj_t *itemBox[MENUITEMSTODISPLAY];
lv_obj_t *itemText[MENUITEMSTODISPLAY];

lv_obj_t *alphaScreen;
lv_obj_t *letterBox[MENUITEMSFULLYVISIBLE*LETTERSPERROW];
lv_obj_t *letterText[MENUITEMSFULLYVISIBLE*LETTERSPERROW];

// these variables implement the UI event queue

#define UIQLEN 10

waitlock_t uiLock;
int uiQ[UIQLEN];
int uiQLen;				// number of events in queue
int uiQIn;				// next insertion
int uiQOut;				// next removal


#define KNOWNUTFS 7

char *knownUTF8 [KNOWNUTFS] = {
	"\xe2\x80\x99\x00",							// apostrophe
	"\xc3\xa9",									// e acute
	"\xc3\xa8",									// e grave
	"\xc3\xa1",									// a acute
	"\xc3\xa0",									// a grave
	"\xe2\x80\x9c",								// left double quotes
	"\xe2\x80\x9d"								// right double quotes
};
char asciiEquivalent [KNOWNUTFS] = {
	'\'',										// apostrophe
	'e',										// e acute
	'e',										// e grave
	'a',										// a acute
	'a',										// a grave
	'\"',										// left double quotes
	'\"'										// right double quotes
};


// return the number of consumed characters
// replacement character put at d

int isKnown (char *s, char *d){
	
	int n;
	char *k;
	int l;
	
	for (n=0;n<KNOWNUTFS;n++){
		k = knownUTF8[n];
		l = strlen (k);
		if (strncmp (s,k,l)==0){
			*d = asciiEquivalent[n];
			return l;
		}
	}
    return 0;
}    			
	

void fixApos (char *s){

	int l;
	char *d = s;
	
	while (*s){
		if ((l = isKnown (s,d))){
						
			s += l;
			d++;
		}
		else *d++ = *s++;
	}
	*d = 0;
}

// This converts known utf8 strings to ascii equivalent
// and unknown utf8 strings to spaces

void fixName (char *s){
	fixApos (s);
	char *d = s;
	while (*s){
		if (*s & 0x80){
			while (*s & 0x80) s++;
			*d++ = ' ';
		}
		else *d++ = *s++;
	}
	*d = 0;		 
}

char cleanBuf[MAXNAME];

// cleanName copies the text to cleanBuf so the utf8 cleaning does not corrupt the original
// (so it can still be used on the web UI)
// cleanBuf will be overwritten in due course  so it should be used immediately

char *cleanName (char *s){
	strncpy (cleanBuf,s,MAXNAME);
	fixName (cleanBuf);
	return cleanBuf;
}

lv_style_t menuTitleStyle;	
lv_style_t leftStatusStyle;
lv_style_t rightStatusStyle;
lv_style_t debugStyle;
lv_style_t menuStyle;	
lv_style_t menuSelectedStyle;
lv_style_t menuDeselectedStyle;
lv_style_t menuItemStyle;
lv_style_t popUpStyle;
lv_color_t red;
lv_color_t white;
lv_color_t black;
lv_color_t yellow;
lv_color_t deselected;
lv_color_t popbg;
lv_obj_t *idleTitle;
lv_obj_t *menuTitle;
lv_obj_t *menuTR;
lv_obj_t *idleZone;
lv_obj_t *menuLeftStatus;
lv_obj_t *menuRightStatus;
lv_obj_t *menuRoller; 
lv_obj_t *trackName;
lv_style_t trackNameStyle;
lv_obj_t *logoName;
lv_style_t logoNameStyle;
lv_obj_t *radioName;
lv_style_t radioNameStyle;
lv_obj_t *metadataBox;
lv_style_t metadataBoxStyle;
lv_obj_t *artistName;
lv_style_t artistNameStyle;
lv_obj_t *albumName;
lv_style_t albumNameStyle;
lv_obj_t *alphaTitle;
lv_style_t zoneStyle;
lv_style_t menuTRStyle;
lv_style_t confirmFlexStyle;
lv_style_t confirmStyle1;
lv_style_t confirmStyle2;
lv_obj_t *alphaHelp;

void initStyles (){
	
	red = lv_color_make (0xFF, 0x00, 0x00);	
	black = lv_color_make (0x00, 0x00, 0x00);	
	white = lv_color_make (0xFF, 0xFF, 0xFF);
	yellow = lv_color_make (0xFF, 0xFF, 0x00);
	deselected = lv_color_make (0x80, 0x80, 0x80);	
	popbg = lv_color_make (0x30, 0x30, 0x30);

	lv_style_init (&menuTitleStyle);		
	lv_style_set_text_color (&menuTitleStyle, white);
	lv_style_set_align (&menuTitleStyle, LV_ALIGN_TOP_LEFT);

	lv_style_init (&leftStatusStyle);		
	lv_style_set_text_color (&leftStatusStyle, white);
	lv_style_set_align (&leftStatusStyle, LV_ALIGN_BOTTOM_LEFT);

	lv_style_init (&rightStatusStyle);		
	lv_style_set_text_color (&rightStatusStyle, white);
	lv_style_set_align (&rightStatusStyle, LV_ALIGN_BOTTOM_RIGHT);

	lv_style_init (&trackNameStyle);		
	lv_style_set_text_color (&trackNameStyle, white);
	lv_style_set_align (&trackNameStyle, LV_ALIGN_BOTTOM_LEFT);

	lv_style_init (&logoNameStyle);		
	lv_style_set_text_color (&logoNameStyle, deselected);
    lv_style_set_text_font(&logoNameStyle, &lv_font_montserrat_48);
    lv_style_set_align(&logoNameStyle, LV_ALIGN_CENTER);

	lv_style_init (&radioNameStyle);		
	lv_style_set_text_color (&radioNameStyle, deselected);
	printf ("WARNING missing font lv_font_montserrat_32\n");
//    lv_style_set_text_font(&radioNameStyle, &lv_font_montserrat_32);
    lv_style_set_align(&radioNameStyle, LV_ALIGN_CENTER);


	lv_style_init (&artistNameStyle);		
	lv_style_set_text_color (&artistNameStyle, white);
	lv_style_set_align (&artistNameStyle, LV_ALIGN_TOP_LEFT);

	lv_style_init (&albumNameStyle);		
	lv_style_set_text_color (&albumNameStyle, white);
	lv_style_set_align (&albumNameStyle, LV_ALIGN_LEFT_MID);
	
	lv_style_init (&menuItemStyle);
	lv_style_set_radius (&menuItemStyle, 0);
	lv_style_set_border_width (&menuItemStyle, 0);	
	lv_style_set_width (&menuItemStyle, lv_pct(100));	
	lv_style_set_text_align (&menuItemStyle, LV_TEXT_ALIGN_LEFT);

	lv_style_init (&menuSelectedStyle);
	lv_style_set_outline_width (&menuSelectedStyle, 2);
	lv_style_set_outline_color (&menuSelectedStyle, yellow);
	lv_style_set_text_color (&menuSelectedStyle, white);
//	lv_style_set_bg_color (&menuSelectedStle, red);

	lv_style_init (&menuDeselectedStyle);
	lv_style_set_text_color (&menuDeselectedStyle, deselected);

	lv_style_init (&debugStyle);
	lv_style_set_bg_color (&debugStyle, red);	

	lv_style_init (&popUpStyle);
	lv_style_set_text_color (&popUpStyle, white);

	lv_style_init (&confirmFlexStyle);
	lv_style_set_flex_flow (&confirmFlexStyle, LV_FLEX_FLOW_COLUMN);
	lv_style_set_flex_main_place (&confirmFlexStyle, LV_FLEX_ALIGN_SPACE_EVENLY);	
	lv_style_set_layout (&confirmFlexStyle, LV_LAYOUT_FLEX);

	lv_style_init (&confirmStyle1);
	lv_style_set_text_color (&confirmStyle1, white);

	lv_style_init (&confirmStyle2);
	lv_style_set_text_color (&confirmStyle2, deselected);
	printf ("WARNING missing font lv_font_montserrat_20\n");	
//    lv_style_set_text_font(&confirmStyle2, &lv_font_montserrat_20);   	

	lv_style_init (&menuStyle);
	lv_style_set_radius (&menuStyle, 0);
	lv_style_set_border_width (&menuStyle, 0);	
	lv_style_set_width (&menuStyle, lv_pct(100));	
	lv_style_set_bg_color (&menuStyle, black);
	lv_style_set_pad_left (&menuStyle, 0);
	lv_style_set_pad_top (&menuStyle, 0);
	lv_style_set_pad_right (&menuStyle, 0);
	lv_style_set_pad_bottom (&menuStyle, 0);		

	lv_style_init (&metadataBoxStyle);		
	lv_style_set_bg_color (&metadataBoxStyle, black);
	lv_style_set_align (&metadataBoxStyle, LV_ALIGN_RIGHT_MID);
	lv_style_set_radius (&metadataBoxStyle, 0);
	lv_style_set_border_width (&metadataBoxStyle, 0);	

	lv_style_init (&zoneStyle);		
	lv_style_set_text_color (&zoneStyle, white);
	lv_style_set_align (&zoneStyle, LV_ALIGN_TOP_RIGHT);

	lv_style_init (&menuTRStyle);		
	lv_style_set_text_color (&menuTRStyle, white);
	lv_style_set_align (&menuTRStyle, LV_ALIGN_TOP_RIGHT);
		
}

lv_obj_t *idleScreen;
lv_obj_t *coverArt;

lv_img_dsc_t idsc;

void createIdleScreen() {

  printf("createIdleScreen ()\n");

  idleScreen = lv_obj_create(NULL);

  lv_obj_set_style_bg_color(idleScreen, black, LV_STATE_DEFAULT);
  idleTitle = lv_label_create(idleScreen);
  lv_label_set_text(idleTitle, LV_SYMBOL_WIFI);
  lv_obj_add_style(idleTitle, &menuTitleStyle, 0);

  menuLeftStatus = lv_label_create(idleScreen);
  lv_label_set_text(menuLeftStatus, LV_SYMBOL_PAUSE);
  lv_obj_add_style(menuLeftStatus, &leftStatusStyle, 0);

  menuRightStatus = lv_label_create(idleScreen);
  lv_label_set_text(menuRightStatus, LV_SYMBOL_VOLUME_MAX);
  lv_obj_add_style(menuRightStatus, &rightStatusStyle, 0);

  coverArt = lv_img_create(idleScreen);  

  metadataBox = lv_obj_create(idleScreen);
  lv_obj_add_style(metadataBox, &metadataBoxStyle, 0);
  lv_obj_set_size(metadataBox, 320 - ARTSIZE, ARTSIZE);

  trackName = lv_label_create(metadataBox);
  lv_obj_add_style(trackName, &trackNameStyle, 0);
  lv_label_set_long_mode(trackName, LONGMODE);
  lv_obj_set_width(trackName, 150);
  lv_label_set_text(trackName, "");


  logoName = lv_label_create(metadataBox);
  lv_obj_add_style(logoName, &logoNameStyle, 0);
  lv_label_set_long_mode(logoName, LV_LABEL_LONG_DOT);
  lv_obj_set_width (logoName, LV_SIZE_CONTENT);
  lv_obj_align (logoName, LV_ALIGN_CENTER, 0, 0);

  radioName = lv_label_create(metadataBox);
  lv_obj_add_style(radioName, &radioNameStyle, 0);
//  lv_label_set_long_mode(radioName, LV_LABEL_LONG_DOT);
  lv_label_set_long_mode(radioName, LV_LABEL_LONG_SCROLL);
  lv_obj_set_width (radioName, 300);
  lv_obj_align (radioName, LV_ALIGN_CENTER, 0, 0);

  artistName = lv_label_create(metadataBox);
  lv_obj_add_style(artistName, &artistNameStyle, 0);
  lv_label_set_long_mode(artistName, LONGMODE);
//  lv_obj_set_width(artistName, 320-ARTSIZE-METAMARGIN);
  lv_obj_set_width(artistName, 150);
  lv_label_set_text(artistName, "");


  albumName = lv_label_create(metadataBox);
  lv_obj_add_style(albumName, &albumNameStyle, 0);
  lv_label_set_long_mode(albumName, LONGMODE);
//  lv_obj_set_width(albumName, 320-ARTSIZE-METAMARGIN);
  lv_obj_set_width(albumName, 150);
  lv_label_set_text(albumName, "");

  idleZone = lv_label_create(idleScreen);
  lv_label_set_long_mode(idleZone, LONGMODE);  
  lv_obj_set_width(idleZone, 150);
  lv_obj_set_style_text_align(idleZone, LV_TEXT_ALIGN_RIGHT,0);
  lv_label_set_text(idleZone, "Zone");
  lv_obj_add_style(idleZone, &zoneStyle, 0);

}

lv_obj_t *menuBox;
lv_obj_t *alphaBox;

void createMenuScreen () {

	printf ("createMenuScreen()\n");

	int n;

  menuScreen = lv_obj_create(NULL);

  lv_obj_set_style_bg_color (menuScreen, black, LV_STATE_DEFAULT);
  menuTitle = lv_label_create(menuScreen);
  lv_label_set_text(menuTitle, "");
  lv_obj_add_style(menuTitle, &menuTitleStyle, 0);


  menuBox = lv_obj_create(menuScreen);
  lv_obj_set_size(menuBox, lv_pct(100),MENUHEIGHT);
  lv_obj_set_x(menuBox, 0);
  lv_obj_set_y(menuBox, MENUYPOS);
  lv_obj_add_style(menuBox, &menuStyle, 0);
  lv_obj_set_scrollbar_mode (menuBox, LV_SCROLLBAR_MODE_OFF);

  for (n = 0; n < MENUITEMSTODISPLAY; n++) {

    itemBox[n] = lv_obj_create(menuBox);
    
    lv_obj_set_size(itemBox[n], lv_pct(100),MENUPITCH);
    lv_obj_set_x(itemBox[n], 0);
    lv_obj_set_y(itemBox[n], n * MENUPITCH);
    
    lv_obj_add_style(itemBox[n], &menuStyle, 0);
    lv_obj_set_scrollbar_mode (itemBox[n], LV_SCROLLBAR_MODE_OFF);    

    itemText[n] = lv_label_create(itemBox[n]);

	lv_label_set_long_mode(itemText[n], LONGMODE);
	lv_obj_set_width(itemText[n], 320-20);
    	
    lv_label_set_text(itemText[n],"");
    lv_obj_set_x(itemText[n], MENUITEMX);
    lv_obj_set_y(itemText[n], MENUITEMY);
          
	lv_obj_add_style(itemBox[n], &menuDeselectedStyle, 0);    
  }  
   
  menuTR = lv_label_create(menuScreen);
  lv_obj_add_style(menuTR, &menuTRStyle, 0);
  lv_obj_set_style_text_align(menuTR, LV_TEXT_ALIGN_RIGHT,0);  
  lv_label_set_long_mode(menuTR, LONGMODE);
  lv_obj_set_width(menuTR, 150);
  lv_label_set_text(menuTR, "menuTR");   
   
}

void createAlphaScreen() {

  printf("createAlphaScreen()\n");

  int n, m;

  alphaScreen = lv_obj_create(NULL);

  lv_obj_set_style_bg_color(alphaScreen, black, LV_STATE_DEFAULT);
  alphaTitle = lv_label_create(alphaScreen);
  lv_label_set_text(alphaTitle, "");
  lv_obj_add_style(alphaTitle, &menuTitleStyle, 0);

  alphaHelp = lv_label_create(alphaScreen);
  lv_label_set_text(alphaHelp, "Press Next when done");
  lv_obj_add_style(alphaHelp, &leftStatusStyle, 0);

  alphaBox = lv_obj_create(alphaScreen);
  lv_obj_set_size(alphaBox, lv_pct(100), MENUHEIGHT);
  lv_obj_set_x(alphaBox, 0);
  lv_obj_set_y(alphaBox, MENUYPOS);
  lv_obj_add_style(alphaBox, &menuStyle, 0);
  lv_obj_set_scrollbar_mode(alphaBox, LV_SCROLLBAR_MODE_OFF);

  int l = 0;
  for (n = 0; n < MENUITEMSFULLYVISIBLE; n++) {

    for (m = 0; m < LETTERSPERROW; m++) {

// NOTE lv_obj_create crashes if I attempt to create 64 letters

      letterBox[l] = lv_obj_create(alphaBox);

      lv_obj_set_size(letterBox[l], LETTERWIDTH, MENUPITCH);
      lv_obj_set_x(letterBox[l], m * LETTERWIDTH);
      lv_obj_set_y(letterBox[l], n * MENUPITCH);

      lv_obj_add_style(letterBox[l], &menuStyle, 0);
      lv_obj_set_scrollbar_mode(letterBox[l], LV_SCROLLBAR_MODE_OFF);

      letterText[l] = lv_label_create(letterBox[l]);

      lv_label_set_text(letterText[l], "");

		lv_obj_align (letterText[l], LV_ALIGN_CENTER, 0, 0);

      lv_obj_add_style(letterBox[l], &menuDeselectedStyle, 0);

      l++;
    }
  }
}

lv_obj_t *parking;
lv_obj_t *popUpText;

lv_obj_t *popUpScreen;
lv_obj_t *popUpBox;

void createPopUp() {

  printf("createPopUp ()\n");

  parking = lv_obj_create(NULL);

  popUpScreen = lv_obj_create(parking);
  lv_obj_set_size(popUpScreen, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(popUpScreen, black, LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(popUpScreen, 0x80, LV_STATE_DEFAULT);
  lv_obj_set_style_border_width (popUpScreen, 0, LV_STATE_DEFAULT);	

  popUpBox = lv_obj_create(popUpScreen);
  lv_obj_set_size(popUpBox, lv_pct(75), lv_pct(75));
  lv_obj_set_style_bg_color(popUpBox, popbg, LV_STATE_DEFAULT);

  lv_obj_set_style_radius(popUpBox, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(popUpBox, 0, LV_STATE_DEFAULT);
  lv_obj_center(popUpBox);

  popUpText = lv_label_create(popUpBox);
  lv_obj_center(popUpText);
  lv_obj_add_style(popUpText, &popUpStyle, 0);
}

void popUp(char *text) {

  lockLVGL();	
  lv_obj_set_parent(popUpScreen, lv_scr_act());
  lv_label_set_text(popUpText, text);
  unlockLVGL();
}

void hidePopUp (){
  lockLVGL();		
	lv_obj_set_parent(popUpScreen, parking);
  unlockLVGL();	
}


lv_obj_t *confirmScreen;
lv_obj_t *confirmContainer;
lv_obj_t *confirmMsg1;
lv_obj_t *confirmMsg2;

void createConfirmScreen () {

	printf ("createConfirmScreen()\n");

	int n;

  confirmScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color (confirmScreen, black, LV_STATE_DEFAULT);
  lv_obj_set_style_border_width (confirmScreen, 0, LV_STATE_DEFAULT);  
  
  confirmContainer = lv_obj_create (confirmScreen);
  lv_obj_set_style_bg_color (confirmContainer, black, LV_STATE_DEFAULT);
  lv_obj_set_style_border_width (confirmContainer, 0, LV_STATE_DEFAULT); 
  lv_obj_set_width (confirmContainer, lv_pct (100));
  lv_obj_set_height (confirmContainer, lv_pct (100));  
  lv_obj_align (confirmContainer, LV_ALIGN_CENTER, 0, 0);

  lv_obj_add_style(confirmContainer, &confirmFlexStyle, 0);

  confirmMsg1 = lv_label_create (confirmContainer);
  lv_label_set_text(confirmMsg1, "Confirm message 1");
  lv_obj_center(confirmMsg1);
  lv_obj_add_style(confirmMsg1, &confirmStyle1, 0);  

  confirmMsg2 = lv_label_create (confirmContainer);
  lv_label_set_text(confirmMsg2, "Push knob to confirm\nOr any other key to go back");
  lv_obj_center(confirmMsg2);
  lv_obj_add_style(confirmMsg2, &confirmStyle2, 0);
 
      
}

struct timespec lastTimeout;

void uiInit (){
	waitlock_init (&uiLock);
	uiQLen = 0;
	uiQIn = 0;
	uiQOut = 0;
	
	myPlaylistUri = heap_caps_malloc (MAXPLAYLISTS*STATETRACKURILEN,MALLOC_CAP_SPIRAM);	
	myPlaylistName = heap_caps_malloc (MAXPLAYLISTS*MAXNAMELEN,MALLOC_CAP_SPIRAM);

	myShowUri = heap_caps_malloc (MAXSHOWS*STATETRACKURILEN,MALLOC_CAP_SPIRAM);	
	myShowName = heap_caps_malloc (MAXSHOWS*MAXNAMELEN,MALLOC_CAP_SPIRAM);		

	int n;
	for (n=0;n<MAXPLAYLISTS;n++){
		myPlaylistName[n][0] = 0;
		myPlaylistUri[n][0] = 0;		
	}
	for (n=0;n<MAXSHOWS;n++){
		myShowName[n][0] = 0;
		myShowUri[n][0] = 0;		
	}
	
	ap_info = (wifi_ap_record_t *) heap_caps_malloc (DEFAULT_SCAN_LIST_SIZE*sizeof(wifi_ap_record_t),MALLOC_CAP_SPIRAM);	

	clock_gettime(CLOCK_REALTIME, &lastTimeout);
	
	initStyles ();	
	
	createIdleScreen();
	createMenuScreen ();
	createAlphaScreen ();
	createPopUp ();
	createConfirmScreen ();			

// These attempts to set the timezone did not work
/*	
//	setenv ("TZ","Europe/London",1);
	setenv ("TZ","America/New_York",1);
	tzset ();
*/	
//	setlocale (LC_ALL, "en_GB");
	
	loadFavourites ();
	
	refreshUI ();		
}



void addEvent (int e){
	pthread_mutex_lock (&uiLock.mutex);			// lock the variables
	
	if (uiQLen < UIQLEN){						// insert in queue
		uiQ[uiQIn++] = e;
		uiQLen++;
		if (uiQIn >= UIQLEN) uiQIn = 0;
	}
	
	pthread_cond_signal (&uiLock.condition);
	pthread_mutex_unlock (&uiLock.mutex);
}


void orEvent (int e){
	pthread_mutex_lock (&uiLock.mutex);			// lock the variables

	// search queue for the event

	int n,o;
	for (o=uiQOut,n=0;n<uiQLen;n++){
		if (uiQ[o++] == e) {
			pthread_mutex_unlock (&uiLock.mutex);
			return;
		}
		if (o >= UIQLEN) o = 0;
	}							
	
	// add the event of not there already
	
	if (uiQLen < UIQLEN){						// insert in queue
		uiQ[uiQIn++] = e;
		uiQLen++;
		if (uiQIn >= UIQLEN) uiQIn = 0;
	}
	pthread_cond_signal (&uiLock.condition);
	pthread_mutex_unlock (&uiLock.mutex);
}

void refreshUI (){
	orEvent (UIREFRESH);
}	

// This function consumes all up/down events in the queue so they can be processed togther
// returns the net up event count


int getUpDownEvents (){
	
	int u = 0;
	int e;
	waitlock_t *wl = &uiLock;
	
	pthread_mutex_lock(&wl->mutex);	
	
	while (uiQLen){
		e = uiQ[uiQOut];
		if (e == ROTARYUP){
			u++;
			uiQLen--;
			uiQOut++;
		}
		else if (e == ROTARYDOWN){
			u--;
			uiQLen--;
			uiQOut++;
		}
		else break;
		if (uiQOut >= UIQLEN) uiQOut = 0;				
	}	

	pthread_mutex_unlock(&wl->mutex);
	return u;
	
}



void waitlock_init (waitlock_t *wl){
	pthread_mutex_init (&wl->mutex,NULL);
	wl->flag = 0;
}

void waitlock_signal (waitlock_t *wl){
	pthread_mutex_lock (&wl->mutex);
	wl->flag = 1;
	pthread_cond_signal (&wl->condition);
	pthread_mutex_unlock (&wl->mutex);	
}	

/*
 waitlock_wait waits for a signal on a waitlock structure or a timeout
 returns zero on timeout 1 on being signalled
*/

int waitlock_wait (waitlock_t *wl, int msdelay)
{
	struct timespec timeout;

	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += msdelay / 1000;
	timeout.tv_nsec += (msdelay % 1000) * 1000000;
	if (timeout.tv_nsec > 1000000000) {
		timeout.tv_sec++;
		timeout.tv_nsec %= 1000000000;
	}

	pthread_mutex_lock(&wl->mutex);

	/* If event is not already signalled ... */
	while (!wl->flag) {			// to avoid missed wakeup
		/* Wait until either timeout or event condition signalled */
		if (pthread_cond_timedwait(&wl->condition, &wl->mutex, &timeout)) {
//			fprintf(stderr, "waitevent: timeout\n");
			pthread_mutex_unlock(&wl->mutex);
			return 0;
		}
	}

	wl->flag = 0;
	pthread_mutex_unlock(&wl->mutex);
	return 1;
}


int getEvent (){

	int e = 0;
	waitlock_t *wl = &uiLock;	
	pthread_mutex_lock(&wl->mutex);
		
	if (uiQLen) {
		e = uiQ[uiQOut++];							// get event from queue
		uiQLen--;
		if (uiQOut >= UIQLEN) uiQOut = 0;
	}		
	pthread_mutex_unlock(&wl->mutex);
	return e;
}


typedef struct {
	PFI test;			// test to say whether menu item should be displayed
	PFV function;		// code to execute when selected
	char *text;			// description of function
} smartMenuItem;

smartMenuItem *getMenuItem (smartMenuItem *menu, int index){
	int n = 0;
	int i = 0;
	while (menu[n].function){
		PFI test = menu[n].test;
		if (!test || test ()){
			if (i == index) return &menu[n];
			i++;
		}
		n++;
	}
	return NULL;					
}

char * getMenuItemText (smartMenuItem *menu, int index){
	smartMenuItem *item = getMenuItem (menu,index);
	if (item) return item->text;
	else return "";
}

// index should be < menu length

void adjustMenuOffset (int index, int *offset){
	
	if (index < *offset) *offset = index;
	else if (index >= *offset + MENUITEMSFULLYVISIBLE) *offset = index + 1 - MENUITEMSFULLYVISIBLE;
}


int getMenuLength (smartMenuItem *menu){
	int n = 0;
	int count = 0;
	while (menu[n].function) {
		PFI test = menu[n].test;
		if (!test || test ()) count++;
		n++;
	}	
	return count;
}

void doMenuItem (smartMenuItem *menu, int index){
	smartMenuItem *item = getMenuItem (menu,index);
	if (item){
		PFV f = item->function;
		if (f) f();
	}	
}


void paintItems (int index, int offset, PFC nameFunction) {

  int n;
  
  for (n = 0; n < MENUITEMSTODISPLAY; n++) {

	char *text = nameFunction (offset+n);
		
    lv_label_set_text(itemText[n],text);

    
    int w = lv_obj_get_style_outline_width (itemBox[n], LV_PART_MAIN);
    
    if (offset + n == index){
		lv_obj_add_style(itemBox[n], &menuSelectedStyle, 0);
	}	 	
	else if (w) {	  
		lv_obj_remove_style (itemBox[n], &menuSelectedStyle, 0); 

	}	    

  }   
}



#define UISTACKDEPTH 20

u16 uiStack[UISTACKDEPTH];
u16 uiDepth = 0;
int uiState = 0;

void pushState (int state)
{
	if (state == UIIDLE) uiDepth = 0;
    else if (uiDepth < UISTACKDEPTH) uiStack[uiDepth++] = state;
    else {
        printf ("************ pushState (%d) full \n",state);
    }        
}

int popState (){
    if (uiDepth) return (uiStack[--uiDepth]);
    return (UIIDLE);
} 

void gotoState (int state){
	uiState = state;
}

void callState (int state){
	
	if (uiState == UIALERT) uiState = popState ();
	pushState (uiState);
	gotoState (state);
}

void goBack (){
	gotoState (popState());
	uiHandlers[uiState](UIREFRESH);			// force a refresh 	
}

void gotoIdle (){
	uiDepth = 0;
	uiState = UIIDLE;
	refreshUI ();
}

int lastPaintLettersOffset;
int lastPaintLettersSelected;

#define LETTERROWS (MENUITEMSFULLYVISIBLE)	

void paintLetters (int index, int offset, PFC nameFunction) {

  int n;
  
//  printf ("paintLetters() %d/%d\n",index,offset);
  
  int co = offset * LETTERSPERROW;	
  
	if (lastPaintLettersSelected >= 0)
		lv_obj_remove_style (letterBox[lastPaintLettersSelected], &menuSelectedStyle, 0); 	
  
  for (n = 0; n < (LETTERROWS*LETTERSPERROW); n++) {


	if (co != lastPaintLettersOffset)
		lv_label_set_text(letterText[n],nameFunction (co+n));
    
    if (co + n == index){
		lv_obj_add_style(letterBox[n], &menuSelectedStyle, 0);
		lastPaintLettersSelected = n;
	}	 	

  }   
  lastPaintLettersOffset = co;
}

void initPaintLetters (){
	
	int n;
	
	lastPaintLettersOffset = -1;
	lastPaintLettersSelected = -1;
	
  for (n = 0; n < (LETTERROWS*LETTERSPERROW); n++) {
		lv_obj_remove_style (letterBox[n], &menuSelectedStyle, 0); 

  }  	

}


/************************* Main Menu *************************/

int mainMenuIndex = 0;
int mainMenuOffset = 0;


void dummy (){
	printf ("dummy()\n");
}


smartMenuItem mainMenu[] = {

	{NULL,gotoMyPlaylists,"My Playlists"},
	{NULL,gotoMyShows,"My Podcasts"},
	{NULL,gotoFavourites,"Favourite Radio Stations"},
	{NULL,gotoPickSSID,"Setup Wifi"},	
	{NULL,NULL,"Main Menu"}
};

char *getMainMenuText (int index){
	
	return getMenuItemText(mainMenu, index);
}	



void displayMain() {

	printf ("displayMain()\n");

  lockLVGL();
    	
  lv_label_set_text(menuTitle, "Main Menu");

  lv_label_set_text(menuTR, ""); 
  
  adjustMenuOffset (mainMenuIndex, &mainMenuOffset);

  paintItems (mainMenuIndex, mainMenuOffset, getMainMenuText);
  
  lv_scr_load(menuScreen);  
  
  unlockLVGL(); 
    
}

int mainMenuHandler (int e){

//	printf ("mainMenuHandler event %d\n",e);
	
  int top = getMenuLength (mainMenu)-1;	
	
	if (e == BACKBUTTON){
		goBack ();
		return 1;
	}
	else if (e == UIREFRESH){	
		displayMain ();
		return 1;
	}
	else if (e == ROTARYUP){
		if (mainMenuIndex < top) mainMenuIndex++;
		else mainMenuIndex = 0;
		displayMain();
		return 1;		
	}		
	else if (e == ROTARYDOWN){
		if (mainMenuIndex > 0) mainMenuIndex--;
		else mainMenuIndex = top;
		displayMain();
		return 1;
	}
	else if (e == KNOBPUSH){
		doMenuItem (mainMenu,mainMenuIndex);
		return 1;
	}	
	return 0;

}

#if 1
/************************* MyPlaylists Menu *************************/


// for this to work as intended index should equal the 
// current value of myPlaylistsCount


void getMyPlaylistsPage (int index){
	
	printf ("getMyPlaylistsPage %d\n",index);
	
	cJSON *response = getMyPlaylists (index,10);
	if (!response) return;

	cJSON *total = cJSON_GetObjectItemCaseSensitive(response, "total");
	if (!total) goto gmppx;
	myPlaylistsTotal = total->valueint;
	cJSON *items = cJSON_GetObjectItemCaseSensitive(response, "items");
	int count = cJSON_GetArraySize (items);
	printf ("getMyPlaylistsPage got %d items\n",count);
	if ((index + count)>MAXPLAYLISTS){
		printf ("getMyPlaylistsPage () too many playlists\n");
		goto gmppx;
	}
	int n;
	for (n=0;n<count;n++){	
		cJSON *item = cJSON_GetArrayItem (items,n);
		cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
		cJSON *uri = cJSON_GetObjectItemCaseSensitive(item, "uri");			
		if (name) strcpy (myPlaylistName[index+n],name->valuestring);
		else *myPlaylistName[index+n] = 0;
		printf ("Got name %s\n",myPlaylistName[index+n]);
//		cdump ((unsigned char *)myPlaylistName[index+n],8);		
		if (uri) strcpy (myPlaylistUri[index+n],uri->valuestring);
		else *myPlaylistUri[index+n] = 0;
	}
	myPlaylistsCount += count;
	
gmppx:
	cJSON_Delete (response);	
	return;
}

char *getLoadedPlaylistName (int index){
	
	if (index >= MAXPLAYLISTS) return "";
	if (index >= myPlaylistsTotal) return "";
	return myPlaylistName[index];		
}


char *getPlaylistName (int index){
	
	if (index >= MAXPLAYLISTS) return "";
	if (index >= myPlaylistsTotal) return "";
	while (index >= myPlaylistsCount){
		getMyPlaylistsPage (myPlaylistsCount);
	}
	return myPlaylistName[index];		
}

char *getPlaylistUri (int index){
	
	if (index >= MAXPLAYLISTS) return "";
	if (index >= myPlaylistsTotal) return "";
	while (index >= myPlaylistsCount){
		getMyPlaylistsPage (myPlaylistsCount);
	}
	return myPlaylistUri[index];		
}

void initMyPlaylists (){
	
	myPlaylistsCount = 0;
	myPlaylistsTotal = 0;
	getMyPlaylistsPage (0);
}

int getMyPlaylistsTotal (){
	return myPlaylistsTotal;
}

int getMyPlaylistsCount (){
	return myPlaylistsCount;
}

int myPlaylistsMenuIndex = 0;
int myPlaylistsMenuOffset = 0;


void gotoMyPlaylists2 (){
		 
	callState(MYPLAYLISTS);
	initMyPlaylists ();
	displayMyPlaylists ();
	
}

void gotoMyPlaylists (){
		 
//    if (!getIsActive()) {
    if (!isDealerConnected()) {
		gotoLogin (gotoMyPlaylists2);
	}	
    else {
		callState(MYPLAYLISTS);
		initMyPlaylists ();
		displayMyPlaylists ();
	}	
}	

void loadMyPlaylists2 (){
	printf ("loadMyPlaylists2 ()\n"); 
	initMyPlaylists ();
	char *title;
	int index = 0;
	do {
		title = getPlaylistName (index++);
	} while (title[0]);	
	printf ("loadMyPlaylists2 got %d items\n",index-1);
	goBack (); 
}	

void loadMyPlaylists (){
//    if (!getIsActive()) {
    if (!isDealerConnected()) {
		gotoLogin (loadMyPlaylists2);
	}
	else loadMyPlaylists2 ();		
}	

char *getCleanPlaylistName (int index){
	return (cleanName(getPlaylistName(index)));
}	


void displayMyPlaylists () {

  printf ("displayMyPlaylists ()\n");

  lockLVGL();
    	
  lv_label_set_text(menuTitle, "My Playlists");

  lv_label_set_text(menuTR, ""); 
  
  adjustMenuOffset (myPlaylistsMenuIndex, &myPlaylistsMenuOffset);

  paintItems (myPlaylistsMenuIndex, myPlaylistsMenuOffset, getCleanPlaylistName);
  
  lv_scr_load(menuScreen);  
  
  unlockLVGL(); 

  printf ("displayMyPlaylists () done\n");
    
}

	


int myPlaylistsHandler (int e){

//	printf ("myPlaylistsHandler event %d\n",e);
	
  int top = getMyPlaylistsTotal ()-1;	
	
	if (e == BACKBUTTON){
		gotoIdle ();
		return 1;
	}
	else if (e == UIREFRESH){	
		displayMyPlaylists ();
		return 1;
	}
	else if (e == ROTARYUP){
		if (myPlaylistsMenuIndex < top) myPlaylistsMenuIndex++;
		else myPlaylistsMenuIndex = 0;
		displayMyPlaylists();
		return 1;		
	}		
	else if (e == ROTARYDOWN){
		if (myPlaylistsMenuIndex > 0) myPlaylistsMenuIndex--;
		else myPlaylistsMenuIndex = top;
		displayMyPlaylists();
		return 1;
	}	
	else if ((e == KNOBPUSH) || (e == PLAYSTOPBUTTON)){
		char *uri = getPlaylistUri(myPlaylistsMenuIndex);
		printf ("Play uri = %s\n",uri);		
		playUri (uri);
		gotoIdle ();		
		return 1;
	}
	return 0;

}
#endif

#if 1
/************************* MyShows Menu *************************/

int myShowsMenuIndex = 0;
int myShowsMenuOffset = 0;

// for this to work as intended index should equal the 
// current value of myShowsCount


void getMyShowsPage (int index){
	
	cJSON *response = getMyShows (index,10);
	
	
	if (!response) return;
	cJSON *total = cJSON_GetObjectItemCaseSensitive(response, "total");
	if (!total) goto gmppx;
	myShowsTotal = total->valueint;

	printf ("Total = %d\n",myShowsTotal);

	cJSON *items = cJSON_GetObjectItemCaseSensitive(response, "items");
	int count = cJSON_GetArraySize (items);
	
	printf ("Count = %d\n",count);	
	
	if ((index + count)>MAXSHOWS){
		printf ("getMyShowsPage () too many shows\n");
		goto gmppx;
	}
	int n;
	for (n=0;n<count;n++){
		cJSON *item = cJSON_GetArrayItem (items,n);
		cJSON *show = cJSON_GetObjectItemCaseSensitive(item, "show");
		cJSON *name = cJSON_GetObjectItemCaseSensitive(show, "name");
		cJSON *uri = cJSON_GetObjectItemCaseSensitive(show, "uri");	
		
		printf ("Name = %s\n",name->valuestring);
		printf ("Uri = %s\n",uri->valuestring);
		printf ("Index = %d\n",index+n);
				
		if (name) strcpy (myShowName[index+n],name->valuestring);
		else *myShowName[index+n] = 0;
		if (uri) strcpy (myShowUri[index+n],uri->valuestring);
		else *myShowUri[index+n] = 0;
	}
	myShowsCount += count;
	
gmppx:
	cJSON_Delete (response);
	return;
}

char *getShowName (int index){
	
	if (index >= MAXSHOWS) return "";
	if (index >= myShowsTotal) return "";
	while (index >= myShowsCount){
		getMyShowsPage (myShowsCount);
	}
	return myShowName[index];		
}

char *getShowUri (int index){

	printf ("getShowUri %d, %s\n",index,myShowUri[0]);
	
	if (index >= MAXSHOWS) return "";
	if (index >= myShowsTotal) return "";
	while (index >= myShowsCount){
		getMyShowsPage (myShowsCount);
	}
	return myShowUri[index];		
}

void initMyShows (){
	
	myShowsCount = 0;
	myShowsTotal = 0;
	getMyShowsPage (0);
}

int getMyShowsTotal (){
	return myShowsTotal;
}




void gotoMyShows (){	
    callState(MYSHOWS);
	initMyShows ();
	displayMyShows ();
}	


void displayMyShows () {

  printf ("displayMyShows ()\n");

  lockLVGL();
    	
  lv_label_set_text(menuTitle, "My Shows");

  lv_label_set_text(menuTR, ""); 
  
  adjustMenuOffset (myShowsMenuIndex, &myShowsMenuOffset);

  paintItems (myShowsMenuIndex, myShowsMenuOffset, getShowName);
  
  lv_scr_load(menuScreen);  
  
  unlockLVGL(); 
    
}


int myShowsHandler (int e){

//	printf ("myShowsHandler event %d\n",e);
	
    int top = getMyShowsTotal ()-1;	
	
	if (e == BACKBUTTON){
		goBack ();
		return 1;
	}
	else if (e == UIREFRESH){	
		displayMyShows ();
		return 1;
	}
	else if (e == ROTARYUP){
		if (myShowsMenuIndex < top) myShowsMenuIndex++;
		else myShowsMenuIndex = 0;
		displayMyShows();
		return 1;		
	}		
	else if (e == ROTARYDOWN){
		if (myShowsMenuIndex > 0) myShowsMenuIndex--;
		else myShowsMenuIndex = top;
		displayMyShows();
		return 1;
	}
	else if (e == KNOBPUSH){
		return 1;
	}	
	else if (e == PLAYSTOPBUTTON){	
		char *uri = getShowUri(myShowsMenuIndex);
		printf ("Play %d uri = %s\n",myShowsMenuIndex,uri);		
		playUri (uri);		
		gotoIdle ();		
		return 1;
	}
	return 0;

}

#endif

/************************* Favourites Menu *************************/



cJSON *favourites = NULL;

cJSON *getFavourites (){
	return favourites;
}	


int favouritesMenuIndex = 0;
int favouriteMenuOffset = 0;


char *getFavouriteName (int index){
	
	if (index == 0) return "Add Current Station";
	if (!favourites) return "";
	cJSON *station = cJSON_GetArrayItem (favourites,index-1);
	if (station){
		cJSON *name = cJSON_GetObjectItemCaseSensitive(station, "name");
  return name->valuestring;
 }
 return ""; 
}

int playFavourite (int index){

	if (index == 0) return 0;
	if (!favourites) return 0;
	cJSON *station = cJSON_GetArrayItem (favourites,index-1);
	if (station){
		cJSON *name = cJSON_GetObjectItemCaseSensitive(station, "name");
		cJSON *logo = cJSON_GetObjectItemCaseSensitive(station, "logo");
		cJSON *url = cJSON_GetObjectItemCaseSensitive(station, "url");
  return startDirect2 (name->valuestring, url->valuestring, logo->valuestring);
 }
 return 0; 
	
}	

int getFavouritesTotal (){
	if (favourites) return cJSON_GetArraySize (favourites);
	else return 0;
}	

void saveFavourites() {

  if (!favourites)
    return;

  char *jsonString = cJSON_Print(favourites);

  //  printf("saveFavourites %s\n", jsonString);

  FILE *f = fopen("/spiffs/Favourites", "wb");
  fprintf(f, "%s", jsonString);
  fclose(f);
  cJSON_free(jsonString);
}

int loadFavourites() {

  if (!fileExists("/spiffs/Favourites"))
    return 0;

  FILE *f = fopen("/spiffs/Favourites", "r");
  if (!f)
    return 0;

  fseeko(f, 0, SEEK_END);
  int size = ftello(f);
  fseeko(f, 0, SEEK_SET);

  //  printf("Favourites file length %d\n", size);
  char *buf = (char *) malloc(size + 1);
  if (!buf) {
    fclose(f);
    return 0;
  }
  memset(buf, 0, size + 1);
  fread(buf, 1, size, f);
  fclose(f);

  printf("favourites = %s\n", buf);

  favourites = cJSON_Parse(buf);

  free(buf);
  return (int)favourites;
}


void gotoFavourites (){
		 
		callState(FAVOURITES);
		displayFavourites ();
	}	
	


void displayFavourites () {

  printf ("displayFavourites ()\n");

  lockLVGL();
    	
  lv_label_set_text(menuTitle, "Favourite Stations");

  lv_label_set_text(menuTR, ""); 
  
  adjustMenuOffset (favouritesMenuIndex, &favouriteMenuOffset);

  paintItems (favouritesMenuIndex, favouriteMenuOffset, getFavouriteName);
  
  lv_scr_load(menuScreen);  
  
  unlockLVGL(); 

  printf ("displayFavourites () done\n");
    
}


void addCurrentToFavourites (){
	
	if (!favourites) favourites = cJSON_CreateArray();

	cJSON *station = cJSON_CreateObject(); 

 cJSON *name = cJSON_CreateString(getCurrentStationName ());
 cJSON_AddItemToObject(station, "name", name);
	
 cJSON *logo = cJSON_CreateString(getCurrentStationLogo ());
 cJSON_AddItemToObject(station, "logo", logo);	

 cJSON *url = cJSON_CreateString(getCurrentStationUrl ());
 cJSON_AddItemToObject(station, "url", url);
	
 cJSON_AddItemToArray(favourites, station);
		
}		


int favouritesHandler(int e) {

  //	printf ("favouritesHandler event %d\n",e);

  int top = getFavouritesTotal();

  if (e == BACKBUTTON) {
    goBack();
    return 1;
  } else if (e == UIREFRESH) {
    displayFavourites();
    return 1;
  } else if (e == ROTARYUP) {
    if (favouritesMenuIndex < top)
      favouritesMenuIndex++;
    else
      favouritesMenuIndex = 0;
    displayFavourites();
    return 1;
  } else if (e == ROTARYDOWN) {
    if (favouritesMenuIndex > 0)
      favouritesMenuIndex--;
    else
      favouritesMenuIndex = top;
    displayFavourites();
    return 1;
  } else if (e == KNOBPUSH) {
    if (favouritesMenuIndex == 0) {
      if (isRadioPlaying()) {
        printf("add to favourites\n");
        addCurrentToFavourites();
        saveFavourites();
        displayFavourites();
        alertTimer = 5;
        popUp("Done");
      } else {
        alertTimer = 5;
        popUp("Nothing Playing");
      }
    }
    return 1;
  } else if (e == PLAYSTOPBUTTON) {
    playFavourite(favouritesMenuIndex);
    gotoIdle();
    return 1;
  }
  return 0;
}



/************************* Idle *************************/


char timeString[20];

char *getTimeString (){

	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) sprintf (timeString,"No Clock");
	else if (tmp->tm_year < 100) sprintf (timeString,"Not Set");
	else {												
//		sprintf (timeString,"%02d:%02d",(tmp->tm_hour+1)%24,tmp->tm_min);	// temporary timeZone BST
		sprintf (timeString,"%02d:%02d",(tmp->tm_hour)%24,tmp->tm_min);		// temporary timeZone GMT
	}
	return timeString;
}



void paintTopLeftStatus (){

  char text[32];
  char *ip = getLocalIp();	

  if (!ip[0]){
	sprintf(text, "#808080 %s#", LV_SYMBOL_WIFI);	// grey
  }
  else if (!isApResolved()){
	sprintf(text, "#FF0000 %s# %s", LV_SYMBOL_WIFI,ip);	// red  
  }	
  else if (!isDealerConnected()){
	sprintf(text, "#FFBF00 %s# %s", LV_SYMBOL_WIFI,ip);	// amber  
  }
  else {	
	sprintf(text, "#00FF00 %s# %s", LV_SYMBOL_WIFI,ip);	// green  
  }	
  lv_label_set_text(idleTitle, text);
  lv_label_set_recolor(idleTitle, true);

}



char *getTransportIcon() {
  if (getStateIsPlaying()) {
    if (getStateIsPaused())
      return LV_SYMBOL_PAUSE;
    else
      return LV_SYMBOL_PLAY;
  } else
    return LV_SYMBOL_STOP;
}


void paintBottomLeftStatus() {

  char msg[60];

  if (isSpotifySource()) {
    if (getIsActive())
      sprintf(msg, "%s %s %s/%s %d/%d", LV_SYMBOL_OK, getTransportIcon(),
              getProgressString(), getDurationString(), getCurrentTrackIndex() + 1, getTrackCount());
    else
      sprintf(msg, "%s %s %d/%d", getTransportIcon(), getProgressString(),
              getCurrentTrackIndex() + 1, getTrackCount());

    lv_label_set_text(menuLeftStatus, msg);
  } else if (isRadioSource()) {
    if (isRadioPlaying())
      lv_label_set_text(menuLeftStatus, LV_SYMBOL_PLAY);
    else
      lv_label_set_text(menuLeftStatus, LV_SYMBOL_STOP);
  }
}

void displaySpotifyMetadata (){


	uint8_t *img = getArt (getPlayingArtUrl());

	if (stress) {
		artToggle = !artToggle;
		if (artToggle) img = NULL;
	}	

  if (img) {

    idsc.header.always_zero = 0;
    idsc.header.w = getArtWidth();
    idsc.header.h = getArtHeight();
    idsc.data_size = getArtWidth() * getArtHeight() * 2;
    idsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    idsc.data = img;

    int zoom = (256 * ARTSIZE) / getArtHeight();
    int offset = (((getArtWidth() * zoom) >> 8) - getArtWidth()) >> 1;


    lv_img_set_src(coverArt, &idsc);
    lv_img_set_zoom(coverArt, zoom);
    lv_obj_align(coverArt, LV_ALIGN_LEFT_MID, offset, 0);
    
	lv_obj_set_scrollbar_mode (coverArt, LV_SCROLLBAR_MODE_OFF);    
    
  }
  
  lv_obj_set_scrollbar_mode (metadataBox, LV_SCROLLBAR_MODE_OFF);
  
  if (img) {
    lv_obj_set_size(metadataBox, 320 - ARTSIZE, ARTSIZE);
    lv_obj_set_width(trackName, 320 - ARTSIZE - METAMARGIN);
    lv_obj_set_width(albumName, 320 - ARTSIZE - METAMARGIN);
    lv_obj_set_width(artistName, 320 - ARTSIZE - METAMARGIN);
  } else {
    lv_obj_set_size(metadataBox, 320, ARTSIZE);
    lv_obj_set_width(trackName, 320 - METAMARGIN);
    lv_obj_set_width(albumName, 320 - METAMARGIN);
    lv_obj_set_width(artistName, 320 - METAMARGIN);
    lv_obj_set_width(radioName, 320 - METAMARGIN);
  }
  

  
  if (getPlayingTrackName()[0]){	  
    lv_label_set_text(trackName, cleanName(getPlayingTrackName()));
    lv_label_set_text(artistName, cleanName (getPlayingArtistName()));
    lv_label_set_text(albumName, cleanName(getPlayingAlbumName()));
    lv_label_set_text(logoName, "");
    lv_label_set_text(radioName, "");
  }
  else {
    lv_label_set_text(trackName, "");
    lv_label_set_text(artistName, "");
    lv_label_set_text(albumName, "");
    lv_label_set_text(radioName, "");
    lv_label_set_text(logoName, "Loco");
  }
  
}

void displayLogo() {

  lv_obj_set_scrollbar_mode(metadataBox, LV_SCROLLBAR_MODE_OFF);

  lv_obj_set_size(metadataBox, 320, ARTSIZE);
  lv_obj_set_width(trackName, 320 - METAMARGIN);
  lv_obj_set_width(albumName, 320 - METAMARGIN);
  lv_obj_set_width(artistName, 320 - METAMARGIN);
  lv_obj_set_width(radioName, 320 - METAMARGIN);

  lv_label_set_text(trackName, "");
  lv_label_set_text(artistName, "");
  lv_label_set_text(albumName, "");
  lv_label_set_text(radioName, "");
  lv_label_set_text(logoName, "Loco");
}




void displayRadioMetadata (){
	
	
			fetchArt (getCurrentStationLogo());	
			uint8_t *img = getArt (getCurrentStationLogo());	
			
  if (img) {

    idsc.header.always_zero = 0;
    idsc.header.w = getArtWidth();
    idsc.header.h = getArtHeight();
    idsc.data_size = getArtWidth() * getArtHeight() * 2;
    idsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    idsc.data = img;

    int zoom = (256 * ARTSIZE) / getArtHeight();
    int offset = (((getArtWidth() * zoom) >> 8) - getArtWidth()) >> 1;

//	printf ("displayMetadata zoom %d offset %d\n",zoom,offset);

    lv_img_set_src(coverArt, &idsc);
    lv_img_set_zoom(coverArt, zoom);
    lv_obj_align(coverArt, LV_ALIGN_LEFT_MID, offset, 0);
    
	lv_obj_set_scrollbar_mode (coverArt, LV_SCROLLBAR_MODE_OFF);    
    
  }			
			
  if (img) {
    lv_obj_set_size(metadataBox, 320 - ARTSIZE, ARTSIZE);
    lv_obj_set_width(trackName, 320 - ARTSIZE - METAMARGIN);
    lv_obj_set_width(albumName, 320 - ARTSIZE - METAMARGIN);
    lv_obj_set_width(artistName, 320 - ARTSIZE - METAMARGIN);
    lv_obj_set_width(radioName, 320 - ARTSIZE - METAMARGIN);
  } else {
    lv_obj_set_size(metadataBox, 320, ARTSIZE);
    lv_obj_set_width(trackName, 320 - METAMARGIN);
    lv_obj_set_width(albumName, 320 - METAMARGIN);
    lv_obj_set_width(artistName, 320 - METAMARGIN);
    lv_obj_set_width(radioName, 320 - METAMARGIN);
  }					
  
    lv_label_set_text(trackName, "");
    lv_label_set_text(artistName, "");
    lv_label_set_text(albumName, "");
    lv_label_set_text(logoName, "");
    lv_label_set_text(radioName, cleanName (getCurrentStationName()));  
}


void displayIdle() {

  char rsText[32];
  int v;

  lockLVGL();

  lv_label_set_text(menuTitle, "Idle");

  paintTopLeftStatus();
  lv_label_set_text(idleZone, getTimeString());

  paintBottomLeftStatus();

  if (locoStarted && isSpotifySource())
    displaySpotifyMetadata();
  else if (isRadioSource())
    displayRadioMetadata();
  else displayLogo ();

  v = getSettingsVolume();

  sprintf(rsText, "%s %d", LV_SYMBOL_VOLUME_MAX, v);
  lv_label_set_text(menuRightStatus, rsText);

  lv_scr_load(idleScreen);

  unlockLVGL();
}

// don't redo metadata every second or it resets scrolling names

void refreshIdle() {

  lockLVGL();
 
  paintTopLeftStatus ();  

  lv_label_set_text(idleZone, getTimeString());
  
  paintBottomLeftStatus();
  
  lv_scr_load(idleScreen);

  unlockLVGL();

}




int idleHandler(int e) {

  int v;

  if (e == KNOBPUSH) {
    callState(MAINMENU);
    displayMain();
    return 1;
  } else if (e == UIREFRESH) {
    displayIdle();
    return 1;
  } else if (e == UITIMER) {

    if (stress)
      displayIdle();
    else
      refreshIdle();

    return 1;
  } else if (e == PLAYSTOPBUTTON) {
    printf("idleHandler Play/Stop\n");
    if (isSpotifySource())
      playPause();
    else if (isRadioSource()) {
      if (isRadioPlaying())
        stopPlay();
      else
		restartCurrentRadio();
    }
    displayIdle();
    return 1;
  } else if (e == NEXTBUTTON) {
    printf("idleHandler Next\n");
    startNext();
    return 1;
  } else if (e == BACKBUTTON) {
    printf("idleHandler Back\n");
    startPrev();
    return 1;
  } else if (e == ROTARYUP) {
    v = getSettingsVolume();
    if (v < 99) {
      v++;
      setSettingsVolume(v);
      setVolume(v);
      displayIdle();
    }
    return 1;
  } else if (e == ROTARYDOWN) {
    v = getSettingsVolume();
    if (v > 0) {
      v--;
      setSettingsVolume(v);
      setVolume(v);
      displayIdle();
    }
    return 1;
  } else if (e == INITPLAYLISTS){
	  printf ("got INITPLAYLISTS\n");
	 loadMyPlaylists (); 
	}
  else {
    printf("idleHandler event = %d\n", e);
  }
  return 0;
}


int isUserEvent(int e) {

  if (e == ROTARYUP) return 1;
  if (e == ROTARYDOWN) return 1;
  if (e == KNOBPUSH) return 1;
  if (e == NEXTBUTTON) return 1;
  if (e == BACKBUTTON) return 1;
  if (e == PLAYSTOPBUTTON) return 1;
  if (e == EJECTBUTTON) return 1;    
  return 0;
}


#include <esp_netif_sntp.h>
#include <esp_sntp.h>

void gotTime (struct timeval *tv){
	printf ("gotTime()\n");
	refreshUI ();
}	

void configureSntp (){
	
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG ("pool.ntp.org");
	config.start = false;
	config.sync_cb = gotTime;
	
	esp_netif_sntp_init (&config);

}

void initSntp (){		
	esp_netif_sntp_start ();
}
	


int doUI(int e) {

  //	printf ("doUI %d\n",e);

  if (reconnectFlag) {
    reconnectFlag = 0;
    newCredentials = 1;
    startWifiSta2();
  }
  if (newIpFlag) {
	  
	printf ("newIPFlag\n");
    newIpFlag = 0;

	configureSntp ();
	initSntp (); 

    startLoco ();
    
	locoStarted = 1; 
   
    startWebserver();      
    if (newCredentials) {
      newCredentials = 0;
      saveWifiCredentials();
    }
    
    if (wifiDisconnectWhileActive){

		printf ("wifiDisconnectWhileActive...try to reconnect\n");
		reconnectTest (wifiDisconnectWhilePlaying);		
		wifiDisconnectWhileActive = 0;
		wifiDisconnectWhilePlaying = 0;			
	}	
    
     
  }
  if (wifiDisconnectedFlag){
	printf ("wifiDisconnectedFlag\n");
	wifiDisconnectedFlag = 0;

	printf ("isACtive = %d isPlaying %d\n",getIsActive(),getStateIsPlaying());
	if (getIsActive()){
		wifiDisconnectWhileActive = 1;
		if (getStateIsPlaying())
			wifiDisconnectWhilePlaying = 1;
		else 	
			wifiDisconnectWhilePlaying = 0;
	}				
	else {	
		wifiDisconnectWhileActive = 0;	
		wifiDisconnectWhilePlaying = 0;	
	}
	
	stopLoco ();
	
	stopWebserver ();
	
	esp_netif_sntp_deinit ();
	
	
  }

  if (e == ENDOFTRACK) {
    printf("doUI () got ENDOFTRACK\n");
    //		if (audioSource == SPOTIFYSOURCE)
    printf("WARNING ENDOFTRACK NIY\n");
    //			nextFlag = 1;
    return 1;
  }

  if (e == UITIMER) {
    if (alertTimer) {
      alertTimer--;
      if (!alertTimer)
        hidePopUp();
    }
  }
  if (isUserEvent(e) && alertTimer) {
    alertTimer = 0;
    hidePopUp();
  }

  //	if ((uiState == UIIDLE) && (e == EJECTHELD)) gotoFactory ();

  if (uiHandlers[uiState](e))
    return 1;


	int tn = getTrackCount ();
	if ((tn < 0)||(tn > 200)){
		printf ("Invalid trackCount\n");
		esp_restart ();
	}	

  return 0;
}


/************************* Wifi Setup *************************/

int pickSSIDIndex = 0;
int pickSSIDOffset = 0;

void gotoPickSSID() {
  pickSSIDIndex = 0;
  pickSSIDOffset = 0;
  startWifiScan();
  callState(PICKSSID);
  displayPickSSID();
}	

char *getSSIDNameOrStatus (int index){
	
	if (isScanning()){
		if (index) return "";
		else return "Scanning";
	}
	else if (index < getSSIDCount())
		return 	getSSIDTitle (index);
	else {
		if (index) return "";
		else return "No Networks";
	}
}		


void displayPickSSID() {


	printf ("displayPickSSID \n");
	
  lockLVGL();	

  lv_label_set_text(menuTitle, "Pick SSID");
  
  lv_label_set_text(menuTR, "");   

  adjustMenuOffset (pickSSIDIndex, &pickSSIDOffset);
  paintItems(pickSSIDIndex, pickSSIDOffset, getSSIDNameOrStatus);
    
  lv_scr_load(menuScreen);

  unlockLVGL();
}

/************************* Wifi Password *************************/


int pickSSIDHandlerU(int e) {

  //	printf ("pickSSIDHandler event %d\n",e);

  if (scanComplete) {
    parseWifiResults(1);
    scanComplete = 0;
  }

  int top = getSSIDCount() - 1;

  if (e == BACKBUTTON) {
    goBack();
  } else if (e == UIREFRESH) {
    displayPickSSID();
  } else if (e == ROTARYUP) {
    if (pickSSIDIndex < top)
      pickSSIDIndex++;
    else
      pickSSIDIndex = 0;
    displayPickSSID();
  } else if (e == ROTARYDOWN) {
    if (pickSSIDIndex > 0)
      pickSSIDIndex--;
    else
      pickSSIDIndex = top;
    displayPickSSID();
  } else if (e == KNOBPUSH) {
    gotoSetPassword();
  } else if (e == NEXTBUTTON) {
    popUp("Connecting");
  } else if (e == EJECTBUTTON) {
    hidePopUp();
  }
  return 1;
}


int setPasswordIndex = 0;
int setPasswordOffset = 0;
char uiPassword[65];

void gotoSetPassword (){
	uiPassword[0] = 0;
	setPasswordIndex = 0;
	initPaintLetters ();
	callState(SETPASSWORD);
    displaySetPassword();
}	

char passwordString [2];
char *alphaChars = "abcdefghijklmnopqrstuvwxyz0123456789 _./,!&-?'()#~$@<>=ABCDEFGHIJKLMNOPQRSTUVWXYZ*+%[]{}^";


char *getPasswordString (int index){
	if (index >= strlen(alphaChars)) return "";
	sprintf (passwordString,"%c",alphaChars[index]);
	return passwordString;
}	


void adjustLetterOffset (int index, int *offset){
	
	int line = index / LETTERSPERROW;
	if (line < *offset) *offset = line;
	else if (line >= *offset + LETTERROWS) *offset = line + 1 - LETTERROWS;
}


void displaySetPassword() {

  char tp[80];

  //	printf ("displaySetPassword ()\n");

  lockLVGL();
  sprintf(tp, "Password: %s", uiPassword);
  lv_label_set_text(alphaTitle, tp);

  adjustLetterOffset(setPasswordIndex, &setPasswordOffset);
  paintLetters(setPasswordIndex, setPasswordOffset, getPasswordString);

  lv_scr_load(alphaScreen);
  unlockLVGL();
}	

int setPasswordHandler (int e){

//	printf ("setPasswordHandler event %d\n",e);
	
  int top = strlen (alphaChars) - 1;		
	

	if (e == UIREFRESH){	
		displaySetPassword ();
	}
	else if (e == ROTARYUP){
		int r = getUpDownEvents();		
//		if (r) printf ("UP Got %d updown events\n",r);	
		setPasswordIndex += (r + 1);
		if (setPasswordIndex < 0) setPasswordIndex += top + 1;
		else if (setPasswordIndex > top) setPasswordIndex -= (top + 1);

		displaySetPassword();		
	}		
	else if (e == ROTARYDOWN){
		int r = getUpDownEvents();		
//		if (r) printf ("DOWN Got %d updown events\n",r);
		setPasswordIndex += (r - 1);
		if (setPasswordIndex < 0) setPasswordIndex += top + 1;
		else if (setPasswordIndex > top) setPasswordIndex -= (top + 1);
	
		displaySetPassword();		
	}
	else if (e == KNOBPUSH){
		int l = strlen (uiPassword);
		if (l< 65) {
			uiPassword[l] = alphaChars[setPasswordIndex];
			uiPassword[l+1] = 0;
		}	
		displaySetPassword();
	}
	else if (e == BACKBUTTON){
		int l = strlen (uiPassword);
		if (l){
			uiPassword[l-1] = 0;
			displaySetPassword();
		}
		else goBack ();	
	}		
	else if (e == NEXTBUTTON){		

		uint8_t *mac = (uint8_t *) getBSSID (pickSSIDIndex);			
		printf ("BSSID %02x,%02x,%02x,%02x,%02x,%02x\n",
					mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		reconnect (getSSIDName(pickSSIDIndex),uiPassword,(uint8_t *)getBSSID(pickSSIDIndex));
		gotoConnecting ();

	}
	else if (e == EJECTBUTTON){
		if (!strcmp (uiPassword,"February27"))
			strcpy (uiPassword,"hmdw7HwJgdgh");
		else if (!strcmp (uiPassword,"hmdw7HwJgdgh"))	
			strcpy (uiPassword,"59b46a6865");
		else if (!strcmp (uiPassword,"59b46a6865"))	
			strcpy (uiPassword,"February27");
		else
			strcpy (uiPassword,"hmdw7HwJgdgh");
		displaySetPassword();		
	}

	return 1;

}

/************************* Connecting Wifi *************************/


int connectingTimer;
int connectingPhase;	// 0 connecting 1 status

void gotoConnecting (){
	popUp ("Connecting");
	callState(CONNECTING);
	connectingTimer = 7;
	connectingPhase = 0;
}	


int connectingHandler (int e){

	printf ("connectingHandler event %d\n",e);
	
	if (isUserEvent (e)){
		hidePopUp ();
		gotoIdle ();
	}
	else if (e == UIREFRESH){
		if (isWifiConnected ()) {
			popUp ("Success");
			connectingPhase = 1;
			connectingTimer = 3;
		}	
	}	
	else if (e == UITIMER){
		if (connectingTimer) connectingTimer--;
		if (!connectingTimer){
			if (connectingPhase){
				hidePopUp ();
				if (isWifiConnected()) gotoIdle ();					
				else goBack ();
			}
			else {
				popUp ("Failed");
				connectingPhase = 1;
				connectingTimer = 4;				
			}	
		}	
	}
	return 1;

}

/************************* Login (Connecting Spotify) *************************/


int loginTimer;
int loginPhase;	// 0 connecting 1 status
PFV loginCallback;


char tmpCredentials[300];
char tmpUsername[300];

int fileExists(char *path) {
  FILE *tmp = fopen(path, "r");
  if (tmp) {
    fclose(tmp);
    return 1;
  }
  return 0;
}


int loadCredentials () {

  tmpCredentials[0] = 0;
  	
  char *name = "/spiffs/credentials";
  if (!fileExists(name))
    return 0;
  FILE *cFile = fopen(name, "r");
  if (!cFile)
    return 0;

  fseeko(cFile, 0, SEEK_END);
  int size = ftello(cFile);
  fseeko(cFile, 0, SEEK_SET);
  
  if (size > 299){					// WARNING
	  printf ("credentials file too big\n");
	  fclose (cFile);
	  return 0;
  }	  
  
  fread(tmpCredentials, 1, size, cFile);
  fclose(cFile);

  tmpCredentials[size] = 0;

  printf("loadCredentials len = %d\n%s\n", size,tmpCredentials);

  return size;
}

int loadUsername () {

  tmpUsername[0] = 0;
  	
  char *name = "/spiffs/username";
  if (!fileExists(name))
    return 0;
  FILE *cFile = fopen(name, "r");
  if (!cFile)
    return 0;

  fseeko(cFile, 0, SEEK_END);
  int size = ftello(cFile);
  fseeko(cFile, 0, SEEK_SET);
  
  if (size > 299){					// WARNING
	  printf ("username file too big\n");
	  fclose (cFile);
	  return 0;
  }	  
  
  fread(tmpUsername, 1, size, cFile);
  fclose(cFile);

  tmpUsername[size] = 0;

  printf("loadUsername len = %d\n%s\n", size,tmpUsername);

  return size;
}


void gotoLogin (PFV cb){
	loginCallback = cb;
	popUp ("Login");
	gotoState(LOGIN);
	loginTimer = 7;
	loginPhase = 0;
	
	if (!loadCredentials()){
		printf ("No credentials file\n");
		popUp ("No Credentials");
		return;
	}
	if (!loadUsername()){
		printf ("No username file\n");
		popUp ("No User Name");
		return;
	}

	connectWithAuth (tmpCredentials,tmpUsername);	
}	


int loginHandler (int e){

	printf ("loginHandler event %d\n",e);
	
	if (isUserEvent (e)){
		hidePopUp ();
		gotoIdle ();
	}
	else if (e == UIREFRESH){
		if (isDealerConnected()) {
			popUp ("Success");
			loginPhase = 1;
			loginTimer = 3;
		}	
	}	
	else if (e == UITIMER){
		if ((loginPhase == 0) && isDealerConnected()){
			popUp ("Success");
			loginPhase = 1;
			loginTimer = 4;		
		}	
		else {	
			if (loginTimer) loginTimer--;
			if (!loginTimer){
				if (loginPhase){
					hidePopUp ();
//					loginCallback ();	
					if (isDealerConnected()) {
						loginCallback ();
//						gotoIdle ();
					}						
					else gotoIdle();
				}
				else {
					popUp ("Failed");
					loginPhase = 1;
					loginTimer = 4;				
				}	
			}
		}	
	}
	return 1;

}
