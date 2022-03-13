#!/usr/bin/sh

sudo rm /usr/bin/wicked-engine
sudo cp wicked-engine.sh /usr/bin/wicked-engine

sudo chmod +x /usr/bin/wicked-engine

sudo rm /usr/share/applications/wicked-engine.desktop
sudo cp wicked-engine.desktop /usr/share/applications/wicked-engine.desktop
