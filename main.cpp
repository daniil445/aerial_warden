#include "mainwindow.h"

#include <QApplication>
#include <QDir>

//void initGStreamer()
//{
//    auto base = QCoreApplication::applicationDirPath();
//    auto gst  = base + "/gstreamer-1.0";
//    if (QDir(gst).exists())
//    {
//        qDebug() << "Portable GStreamer";
//        qputenv("PATH", (qgetenv("PATH") + ";" + base.toUtf8() + ";" + gst.toUtf8()));
//        qputenv("GST_PLUGIN_PATH", gst.toUtf8());
//        qputenv("GST_PLUGIN_SYSTEM_PATH", gst.toUtf8());
//        qputenv("GST_REGISTRY_FORK", "no");
//    }else{
//        qDebug() << "System GStreamer";
//    }
//#ifdef QT_DEBUG
//    qputenv("GST_DEBUG", "4");
//#endif
//    gst_init(nullptr, nullptr);
//}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
//    initGStreamer();
    return a.exec();
}

