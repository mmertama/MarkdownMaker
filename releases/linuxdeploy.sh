#!/bin/bash
regexp="Qt\/([0-9]+)\.([0-9]+)\.([0-9]+)"
if [[ "$QTDIR" =~ $regexp ]]
then 
    ver=${BASH_REMATCH[1]}
    major=${BASH_REMATCH[2]}
    minor=${BASH_REMATCH[3]}
    # static build axqbuild=$(ls -d "../../build-Axq-Desktop_Qt_${ver}_${major}_${minor}_GCC_64bit"?"-Release")
    mdmbuild=$(ls -d "../../build-mdmaker-Desktop_Qt_${ver}_${major}_${minor}_GCC_64bit"?"-Release")
    
    if [[ ! -d $mdmbuild ]] ; then
        echo Error: $mdmbuild not found >&2
        exit 1
    fi
    
    export QT_SELECT=qt${ver}.${major}.${minor}
    
    # static build cp ${axqbuild}/lib/libAxq.so.1  MarkdownMaker-x86_64.AppDir/usr/lib/
    cp ${mdmbuild}/mdmaker          MarkdownMaker-x86_64.AppDir/usr/bin/
    
    if [[ ! -f MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so ]] ; then
     # ln -s /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so  MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so
	    cp -s /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so  MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so
        if [[ ! -f MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so ]] ; then
            echo Warning: MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so does not exists, that may be an issue >&2
        fi
    fi      

    linuxdeployqt-continuous-x86_64.AppImage MarkdownMaker-x86_64.AppDir/usr/share/applications/MarkdownMaker.desktop -appimage -always-overwrite -qmldir=.. -extra-plugins=webview
  
else
    echo Error: QTDIR is not set >&2
    exit
fi



