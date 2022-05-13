#!/bin/sh

echo "Starting Wicked Engine Editor"

cd @CMAKE_INSTALL_PREFIX@/@EDITOR_INSTALL_FOLDER@
exec ./WickedEngineEditor $@
