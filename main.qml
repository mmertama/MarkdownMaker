import QtQuick 2.8
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtWebView 1.1
import QtWebChannel 1.0
import QtQuick.Dialogs 1.0
import Axq 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello Axq")
    property bool enforceQuit: false

    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: {
            enforceQuit = true;
            Qt.quit()
        }
    }

    Connections{
        target: Axq
        onError: console.log(errorString)
    }

    Connections {
        target: markupMaker
        onContentChanged: webView.setContent(markupMaker.content)
        onShowFileOpen: fileDialog.visible = true;
    }

    onClosing: {
        if(!markupMaker.hasOutput() && markupMaker.hasInput() && !enforceQuit){
            close.accepted = false
            saveDialog.visible = true
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        nameFilters: ["Headers (*.h *.hxx *.hh)", "All files (*)"]
        selectMultiple: true
        visible: false
        onAccepted: {
            markupMaker.setSourceFiles(fileUrls);
            visible = false
        }
        onRejected: {
            visible = false
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save to..."
        folder: shortcuts.home
        nameFilters: ["Markups (*.md)", "All files (*)"]
        selectMultiple: false
        selectExisting: false
        visible: false
        onAccepted: {
            markupMaker.setOutput(fileUrl);
            Qt.quit();
        }
        onRejected: {
            visible = false
        }
    }


    WebView{
        id: webView
        anchors.fill: parent
        url: "qrc:/index.html"
        property var onLoaded
        function setContent(content){
            var load = function() {
                runJavaScript('updateText("' + content + '")')
            }
            if(webView.loading || loadProgress < 100){
                onLoaded = load
            } else {
                load()
            }
        }
        onLoadingChanged: {
            if(onLoaded && loadProgress == 100){
                onLoaded();
            }
        }
    }
}
