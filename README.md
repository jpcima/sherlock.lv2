# Sherlock

## An investigative LV2 plugin bundle

### Webpage 

Get more information at: [http://open-music-kontrollers.ch/lv2/sherlock](http://open-music-kontrollers.ch/lv2/sherlock)

### Build status

[![Build Status](https://travis-ci.org/OpenMusicKontrollers/sherlock.lv2.svg)](https://travis-ci.org/OpenMusicKontrollers/sherlock.lv2)

### Plugins

#### Atom Inspector

##### Screenshot 

![Screeny](http://open-music-kontrollers.ch/lv2/sherlock/sherlock_atom_inspector.png "")

### Dependencies

* [LV2](http://lv2plug.in) (LV2 Plugin Standard)
* [EFL](http://docs.enlightenment.org/stable/elementary/) (Enlightenment Foundation Libraries)
* [Elementary](http://docs.enlightenment.org/stable/efl/) (Lightweight GUI Toolkit)

### Build / install

	git clone https://github.com/OpenMusicKontrollers/sherlock.lv2.git
	cd sherlock.lv2
	mkdir build
	cd build
	cmake -DCMAKE_C_FLAGS="-std=gnu99" ..
	make
	sudo make install
