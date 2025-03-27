This is a fully featured example program to show off the capabilities of the Loco Software Library
It runs on the Loco V4 PCB with an attached Loco Display V1

Find out more at www.brennan.co.uk/loco

The program is built using Espressifs ESP-IDF framework.

Instructions to install ESP-IDF and build the program can be found here

https://brennan.co.uk/pages/esp-idf-installation-and-build

The program presents a fairly self explanatory menu based UI on the TFT.

Push and turn the rotary to use the menu
The push button switches have the following uses

SW4 - Back
SW3 - Next
SW2 - Play/Stop
SW5 - unused

The first step is to Setup Wifi - you can find this in the main menu.
Loco will store the chosen Wifi network and password (in spiffs) and will automatically connect on restart
The TFT will display the connection status and IP address at the top left and the time at the top right.
If you type the IP address into a web browser you will see the Loco Web UI

You can change the code for the UI and Web UI to customise things.
The UI is mainly defined in ui.c - it wil help to have some familiarity with LVGL to understand and build on this.
The web UI is defined in locoPage.cpp and the backend is implemented by web.c

You can use the web UI to search for, browse and play internet radio stations provided by Airable.
You can assign stations to Favourites from the web UI and these can be played from the menu.

You can play Spotify through Loco using its Spotify Connect capability.
Spotify Connect is available to the 260 million Spotify subscribers. It is not available on the free tier.
When Loco is connected to the Wifi network you can see it if you tap the devices icon on the Spotify app.
Loco has the name "Beauty" - like the quark - you can change this in main.c

When you play music through Loco it displays the album art and metadata (track, artist and album) as well as duration and other details

When you select Loco your Spotify app will send your user name and credentials to Loco.
This allows Loco to play music without the app in future.

As an example.

If you navigate to My Playlists you will see all your Spotify Playlists. Loco will play the selected playlist when you hit play.
Next, Back and Play/Stop operate as you would expect while playing.
Turning the rotary when not in in a menu will adjust the volume

The Playlists function is coded outside the Loco Library and uses the Spotify API. You will find the code for this in api.c

You can add your own functions using this as a template.











