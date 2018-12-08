import QtQuick 2.11
import QtQuick.Window 2.11

import QtWebView 1.1 //oherwise plugin is not found

import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.0
import Axq 1.0

Window {
    id: main
    visible: true
    width: 640
    height: 480
    title: qsTr("Markdown Maker")
    property bool enforceQuit: false
    property var setContent

    Shortcut {
        sequence: "Ctrl+Q"
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
        onContentChanged: setContent(markupMaker.content)
        onShowFileOpen: fileDialog.visible = true;
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

    property var webComponent
    function loadView(isView) {
        var view = "\n
               import QtWebView 1.1
               WebView {"
        var engine = "\n
               import QtWebEngine 1.4
                WebEngineView {"
        var loader = "import QtQuick 2.11\n" + (isView ? view : engine) +
                "id: web
                anchors.fill: parent
                url: 'qrc:/html/index.html'
            }"
        return Qt.createQmlObject(loader, main, "webstuff")
    }

    function runDelay(delay, f) {
        var timer =  Qt.createQmlObject("import QtQuick 2.11\nTimer { running: true; repeat: false; interval:"
                                        + delay + " }", main, "delay")
        timer.triggered.connect(f)
        }

    function setWebContent(content){
        var webComponent = this
        var updateFunction = function() {
            runDelay(1000, function(){
            webComponent.runJavaScript('updateText("' + content + '") ; true;',
                             function(result){
                                if(!result){
                                    console.log("Loading failed")
                                }
                             })})
            }
        if(this.loading){
            this.loadingChanged.connect(function(){
              updateFunction()
            })
        } else if(this.loadProgress >= 100){
            updateFunction()
        } else {
            this.loadProgressChanged.connect(function(){
                if(this.loadProgress >= 100) {
                    updateFunction()
                }})
        }
    }

    Component.onCompleted: {
        var webComponent = loadView(UseWebView)
        setContent = setWebContent.bind(webComponent)
    }

    onClosing: {
        if(!markupMaker.hasOutput() && markupMaker.hasInput() && !enforceQuit){
            close.accepted = false
            saveDialog.visible = true
        }
    }

    MouseArea {
           acceptedButtons:  Qt.RightButton
           anchors.fill: parent
           z: 100
       }
}
