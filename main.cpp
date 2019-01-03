#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFileInfo>
#include <QQmlContext>
#include <QLibraryInfo>

#ifdef WEBVIEW
#include <QtWebView/QtWebView>
#else
#include <QtWebEngine>
#endif

#include "axq.h"
#include "markdownmaker.h"

#include <QDebug>

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Markus Mertama");

    QQmlApplicationEngine engine;
    Axq::registerTypes(); //For QML

    engine.rootContext()->setContextProperty("UseWebView",
#ifdef WEBVIEW
     true);
     QtWebView::initialize();
#else
    false);
    QtWebEngine::initialize();
#endif

    QScopedPointer<MarkdownMaker> mm(new MarkdownMaker);


    QStringList files;
    bool isQuiet = false;

    for(auto i = 1 ; i < argc; i++) {
        QString arg(argv[i]);
        if(arg.startsWith('-')) {
            auto p = arg.mid(1);
            if(p == "o" && i < argc - 1) {
                mm->setOutput(argv[i + 1]);
            }
            if(p == "q") {
                isQuiet = true;
            }
        } else {
            files.append(arg);
        }
    }

    if(!isQuiet) {
        engine.rootContext()->setContextProperty("markupMaker", mm.data());
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
        Q_ASSERT(!engine.rootObjects().isEmpty());
    } else {
        QObject::connect(mm.data(), &MarkdownMaker::allMade, mm.data(), [&app]() {
            app.quit();
        });
    }

    if(files.isEmpty()){
        emit mm->showFileOpen();
    }

    if(!mm->hasOutput()){
        mm->setOutput("");
    }

    for(const auto& f : files) {
        const QFileInfo file(f);
        if(file.suffix().toLower() == "md") {
            mm->addMarkupFile(file.absoluteFilePath());
        } else {
            mm->addSourceFile(file.absoluteFilePath());
        }
    }

    qDebug() << QLibraryInfo::location(QLibraryInfo::PluginsPath);

    return app.exec();
}
