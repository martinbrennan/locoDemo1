/********************************************************
	art.c
	
	This module fetches and decodes spotify album art using a thread
	so that art can be prefetched as soon as the next track is identified
	
	initArtPipeline initialises the process
	fetchArt (url) registers a new url and starts a fetch and decode
	getArt (url) gets the decoded image for that url if available	
	the decoded image is the format used by the data for lv_img_set_src
	
*********************************************************/
#include <stdio.h>
#include "string.h"
#include "jpeg_decoder.h"
#include <esp_heap_caps.h>

#include <loco.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


#include "locoBoard.h"

uint8_t *decodeArtPath(char *artPath);

static QueueHandle_t artQueue = NULL;

// This will fetch one image and no more
// to test the SPI_INTR problem

#define ONESHOT 0
#define IMAGESIZE 100000
	

int fileLength (char *path){
	FILE *f = fopen (path,"r");
	if (!f) return 0;
	fseek (f,0, SEEK_END);
	int r = ftell (f);
	fclose (f);
	return r;
}	


char downloadedArtUrl[200];

	
/***********************************************************************
 New pipelined art mechanism
************************************************************************/

uint8_t *imageBuffers[2];
char *artPipelineUrls[2];
int artInUse[2];
int artWidth[2];
int artHeight[2];

char targetArtUrl[200];

SemaphoreHandle_t artSemaphore;

int refreshOnArt = 0;

#if ONESHOT
int gotArt = 0;
#endif


void lockArt (){
  xSemaphoreTake(artSemaphore, portMAX_DELAY);	
}

void unlockArt (){
  xSemaphoreGive(artSemaphore);  
}

SemaphoreHandle_t artThreadSemaphore;

void lockArtThread (){
  xSemaphoreTake(artThreadSemaphore, portMAX_DELAY);	
}

void unlockArtThread (){
  xSemaphoreGive(artThreadSemaphore);  
}

void artThread() {

//	char *artPath = "/sdcard/art.jpg";
	char *artPath = "/spiffs/art.jpg";
	char url[200];
	int index;

  while (1) {

	lockArtThread ();
	
#if ONESHOT
	if (gotArt){
		continue;
	}	
#endif	
	
	lockArt ();
	
    strcpy(url,targetArtUrl);
        
    
    if ((strcmp (artPipelineUrls[0],url) == 0)||(strcmp (artPipelineUrls[0],url) == 0)){
		unlockArt ();
		continue;
	}	
    
    
// pick a free buffer

	if (!artInUse[0]){
		index = 0;
		*artPipelineUrls[0] = 0;		// invalidate
	}
	else if (!artInUse[1]){
		index = 1;
		*artPipelineUrls[1] = 0;		// invalidate
	}
	else {
		printf ("ERROR artThread both buffers in use\n");
		unlockArt ();
		continue;
	}			
    
    unlockArt ();

    uint8_t *jpgBuffer = heap_caps_malloc(300000, MALLOC_CAP_SPIRAM);
	int r = streamGet(url, (char *)jpgBuffer, 300000);
    if (!r){
		free (jpgBuffer);
		continue;
	}
	int jpgSize = r;	

    esp_jpeg_image_output_t outimg;

    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = jpgBuffer,
        .indata_size = jpgSize,
        .outbuf = imageBuffers[index],
        .outbuf_size = IMAGESIZE,
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_1_8,
        //                                   .out_scale = JPEG_IMAGE_SCALE_1_2,
        .flags = {
            .swap_color_bytes = 1,
        }};

    r = esp_jpeg_decode(&jpeg_cfg, &outimg);

//    printf("first decode result %d\n", r);

    if (r != 0) {
      free(jpgBuffer);
      continue;
    }

//    printf("Decoded dimensions %d x %d\n", outimg.width, outimg.height);

    int height = outimg.height;
    if (height <= 37) {
      jpeg_cfg.out_scale = JPEG_IMAGE_SCALE_1_2;
      esp_jpeg_decode(&jpeg_cfg, &outimg);
      printf("Resized dimensions %d x %d\n", outimg.width, outimg.height);
    } else if (height <= 75) {
      jpeg_cfg.out_scale = JPEG_IMAGE_SCALE_1_4;
      esp_jpeg_decode(&jpeg_cfg, &outimg);
      printf("Resized dimensions %d x %d\n", outimg.width, outimg.height);
    }

//	printf ("art url = %s\n",url);


    free(jpgBuffer);

	lockArt ();
    artWidth[index] = outimg.width;
    artHeight[index] = outimg.height;
    
    strcpy (artPipelineUrls[index],url);	// now valid
    
    unlockArt ();

#if ONESHOT
	gotArt = 1;
#endif	
    
    if (refreshOnArt){
		refreshOnArt = 0;
		refreshUI ();
	}		
  }
}



