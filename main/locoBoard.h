#ifdef __cplusplus
 extern "C" {
#endif

// locoboard.c

int isMounted ();
int mountSd(int format);
void initSDDetect ();
int getSDDetect ();
void sdPoll();

void initSpiffs();

void locoAudioInit(void);
void startAudioThread();
void setVolume (int volume);
void codecRestart (int volume);

void toggleTestTone();
void toggleStereoTestTone();
void testToneOff();


void initButtons();

void lcdInit();
void set_backlight_brightness(int percent);


// ui.c

void uiInit ();
int doUI (int e);
void refreshUI ();

// UI Events

typedef enum {
	NOTHING,					// 0
	NEXTBUTTON,
	PLAYSTOPBUTTON,
	BACKBUTTON,
	EJECTBUTTON,	
	KNOBPUSH,
	ROTARYUP,
	ROTARYDOWN,
	UITIMEOUT,
					
	UIREFRESH,
	UIALERT,
	UITIMER,
	
	EJECTHELD,
	BACKBUTTONHELD,
	
	ENDOFTRACK,
	INITPLAYLISTS
	
} uievent;


typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	int flag;
} waitlock_t;

void addEvent (int e);
int getEvent ();

// web.c

void startWebserver();
void stopWebserver();
extern httpd_handle_t server2;


// art.c

void newArt ();
uint8_t *getArt (char *url);
void initArtPipeline ();
int getArtWidth ();
int getArtHeight ();
void fetchArt (char *url);


// main.c - TODO cleanup

void postStartSearchResult (int index);
void postStartFavourite (int index);
void postNext ();
void postPrev ();
void postPlayPause ();
void postStartPlaylist (int index);

cJSON *getFavourites ();
int playFavourite (int index);

int getMyPlaylistsTotal ();
int getMyPlaylistsCount ();
char *getPlaylistName (int index);
char *getLoadedPlaylistName (int index);

char *getPlaylistUri (int index);

void initialiseWifi();
int startWifiFromCredentials();

void lockLVGL ();
void unlockLVGL ();
char *cleanName (char *s);

void keepAwake();
uint64_t millis ();

cJSON *getMyPlaylists(int offset, int limit);
cJSON *getMyShows(int offset, int limit);

int getSettingsVolume();
void setSettingsVolume(int volume);
void memDebugB ();
void savePresets ();

void locoCallback(int e);
extern int newIpFlag;
extern int wifiDisconnectedFlag;

void cdump(unsigned char *buf, int len);
void startWifiSta2();

#ifdef __cplusplus
}
#endif

