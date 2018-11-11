regexp="Qt\/([0-9]+)\.([0-9]+)\.([0-9]+)"
if [[ "$QTDIR" =~ $regexp ]]
then 
    ver=${BASH_REMATCH[1]}
    major=${BASH_REMATCH[2]}
    minor=${BASH_REMATCH[3]}
    build=../builds/build-mdmaker-Desktop_Qt_${ver}_${major}_${minor}_clang_64bit2-Release
    $QTDIR/clang_64/bin/macdeployqt $build/mdmaker.app -libpath=../../axq/lib -qmldir=.. -dmg
    mv $build/mdmaker.dmg .
else
    echo Error, QTDIR is not set
fi
