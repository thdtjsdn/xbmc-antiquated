DIRS=guilib xbmc xbmc/FileSystem xbmc/FileSystem/MusicDatabaseDirectory xbmc/FileSystem/VideoDatabaseDirectory xbmc/cores xbmc/cores/paplayer xbmc/cores/DllLoader xbmc/xbox xbmc/linux xbmc/visualizations xbmc/utils guilib/common guilib/tinyXML xbmc/lib/sqlLite xbmc/lib/libscrobbler xbmc/lib/UnrarXLib

all: XboxMediaCenter

.PHONY : guilib xbmc filesystem musicdatabase videodatabase cores paplayer dllloader xbox linux visualizations utils common tinyxml sqllite libscrobbler unrarxlib

guilib: 
	$(MAKE) -C guilib
xbmc: 
	$(MAKE) -C xbmc
filesystem: 
	$(MAKE) -C xbmc/FileSystem
musicdatabase: 
	$(MAKE) -C xbmc/FileSystem/MusicDatabaseDirectory
videodatabase: 
	$(MAKE) -C xbmc/FileSystem/VideoDatabaseDirectory
cores: 
	$(MAKE) -C xbmc/cores
paplayer: 
	$(MAKE) -C xbmc/cores/paplayer
dllloader: 
	$(MAKE) -C xbmc/cores/DllLoader
xbox: 
	$(MAKE) -C xbmc/xbox
linux: 
	$(MAKE) -C xbmc/linux
visualizations: 
	$(MAKE) -C xbmc/visualizations
utils: 
	$(MAKE) -C xbmc/utils
common: 
	$(MAKE) -C guilib/common
tinyxml: 
	$(MAKE) -C guilib/tinyXML
sqllite: 
	$(MAKE) -C xbmc/lib/sqlLite
libscrobbler: 
	$(MAKE) -C xbmc/lib/libscrobbler
unrarxlib: 
	$(MAKE) -C xbmc/lib/UnrarXLib

XboxMediaCenter: guilib xbmc filesystem musicdatabase videodatabase cores paplayer dllloader xbox linux visualizations utils common tinyxml sqllite libscrobbler unrarxlib
	g++-4.1 -o XboxMediaCenter xbmc/*.o xbmc/settings/*.o guilib/*.o guilib/tinyXML/*.o guilib/common/*.o xbmc/FileSystem/*.o xbmc/FileSystem/VideoDatabaseDirectory/*.o xbmc/FileSystem/MusicDatabaseDirectory/*.o xbmc/visualizations/*.o xbmc/cores/*.o xbmc/cores/paplayer/*.o xbmc/linux/*.o xbmc/lib/sqlLite/*.o xbmc/lib/libscrobbler/*.o xbmc/xbox/*.o xbmc/cores/DllLoader/*.o xbmc/utils/*.o xbmc/lib/UnrarXLib/*.o -lSDL_image -lSDL_gfx -lSDL_mixer -lSDL -llzo -lfreetype -lcdio -lsqlite3 -lfribidi -lGL -lGLU

include Makefile.include
