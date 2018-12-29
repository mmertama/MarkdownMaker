regexp="Qt\/([0-9]+)\.([0-9]+)\.([0-9]+)"
if [[ "$QTDIR" =~ $regexp ]]
then 
    ver=${BASH_REMATCH[1]}
    major=${BASH_REMATCH[2]}
    minor=${BASH_REMATCH[3]}
	axqbuild=../../build-Axq-Desktop_Qt_${ver}_${major}_${minor}_MSVC2017_64bit-Release/lib/release
	build=../../build-mdmaker-Desktop_Qt_${ver}_${major}_${minor}_MSVC2017_64bit-Release
	cp ${axqbuild}/Axq.dll  ${build}/release/
    $QTDIR/bin/windeployqt --release -qmldir=.. $build/release/mdmaker.exe
else
    echo Error, QTDIR is not set
fi
