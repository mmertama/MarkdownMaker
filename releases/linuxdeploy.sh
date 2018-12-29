regexp="Qt\/([0-9]+)\.([0-9]+)\.([0-9]+)"
if [[ "$QTDIR" =~ $regexp ]]
then 
    ver=${BASH_REMATCH[1]}
    major=${BASH_REMATCH[2]}
    minor=${BASH_REMATCH[3]}
    axqbuild=../../build-Axq-Desktop_Qt_${ver}_${major}_${minor}_GCC_64bit?-Release
    mdmbuild=../../build-mdmaker-Desktop_Qt_${ver}_${major}_${minor}_GCC_64bit?-Release
    
    export QT_SELECT=qt${ver}.${major}.${minor}
    

    cp ${axqbuild}/lib/libAxq.so.1  MarkdownMaker-x86_64.AppDir/usr/lib/
    cp ${mdmbuild}/mdmaker          MarkdownMaker-x86_64.AppDir/usr/bin/


    linuxdeployqt-continuous-x86_64.AppImage MarkdownMaker-x86_64.AppDir/usr/share/applications/MarkdownMaker.desktop -appimage -always-overwrite -qmldir=..  -extra-plugins=webview
    # ln -s /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so  MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so
	cp -s /usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so  MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so
    
    if [[ ! -f MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so ]] ; then
    echo Warning MarkdownMaker-x86_64.AppDir/usr/lib/libsoftokn3.so does not exists, that may be an issue
fi
    
else
    echo Error, QTDIR is not set
fi



