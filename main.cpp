#include "mainwindow.h"
#include "app_title.h"
#include "collapsiblegroups.h"
#include <QApplication>
#include <QIcon>
#include <QTranslator>
#include <QLocale>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QPalette>
#include <csignal>
#include <exception>
#include <functional>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

namespace {
volatile std::sig_atomic_t requestedSignal=0;
void requestShutdown(int value) { requestedSignal=value; }

void applyDarkTitleBar(QWidget *widget)
{
#ifdef Q_OS_WIN
    if (!widget || !widget->isWindow() || !widget->internalWinId()
            || (widget->windowType() != Qt::Window && widget->windowType() != Qt::Dialog)) return;
    const HWND handle = reinterpret_cast<HWND>(widget->internalWinId());
    const BOOL dark = TRUE;
    // Numeric attributes keep compatibility with older Windows SDK headers.
    DwmSetWindowAttribute(handle, 20, &dark, sizeof(dark));
    const COLORREF background = RGB(13, 17, 23);
    const COLORREF text = RGB(230, 237, 243);
    // Explicit caption colors are supported on Windows 11.
    DwmSetWindowAttribute(handle, 35, &background, sizeof(background));
    DwmSetWindowAttribute(handle, 36, &text, sizeof(text));
#else
    Q_UNUSED(widget);
#endif
}

class SafeApplication : public QApplication
{
public:
    using QApplication::QApplication;
    std::function<void(const QString&)> onFailure;
    bool notify(QObject *receiver,QEvent *event) override
    {
        try {
            const bool result = QApplication::notify(receiver,event);
            if (event->type() == QEvent::Show || event->type() == QEvent::WinIdChange)
                applyDarkTitleBar(qobject_cast<QWidget*>(receiver));
            return result;
        }
        catch (const std::exception &error) {
            if (onFailure) onFailure(QString::fromUtf8(error.what()));
        } catch (...) {
            if (onFailure) onFailure("Unknown exception in Qt event handler");
        }
        return false;
    }
};
}

int main(int argc,char *argv[])
{
    SafeApplication app(argc,argv);
    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));
    QTranslator russianTranslation;
    if (russianTranslation.load(":/translations/aerialwarden_ru_RU.qm"))
        app.installTranslator(&russianTranslation);
    else
        qWarning() << "Could not load the embedded Russian translation";
    app.setWindowIcon(QIcon(":/img/app_icon.png"));
    CollapsibleGroups collapsibleGroups(&app);
    app.installEventFilter(&collapsibleGroups);
    app.setStyle("Fusion");
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#0D1117"));
    palette.setColor(QPalette::WindowText, QColor("#E6EDF3"));
    palette.setColor(QPalette::Base, QColor("#161B22"));
    palette.setColor(QPalette::AlternateBase, QColor("#1C222B"));
    palette.setColor(QPalette::Text, QColor("#E6EDF3"));
    palette.setColor(QPalette::Button, QColor("#1C222B"));
    palette.setColor(QPalette::ButtonText, QColor("#E6EDF3"));
    palette.setColor(QPalette::Highlight, QColor("#2F81F7"));
    palette.setColor(QPalette::HighlightedText, QColor("#E6EDF3"));
    palette.setColor(QPalette::Link, QColor("#58A6FF"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#8B949E"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#8B949E"));
    app.setPalette(palette);
    QFile theme(":/style/theme.qss");
    if (theme.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    else qWarning() << "Could not load interface theme:" << theme.errorString();
    MainWindow window;
    window.setWindowTitle(QString::fromUtf8(APP_WINDOW_TITLE));
    bool handlingFailure=false;
    app.onFailure=[&](const QString &message) {
        qCritical().noquote()<<"Application exception:"<<message;
        if (handlingFailure) return;
        handlingFailure=true;
        QTimer::singleShot(0,&window,[&]() {
            window.shutdownAndSave();
            app.exit(1);
        });
    };
    QObject::connect(&app,&QCoreApplication::aboutToQuit,&window,
                     &MainWindow::shutdownAndSave,Qt::DirectConnection);
    std::signal(SIGINT,requestShutdown);
    std::signal(SIGTERM,requestShutdown);
    QTimer signalTimer;
    QObject::connect(&signalTimer,&QTimer::timeout,&window,[&]() {
        if (!requestedSignal) return;
        requestedSignal=0;
        window.shutdownAndSave();
        app.quit();
    });
    signalTimer.start(100);
    window.show();
    try { return app.exec(); }
    catch (...) {
        qCritical()<<"Exception leaving the application event loop";
        window.shutdownAndSave();
        return 1;
    }
}