void initArtPipeline (){
    imageBuffers[0] = heap_caps_malloc(IMAGESIZE, MALLOC_CAP_SPIRAM);
    imageBuffers[1] = heap_caps_malloc(IMAGESIZE, MALLOC_CAP_SPIRAM);
    artPipelineUrls[0] = heap_caps_malloc(200, MALLOC_CAP_SPIRAM);
    artPipelineUrls[1] = heap_caps_malloc(200, MALLOC_CAP_SPIRAM);
    *artPipelineUrls[0] = 0;
    *artPipelineUrls[1] = 0;
    artInUse[0] = 0;
    artInUse[1] = 0;
    
   artSemaphore = xSemaphoreCreateBinary();
   unlockArt ();    

   artThreadSemaphore = xSemaphoreCreateBinary();
//   lockArtThread (); 

#if 0
	static StaticTask_t artTaskBuffer;
	uint8_t *artStack = 	heap_caps_malloc (STACKSIZE,MALLOC_CAP_SPIRAM);	
  xTaskCreateStatic(artThread, "Art Thread", STACKSIZE, NULL, 5, artStack, &artTaskBuffer);
#else
  xTaskCreate(artThread, "Art Thread", STACKSIZE, NULL, 5, NULL);
#endif     
  
}


void fetchArt (char *url){
	
	lockArt ();
			
// do we already have it
	
	if (strcmp(artPipelineUrls[0],url) == 0){
		unlockArt ();
		return;
	}	
	if (strcmp(artPipelineUrls[1],url) == 0){
		unlockArt ();
		return;
	}
	
// tell artThread to start
	
	strcpy (targetArtUrl,url);
	printf ("fetchArt\n");
	unlockArtThread ();

	unlockArt ();
	
}

uint8_t *getArt (char *url){
	
#if ONESHOT
	if (gotArt) return imageBuffers[0];
#endif	
	
	if (!url || !url[0]) return NULL;
		
	
	lockArt ();

	if (strcmp(artPipelineUrls[0],url) == 0){
		artInUse[0] = 1;
		artInUse[1] = 0;
		unlockArt ();
		return imageBuffers[0];
	}	
	if (strcmp(artPipelineUrls[1],url) == 0){
		artInUse[1] = 1;
		artInUse[0] = 0;
		unlockArt ();
		return imageBuffers[1];
	}
	printf ("getArt No match\n");
	
	artInUse[0] = 0;
	artInUse[1] = 0;
	unlockArt ();
	
	fetchArt (url);
	refreshOnArt = 1;	
	return NULL;	
}	

// returns the index of the entry with artInUse

int getArtIndex (){
	
#if ONESHOT
	if (gotArt) return 0;
#endif
	
	if (artInUse[0]) return 0;
	else if (artInUse[1]) return 1;
	printf ("ERROR getArtIndex - nothing inUse\n");
	return 0;
}	
	
int getArtWidth (){
	lockArt ();
	int i = getArtIndex();
	int r = artWidth[i];
	unlockArt ();
	return r;
}

int getArtHeight (){
	lockArt ();
	int i = getArtIndex();
	int r = artHeight[i];
	unlockArt ();
	return r;
}


void newArt() {

  printf("newArt ()\n");

  char *url = getNextArtUrl();

  if (url && url[0])
    fetchArt(url);
}



	
