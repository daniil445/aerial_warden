#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_interface_settings.h"
#include <QDialog>
#include "joystickcontroller.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QPointer>
#include <QSettings>
#include <QThread>
#include <QSignalBlocker>
#include <QTimer>
#include <QStatusBar>
#include <QCloseEvent>
#include <QEventLoop>
#include <QComboBox>
#include <QPushButton>
#include <QPainter>
#include <QPixmap>
#include <RtspReceiver.h>

namespace {
QIcon recordingIcon(bool showDot)
{
    QPixmap image(160, 48);
    image.setDevicePixelRatio(2.0);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    if (showDot) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#F85149"));
        painter.drawEllipse(QRectF(2, 6, 12, 12));
    }
    QFont font("Segoe UI", 12, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor("#E6EDF3"));
    painter.drawText(QRect(22, 0, 58, 24), Qt::AlignVCenter | Qt::AlignLeft, "REC");
    painter.end();
    // Provide the same colors for the disabled state during finalization.
    QIcon icon;
    for (QIcon::Mode mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected})
        icon.addPixmap(image, mode, QIcon::Off);
    return icon;
}

int selectableZoom(int value, bool infrared)
{
    return qBound(1, value, infrared ? 6 : 50);
}

int steppedZoom(int value, int steps, bool infrared)
{
    return selectableZoom(value + steps, infrared);
}

QString formatFrameTimestamp(quint64 timestamp)
{
    return QString::number(timestamp / 100000.0, 'f', 3);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->openGLWidget_IR->setOverlaysEnabled(false);
    ui->openGLWidget_IR->setPreviewBorder(true);
    ui->openGLWidget_RGB->setMinimumSize(0, 0);
    ui->openGLWidget_RGB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->display_widget->setMinimumSize(0, 0);
    QPalette videoBackground = ui->display_widget->palette();
    videoBackground.setColor(QPalette::Window, Qt::black);
    ui->display_widget->setPalette(videoBackground);
    ui->display_widget->setAutoFillBackground(true);
    connect(ui->openGLWidget_RGB, &VideoWidget::update_size, this,
            [this](QSize) { scheduleRightBoxAlignment(); });
    connect(ui->openGLWidget_IR, &VideoWidget::update_size, this,
            [this](QSize) { scheduleRightBoxAlignment(); });
    ui->openGLWidget_RGB->installEventFilter(this);
    ui->openGLWidget_IR->installEventFilter(this);
    ui->right_box->installEventFilter(this);
    ui->left_box->installEventFilter(this);
    ui->bottom_box->installEventFilter(this);
    ui->display_widget->installEventFilter(this);
    scheduleRightBoxAlignment();
    auto *interfaceDialog = new QDialog(this);
    interfaceUi = new Ui::InterfaceSettings;
    interfaceUi->setupUi(interfaceDialog);
    connect(ui->actionInterfaceSettings, &QAction::triggered, this, [interfaceDialog]() {
        interfaceDialog->show();
        interfaceDialog->raise();
        interfaceDialog->activateWindow();
    });
    connect(ui->swap_cameras, &QPushButton::clicked, this, [this]() {
        setPrimaryCamera(!m_primaryInfrared);
    });

    //// Video
    connect(interfaceUi->cb_show_moves, &QPushButton::toggled, [=](bool val) {ui->openGLWidget_RGB->motion_visible=val;});
    connect(interfaceUi->cb_show_planes, &QPushButton::toggled,[=](bool val) {ui->openGLWidget_RGB->motion_planes=val;});
    connect(interfaceUi->cb_show_drones, &QPushButton::toggled,[=](bool val) {ui->openGLWidget_RGB->motion_drones=val;});
    connect(interfaceUi->cb_show_birds, &QPushButton::toggled, [=](bool val) {ui->openGLWidget_RGB->motion_birds=val;});
    connect(interfaceUi->cb_show_cars, &QPushButton::toggled,  [=](bool val) {ui->openGLWidget_RGB->motion_cars=val;});
    connect(interfaceUi->cb_show_mans, &QPushButton::toggled,  [=](bool val) {ui->openGLWidget_RGB->motion_mans=val;});
    connect(interfaceUi->cb_show_aim, &QPushButton::toggled,   [=](bool val) {ui->openGLWidget_RGB->show_aim=val;});
    connect(interfaceUi->cb_show_degree, &QPushButton::toggled,[=](bool val) {ui->openGLWidget_RGB->show_degree=val;});
    connect(interfaceUi->cb_show_text, &QPushButton::toggled,  [=](bool val) {ui->openGLWidget_RGB->show_text=val;});
    connect(interfaceUi->cb_show_mouse_steering, &QPushButton::toggled, this, [this](bool visible) {
        ui->openGLWidget_RGB->setMouseSteeringOverlayVisible(visible);
        ui->openGLWidget_IR->setMouseSteeringOverlayVisible(visible);
    });
    connect(ui->controls, &motion_controller::update_screen_click,  [=](bool val) {ui->openGLWidget_RGB->enable_click=val;});
    connect(ui->controls, &motion_controller::update_stabilization_click, [=](bool val) {sender->sendCmd("move_stab",val);});


    connect(ui->joystick_zoom_mode, &QComboBox::currentIndexChanged, this, [this](int count) {
            ui->zoom_speed_spin->setEnabled(count);
    });

    dialog_sett= new conn_settings;
    connect(dialog_sett, &conn_settings::connect_to, this, &MainWindow::try_to_connect);
    connect(ui->actionSettings, &QAction::triggered, dialog_sett, &QWidget::show);
    connect(ui->actionInitialize_rotator, &QAction::triggered,[=]() {sender->sendCmd("init");});

    rtcp_receiver_RGB = new RtspReceiver(this);
    connect(rtcp_receiver_RGB, &RtspReceiver::frameReady, ui->openGLWidget_RGB, &VideoWidget::setFrame);

    rtcp_receiver_IR = new RtspReceiver(this);
    connect(rtcp_receiver_IR, &RtspReceiver::frameReady, ui->openGLWidget_IR, &VideoWidget::setFrame);
    connect(rtcp_receiver_RGB, &RtspReceiver::frameReady, this, [this](quint64, const QImage &frame) {
        if (frame.isNull()) return;
        m_rgbFramesReceived = qMin(2, m_rgbFramesReceived + 1);
        m_rgbFrameAge.restart();
    });
    connect(rtcp_receiver_IR, &RtspReceiver::frameReady, this, [this](quint64, const QImage &frame) {
        if (frame.isNull()) return;
        m_irFramesReceived = qMin(2, m_irFramesReceived + 1);
        m_irFrameAge.restart();
    });
    connect(rtcp_receiver_RGB, &QThread::finished, this, [this]() {
        m_rgbFramesReceived = 0;
        m_rgbFrameAge.invalidate();
    });
    connect(rtcp_receiver_IR, &QThread::finished, this, [this]() {
        m_irFramesReceived = 0;
        m_irFrameAge.invalidate();
    });


    sender = new CommandSender(this);

    m_joystick = new JoystickController(this);
    connect(m_joystick, &JoystickController::precisionModeChanged,
            ui->openGLWidget_RGB, &VideoWidget::setPrecisionMode);
    connect(m_joystick, &JoystickController::precisionModeChanged,
            ui->openGLWidget_IR, &VideoWidget::setPrecisionMode);
    connect(ui->joystick_zoom_mode, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_joystick->setContinuousZoom(index == 1);
    });
    connect(m_joystick, &JoystickController::zoomMotion, this, [this](int direction) {
        if (!joystickZoomCamera.isEmpty()) {
            sender->sendZoomSpeed(joystickZoomCamera, 0);
            joystickZoomCamera.clear();
        }
        if (direction) {
            joystickZoomCamera = m_primaryInfrared ? "ir_cam" : "rgb_cam";
            const double magnitude = qMax(0.01, (ui->zoom_speed_spin->value() - 1) / 7.0);
            sender->sendZoomSpeed(joystickZoomCamera, direction * magnitude);
        }
    });
    m_joystick->speedLimits = [this]() { return ui->controls->joystickSpeedLimits(); };
    m_joystick->inputAllowed = [this]() {
        return !m_shutdownStarted && isActiveWindow()
            && !ui->openGLWidget_RGB->mouseSteeringActive()
            && !ui->openGLWidget_IR->mouseSteeringActive();
    };
    connect(m_joystick, &JoystickController::statusChanged, this, [this](const QString& text) {
        statusBar()->showMessage(text, 6000);
    });
    connect(m_joystick, &JoystickController::controlStarted, this, [this]() {
        const QSignalBlocker blocker(ui->btn_follow);
        ui->btn_follow->setChecked(false);
        ui->btn_follow->setText(tr("follow object"));
        sender->sendUserUpdate(QJsonObject{{"target", QJsonObject{{"id", -1}, {"filled", false}}},
                                          {"ptz", QJsonObject{{"stop", true}}}});
    });
    connect(m_joystick, &JoystickController::velocityChanged, this, [this](QPointF velocity) {
        if (velocity.isNull()) { sender->sendCmd("stop"); return; }
        sender->sendMove(QString::number(velocity.x(), 'f', 4) + "|" + QString::number(velocity.y(), 'f', 4));
    });
    connect(m_joystick, &JoystickController::controlStopped, this, [this]() { sender->sendCmd("stop"); });

    connect(m_joystick, &JoystickController::zoomStep, this, [this](int steps) {
        const bool infrared = m_primaryInfrared;
        const int value = steppedZoom(ui->zoom_spin->value(), steps, infrared);
        if (value == ui->zoom_spin->value()) return;
        setZoomSelection(value);
        sender->sendZoom(infrared ? "ir_cam" : "rgb_cam", value);
    });

    for (VideoWidget* video : {ui->openGLWidget_RGB, ui->openGLWidget_IR}) {
        connect(video, &VideoWidget::mouseSteeringStarted, this, [this]() {
            m_joystick->suspend();
            const QSignalBlocker blocker(ui->btn_follow);
            ui->btn_follow->setChecked(false);
            ui->btn_follow->setText(tr("follow object"));
            sender->sendUserUpdate(QJsonObject{
                {"target", QJsonObject{{"id", -1}, {"filled", false}}},
                {"ptz", QJsonObject{{"stop", true}}}
            });
        });
        connect(video, &VideoWidget::mouseSteeringStep, this, [this](QPoint sectors) {
            if (sectors.isNull()) { sender->sendCmd("stop"); return; }
            const QPointF maximum = ui->controls->mouseSpeedLimits();
            sender->sendMove(QString::number(maximum.x() * sectors.x() / 10.0, 'f', 4)
                             + "|" + QString::number(maximum.y() * sectors.y() / 10.0, 'f', 4));
        });
        connect(video, &VideoWidget::mouseSteeringStopped, this, [this]() {
            sender->sendCmd("stop");
        });
        connect(video, &VideoWidget::mouseZoomSteps, this, [this, video](int steps) {
            const bool infrared = video == ui->openGLWidget_IR;
            const int value = steppedZoom(ui->zoom_spin->value(), steps, infrared);
            if (value == ui->zoom_spin->value()) return;
            setZoomSelection(value);
            sender->sendZoom(infrared ? "ir_cam" : "rgb_cam", value);
        });
    }

    const auto sendSelectedZoom = [this]() {
        const QString camera = m_primaryInfrared
                                    ? "ir_cam" : "rgb_cam";
        sender->sendZoom(camera, ui->zoom_spin->value());
    };
    ui->zoom_spin->setKeyboardTracking(false);

    auto *focusLimits = ui->focus_near_limit;
    auto *applyFocusLimit = ui->apply_focus_near_limit;
    auto *autoFocus = ui->autofocus_toggle;
    connect(ui->ir_cut_toggle, &QPushButton::clicked, this, [this](bool enabled) {
        sender->sendUserUpdate(QJsonObject{{"rgb_cam", QJsonObject{{"ir_cut", enabled}}}});
        ui->ir_cut_toggle->setText(enabled ? tr("IR-cut filter: ON") : tr("IR-cut filter: OFF"));
    });
    connect(applyFocusLimit, &QPushButton::clicked, this, [this, focusLimits]() {
        sender->sendUserUpdate(QJsonObject{{"rgb_cam", QJsonObject{{"focus_near_m", focusLimits->currentText().toInt()}}}});
    });
    connect(autoFocus, &QPushButton::clicked, this, [this, autoFocus](bool enabled) {
        m_joystick->suspend();
        sender->sendUserUpdate(QJsonObject{{"rgb_cam", QJsonObject{{"autofocus", enabled}}}});
    });
    const auto sendFocusMotion = [this, autoFocus](int direction) {
        if (direction && m_primaryInfrared) return;
        sender->sendUserUpdate(QJsonObject{{"rgb_cam", QJsonObject{{"focus_move", direction}}}});
        if (direction) {
            const QSignalBlocker blocker(autoFocus);
            autoFocus->setChecked(false);
        }
    };
    auto *focusTimer = new QTimer(this);
    focusTimer->setInterval(250);
    const auto stopSliderFocus = [this, focusTimer, sendFocusMotion]() {
        if (!focusTimer->isActive()) return;
        focusTimer->stop();
        const QSignalBlocker blocker(ui->focus_slider);
        ui->focus_slider->setValue(0);
        sendFocusMotion(0);
    };
    connect(ui->focus_slider, &QSlider::valueChanged, this,
            [this, focusTimer, sendFocusMotion](int direction) {
        m_joystick->suspend();
        sendFocusMotion(direction);
        if (direction) focusTimer->start();
        else focusTimer->stop();
    });
    connect(ui->focus_slider, &QSlider::sliderReleased, this, stopSliderFocus);
    connect(focusTimer, &QTimer::timeout, this, [this, stopSliderFocus, sendFocusMotion]() {
        if (!isActiveWindow() || m_shutdownStarted
                || m_primaryInfrared) {
            stopSliderFocus();
            return;
        }
        sendFocusMotion(ui->focus_slider->value());
    });
    connect(m_joystick, &JoystickController::focusMotion, this,
            [focusTimer, sendFocusMotion](int direction) {
        if (!focusTimer->isActive()) sendFocusMotion(direction);
    });
    connect(autoFocus, &QPushButton::clicked, this, stopSliderFocus);
    const auto updateFocusControls = [this, focusLimits, applyFocusLimit, autoFocus, stopSliderFocus]() {
        stopSliderFocus();
        m_joystick->suspend();
        const bool rgb = !m_primaryInfrared;
        focusLimits->setEnabled(rgb);
        applyFocusLimit->setEnabled(rgb);
        autoFocus->setEnabled(rgb);
        ui->focus_slider->setEnabled(rgb);
        ui->ir_cut_toggle->setEnabled(rgb);
    };
    connect(this, &MainWindow::activeCameraChanged, this, [updateFocusControls](int) { updateFocusControls(); });
    updateFocusControls();


    auto *zoomAlgorithmTimer = new QTimer(this);
    zoomAlgorithmTimer->setInterval(5000);
    zoomAlgorithmTimer->setTimerType(Qt::PreciseTimer);

    connect(ui->controls, &motion_controller::send_zoom, this, [this, sendSelectedZoom](int move) {
        const bool infrared = m_primaryInfrared;
        setZoomSelection(steppedZoom(ui->zoom_spin->value(), move, infrared));
        sendSelectedZoom();
    });

    connect(ui->zoom_slider, &QSlider::valueChanged, this, [this](int value) {
        const bool infrared = m_primaryInfrared;
        const int selected = selectableZoom(value, infrared);
        setZoomSelection(selected);
    });
    connect(ui->zoom_slider, &QSlider::sliderReleased, this, sendSelectedZoom);

    connect(ui->zoom_spin, &QSpinBox::valueChanged, this, [this](int value) {
        setZoomSelection(value);
    });
    connect(ui->zoom_spin, &QSpinBox::editingFinished, this, sendSelectedZoom);

    connect(this, &MainWindow::activeCameraChanged, this, [this](int) {
        m_joystick->suspend();
        syncActiveCameraZoom();
    });
    syncActiveCameraZoom();

    recorder = new VideoRecorder(this);
    stream_rgb_recorder= new StreamRecorder(this);
    stream_ir_recorder= new StreamRecorder(this);
    m_streamPartTimer.setSingleShot(true);
    m_streamPartTimer.setInterval(600000);
    connect(&m_streamPartTimer, &QTimer::timeout, this, &MainWindow::rotateStreamPart);
    connect(stream_rgb_recorder, &StreamRecorder::segmentLimitReached,
            this, &MainWindow::rotateStreamPart, Qt::QueuedConnection);
    connect(stream_ir_recorder, &StreamRecorder::segmentLimitReached,
            this, &MainWindow::rotateStreamPart, Qt::QueuedConnection);
    const QIcon recOn = recordingIcon(true);
    const QIcon recOff = recordingIcon(false);
    for (auto *button : {ui->self_record, ui->stream_record}) {
        button->setIconSize(QSize(80, 24));
        button->setIcon(recOff);
    }
    auto *recBlink = new QTimer(this);
    recBlink->setInterval(500);
    recBlink->setTimerType(Qt::PreciseTimer);
    connect(recBlink, &QTimer::timeout, this,
            [this, recOn, recOff, phase = false]() mutable {
        phase = !phase;
        ui->self_record->setIcon(recorder->isRecording() && phase ? recOn : recOff);
        const bool streamsRecording = !m_stoppingStream
                && stream_rgb_recorder->isRecording() && stream_ir_recorder->isRecording();
        ui->stream_record->setIcon(streamsRecording && phase ? recOn : recOff);
    });
    recBlink->start();
    auto *recordingAvailability = new QTimer(this);
    recordingAvailability->setInterval(250);
    connect(recordingAvailability, &QTimer::timeout, this, &MainWindow::updateRecordingAvailability);
    recordingAvailability->start();
    updateRecordingAvailability();
    m_streamFinishTimer.setInterval(50);
    connect(&m_streamFinishTimer,&QTimer::timeout,this,&MainWindow::checkStreamTelemetryTail);
    const auto recordingFailure=[this](const QString &message) {
        m_rotateStreamPending = false;
        qWarning().noquote()<<"Recording failed:"<<message;
        ui->stream_record->setToolTip(message);
        statusBar()->showMessage(tr("Recording failed: ")+message,30000);
        if (m_collectingStreamTelemetry && !m_stoppingStream)
            stopStreamRecording();
    };
    connect(stream_rgb_recorder,&StreamRecorder::recordingError,
            this,recordingFailure,Qt::QueuedConnection);
    connect(stream_ir_recorder,&StreamRecorder::recordingError,
            this,recordingFailure,Qt::QueuedConnection);
    //// linkers
    ui->openGLWidget_RGB->link_storages(&storage_move,&storage);
    ui->openGLWidget_IR->link_storages(&storage_move,&storage);

    connect(this, &MainWindow::update_focus,ui->openGLWidget_RGB, &VideoWidget::update_focus);
    connect(this, &MainWindow::update_focus,ui->openGLWidget_IR, &VideoWidget::update_focus);
    connect(ui->openGLWidget_RGB,&VideoWidget::update_list,this, &MainWindow::update_list);
    connect(ui->openGLWidget_IR,&VideoWidget::update_list,this, &MainWindow::update_list);

    connect(ui->controls, &motion_controller::moveCommand,this,&MainWindow::sendMoveCommand);
    connect(ui->controls, &motion_controller::moveToCommand,this,&MainWindow::sendMoveToCommand);

    connect(ui->controls, &motion_controller::change_cam, this,
            [this](int index) { setPrimaryCamera(index == 1); });

    UDP_receiver = new CommandReceiver(this);
    connect(UDP_receiver, &CommandReceiver::metaObjectsReceived,ui->openGLWidget_RGB, &VideoWidget::setMeta);
    connect(UDP_receiver, &CommandReceiver::metaObjectsReceived,ui->openGLWidget_IR, &VideoWidget::setMeta);
    connect(UDP_receiver, &CommandReceiver::metaObjectsReceived,this, &MainWindow::cacheTelemetry);
    connect(UDP_receiver, &CommandReceiver::metaObjectsReceived, this,
            [this](const QJsonObject &packet) {
        m_rgbZoom = packet.value("rgb").toObject().value("zoom").toDouble(0);
        const QJsonObject rgb = packet.value("rgb").toObject();
        const QJsonValue focus = rgb.value("focus_position");
        const qint64 reportId = rgb.value("focus_updated_ms").toInteger();
        if (reportId != m_focusReportId) {
            m_focusReportId = reportId;
            m_focusReportAge.restart();
        }
        ui->focus_position_value->setText(focus.isDouble() && m_focusReportAge.isValid()
                && m_focusReportAge.elapsed() < 5000
                ? QString::number(focus.toInt()) : QString::fromUtf8("—"));
        m_irZoom = packet.value("ir").toObject().value("zoom").toInt(0);
        syncActiveCameraZoom();
    });
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_f_z,this,&MainWindow::update_meta);
    connect(this, &MainWindow::update_zoom,ui->controls, &motion_controller::update_zoom);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p_g_m,this,&MainWindow::update_meta_pos);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p_g_m,ui->controls, &motion_controller::update_aim);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_d,this,&MainWindow::update_distance);
    connect(ui->openGLWidget_RGB, &VideoWidget::moveToCommand,this,&MainWindow::sendMoveToCommandPos);
    connect(ui->openGLWidget_IR, &VideoWidget::moveToCommand,this,&MainWindow::sendMoveToCommandPos);

    QSettings settings("config.ini", QSettings::IniFormat);
    QStringList name= {"Plane","Bird","Drone","Human","Car"};
    QVariantList perfect_size= {78,114,26,36,56};
    settings.setValue("obj_name" ,name);
    settings.setValue("obj_size" ,perfect_size);
    // Restore after all visibility handlers are connected; apply defaults too.
    for (auto *box : interfaceDialog->findChildren<QPushButton*>()) {
        if (!box->isCheckable()) continue;
        const bool checked = settings.value("interface/" + box->objectName(), box->isChecked()).toBool();
        box->setChecked(checked);
        emit box->toggled(checked);
        connect(box, &QPushButton::toggled, this, [box](bool value) {
            QSettings conf("config.ini", QSettings::IniFormat);
            conf.setValue("interface/" + box->objectName(), value);
            conf.sync();
            if (conf.status() != QSettings::NoError)
                qWarning() << "Could not save interface settings to" << conf.fileName();
        });
    }
    ui->joystick_zoom_mode->setCurrentIndex(1);
    ui->controls->restorePreferences(settings);
    const int zoomMode = settings.value("pref/joystick_zoom_mode", ui->joystick_zoom_mode->currentIndex()).toInt();
    // if (zoomMode >= 0 && zoomMode < ui->joystick_zoom_mode->count())
        // ui->joystick_zoom_mode->setCurrentIndex(zoomMode);
    m_joystick->setContinuousZoom(ui->joystick_zoom_mode->currentIndex() == 1);
    ui->zoom_speed_spin->setEnabled(ui->joystick_zoom_mode->currentIndex() == 1);
    ui->openGLWidget_RGB->enable_click = ui->controls->screenClickEnabled();
    const auto savePreferences = [this]() {
        QSettings prefs("config.ini", QSettings::IniFormat);
        prefs.setValue("pref/joystick_zoom_mode", ui->joystick_zoom_mode->currentIndex());
        ui->controls->savePreferences(prefs);
        prefs.sync();
        if (prefs.status() != QSettings::NoError)
            qWarning() << "Could not save preferences to" << prefs.fileName();
    };
    connect(ui->joystick_zoom_mode, &QComboBox::currentIndexChanged, this, savePreferences);
    connect(ui->controls, &motion_controller::preferencesChanged, this, savePreferences);
}

MainWindow::~MainWindow()
{
    shutdownAndSave();
    delete interfaceUi;
    delete ui;
}

void MainWindow::shutdownAndSave()
{
    if (m_shutdownStarted) return;
    m_shutdownStarted=true;
    m_rotateStreamPending = false;
    m_streamPartTimer.stop();
    ui->stream_record->setEnabled(false);
    if (m_recordFrameConnection) disconnect(m_recordFrameConnection);
    if (m_streamTelemetryConnection) disconnect(m_streamTelemetryConnection);
    stopStreamRecording();
    if (m_stoppingStream) {
        // Keep UDP delivery alive during the bounded tail wait, even on quit.
        QEventLoop drain;
        QTimer poll;
        connect(&poll,&QTimer::timeout,&drain,[this,&drain]() {
            if (!m_stoppingStream) drain.quit();
        });
        poll.start(20);
        QTimer::singleShot(1800,&drain,&QEventLoop::quit);
        drain.exec(QEventLoop::ExcludeUserInputEvents);
        finishStreamRecording();
    }
    closeTelemetryFile(m_interfaceTelemetry);
    recorder->stop();
    rtcp_receiver_RGB->stop();
    rtcp_receiver_IR->stop();
    rtcp_receiver_RGB->wait();
    rtcp_receiver_IR->wait();
    if (m_streamChecker) {
        m_streamChecker->requestInterruption();
        m_streamChecker->wait();
    }
}

void MainWindow::try_to_connect(QStringList url_main)
{
    if (m_shutdownStarted) return;
    m_rgbFramesReceived = m_irFramesReceived = 0;
    m_rgbFrameAge.invalidate();
    m_irFrameAge.invalidate();
    updateRecordingAvailability();
    main_ip=url_main[0];
    main_port=url_main[1].toInt();
    meta_port=url_main[2].toInt();
    rgb_port=url_main[3].toInt();
    ir_port=url_main[4].toInt();
    rtcp_receiver_RGB->setUrl("rtsp://"+main_ip+":"+QString::number(rgb_port)+"/rgb");
//    rtcp_receiver_RGB->setUrl("rtsp://admin:admin@192.168.26.108/cam/realmonitor?channel=1&subtype=0");
//    rtcp_receiver_RGB->setUrl("rtsp://admin:admin@192.168.2.108:554");
    rtcp_receiver_RGB->start();
    rtcp_receiver_IR ->setUrl("rtsp://"+main_ip+":"+QString::number(ir_port)+"/ir");
    rtcp_receiver_IR->start();

    qDebug()<<"update stream"<<"rtsp://"<<main_ip+":"+QString::number(rgb_port);
    qDebug()<<"update stream"<<"rtsp://"<<main_ip+":"+QString::number(ir_port);
    qDebug()<<"update meta"<<"UDP"<<main_ip<<url_main[1]<<url_main[2];

    UDP_receiver->bind(main_ip,meta_port);
    sender->sendAddress(main_ip,main_port);
    sender->sendIp();
    sender->sendCmd("move_stab", ui->controls->stabilizationEnabled());
    sender->sendUserUpdate(QJsonObject{{"rgb_cam", QJsonObject{{"profile", "day"}}}});
    const QSignalBlocker irCutBlocker(ui->ir_cut_toggle);
    ui->ir_cut_toggle->setChecked(true);
    ui->ir_cut_toggle->setText(tr("IR-cut filter: ON"));
}

void MainWindow::recordIncomingFrame(quint64 frameIndex, quint64 timestamp, QImage frame)
{
    if (!recorder->isRecording())
        return;
    constexpr quint64 limitTicks = 600ULL * 100000;
    if (m_haveInterfacePartTimestamp && (timestamp < m_interfacePartFirstTimestamp
            || timestamp - m_interfacePartFirstTimestamp >= limitTicks)) {
        closeTelemetryFile(m_interfaceTelemetry);
        recorder->stop();
        if (!startInterfacePart()) {
            ui->self_record->setChecked(false);
            on_self_record_clicked(false);
            statusBar()->showMessage(tr("Could not open the next screen recording segment"), 10000);
            return;
        }
    }
    if (!m_haveInterfacePartTimestamp) {
        m_interfacePartFirstTimestamp = timestamp;
        m_haveInterfacePartTimestamp = true;
    }

    if (m_haveRecordedSourceIndex && frameIndex != m_lastRecordedSourceIndex + 1) {
        qWarning() << "Decoded frame gap during interface recording: expected"
                   << m_lastRecordedSourceIndex + 1 << "received" << frameIndex;
    }
    m_lastRecordedSourceIndex = frameIndex;
    m_haveRecordedSourceIndex = true;

    if (recorder->writeFrame(composeRecorderFrame(frame), timestamp)) {
        enqueueTelemetryFrame(m_interfaceTelemetry,timestamp);
    }
}

void MainWindow::recordStreamTelemetryFrame(quint64 timestamp)
{
    enqueueTelemetryFrame(m_streamTelemetry,timestamp);
}

void MainWindow::cacheTelemetry(const QJsonObject &packet)
{
    const QJsonObject rgb = packet.value("rgb").toObject();
    if (!rgb.contains("ts"))
        return;

    const quint64 timestamp = rgb.value("ts").toInteger();
    // A telemetry packet is valid even when there are no detections. Preserve
    // it and normalize a missing/null AI field to an explicit empty array.
    QJsonObject packetToStore = packet;
    if (!packetToStore.value("ai").isArray())
        packetToStore.insert("ai", QJsonArray());
    m_telemetryPackets.insert(timestamp, packetToStore);
    if (m_collectingStreamTelemetry && !m_streamTelemetryPackets.contains(timestamp))
        m_streamTelemetryPackets.insert(timestamp,packetToStore);


    // Ten seconds of telemetry at 30 Hz is enough for normal UDP/video jitter.
    while (m_telemetryPackets.size() > 300)
        m_telemetryPackets.erase(m_telemetryPackets.begin());

    flushTelemetryLines(m_interfaceTelemetry,false);
    // Stream telemetry is finalized from the MP4 frame list after EOS.
}

bool MainWindow::findTelemetryPacket(quint64 timestamp, bool allowEarlier,
                                     QJsonObject *packet) const
{
    auto it = m_telemetryPackets.constFind(timestamp);
    if (it != m_telemetryPackets.cend()) {
        *packet = it.value();
        return true;
    }

    if (!allowEarlier || m_telemetryPackets.isEmpty())
        return false;

    // Telemetry is produced slower than video. Keep the newest packet that is
    // not newer than this video frame, matching the on-screen box behaviour.
    it = m_telemetryPackets.upperBound(timestamp);
    if (it == m_telemetryPackets.cbegin())
        return false;
    --it;
    *packet = it.value();
    return true;
}

bool MainWindow::openTelemetryFile(TelemetryOutput &output,const QString &filename)
{
    closeTelemetryFile(output);
    output.file.setFileName(filename);
    if (!output.file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;
    output.pendingLines.clear();
    output.missingLines=0;
    output.firstEntry=true;
    output.file.write("{\n");
    return true;
}

void MainWindow::enqueueTelemetryFrame(TelemetryOutput &output,quint64 timestamp)
{
    if (!output.file.isOpen())
        return;
    output.pendingLines.enqueue({timestamp});
    flushTelemetryLines(output,false);
}

void MainWindow::flushTelemetryLines(TelemetryOutput &output,bool force)
{
    if (!output.file.isOpen())
        return;

    constexpr int syncDelayFrames = 15;
    while (!output.pendingLines.isEmpty()) {
        const PendingTelemetryLine &line = output.pendingLines.head();
        const bool deadlineReached = force || output.pendingLines.size() > syncDelayFrames;
        QJsonObject packet;
        if (!findTelemetryPacket(line.sourceTimestamp, deadlineReached, &packet)) {
            if (!deadlineReached)
                break;
            ++output.missingLines;
        }

        QByteArray lineBytes;
        if (!output.firstEntry)
            lineBytes += ",\n";
        lineBytes += "  \"";
        lineBytes += formatFrameTimestamp(line.sourceTimestamp).toUtf8();
        lineBytes += "\":";
        lineBytes += QJsonDocument(packet).toJson(QJsonDocument::Compact);
        output.file.write(lineBytes);
        output.firstEntry = false;
        output.pendingLines.dequeue();
    }
}

void MainWindow::closeTelemetryFile(TelemetryOutput &output)
{
    if (!output.file.isOpen())
        return;

    flushTelemetryLines(output,true);
    output.file.write("\n}\n");
    output.file.flush();
    output.file.close();
}

QImage MainWindow::composeRecorderFrame(const QImage &rgbFrame)
{
    QImage frame = this->grab().toImage().convertToFormat(QImage::Format_RGB888);
    QPoint pos =ui->openGLWidget_RGB->mapTo(this,QPoint(0,0));
    QPainter painter(&frame);
    const QImage renderedVideo = ui->openGLWidget_RGB->renderFrameForRecording(rgbFrame);
    if (!renderedVideo.isNull())
        painter.drawImage(pos, renderedVideo);
    painter.end();
    if(frame.size()!=m_recordSize)
        frame=frame.scaled( m_recordSize,Qt::IgnoreAspectRatio, Qt::FastTransformation);
    return frame;
}

void MainWindow::scheduleRightBoxAlignment()
{
    if (m_rightBoxAlignmentPending) return;
    m_rightBoxAlignmentPending = true;
    QTimer::singleShot(0, this, [this]() {
        m_rightBoxAlignmentPending = false;
        alignRightBox();
    });
}

void MainWindow::alignRightBox()
{
    VideoWidget *mainVideo = m_primaryInfrared ? ui->openGLWidget_IR : ui->openGLWidget_RGB;
    VideoWidget *previewVideo = m_primaryInfrared ? ui->openGLWidget_RGB : ui->openGLWidget_IR;
    const QRect available = ui->display_widget->contentsRect();
    QSize source = mainVideo->sourceSize();
    if (source.isEmpty()) source = QSize(16, 9);
    if (!available.isEmpty()) {
        QRect fitted(QPoint(0, 0), source.scaled(available.size(), Qt::KeepAspectRatio));
        fitted.moveCenter(available.center());
        if (mainVideo->geometry() != fitted) mainVideo->setGeometry(fitted);
    }
    if (available.isEmpty()) return;
    constexpr int margin = 2;
    const auto displayBounds = [this, available](QWidget *widget) {
        QWidget *parent = widget->parentWidget();
        if (!parent) return QRect();
        const QPoint origin = parent->mapFromGlobal(
                ui->display_widget->mapToGlobal(available.topLeft()));
        return QRect(origin, available.size());
    };
    const QRect left = displayBounds(ui->left_box);
    ui->left_box->move(left.left() + margin, left.top() + margin);
    ui->left_box->raise();

    const QRect bottom = displayBounds(ui->bottom_box);
    ui->bottom_box->move(
        qMax(bottom.left(), bottom.left() + (bottom.width() - ui->bottom_box->width()) / 2),
        qMax(bottom.top(), bottom.bottom() + 1 - ui->bottom_box->height() - margin));
    ui->bottom_box->raise();

    const QRect preview = displayBounds(previewVideo);
    previewVideo->move(qMax(preview.left(), preview.right() + 1 - previewVideo->width() - margin),
                       preview.top() + margin);
    previewVideo->raise();

    const QRect right = displayBounds(ui->right_box);
    const QPoint previewBottom = ui->right_box->parentWidget()->mapFromGlobal(
            previewVideo->mapToGlobal(QPoint(0, previewVideo->height())));
    ui->right_box->move(qMax(right.left(), right.right() + 1 - ui->right_box->width() - margin),
                        qMax(right.top() + margin, previewBottom.y() + margin));
    ui->right_box->raise();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->openGLWidget_RGB || object == ui->openGLWidget_IR
            || object == ui->right_box || object == ui->left_box
            || object == ui->bottom_box || object == ui->display_widget) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show
                || event->type() == QEvent::ParentChange || event->type() == QEvent::LayoutRequest
                || (event->type() == QEvent::Move
                    && object == (m_primaryInfrared ? ui->openGLWidget_IR : ui->openGLWidget_RGB)))
            scheduleRightBoxAlignment();
    }
    return QMainWindow::eventFilter(object, event);
}

void MainWindow::setPrimaryCamera(bool infrared)
{
    if (m_primaryInfrared == infrared) return;
    VideoWidget *primaryVideo = m_primaryInfrared ? ui->openGLWidget_IR : ui->openGLWidget_RGB;
    VideoWidget *secondaryVideo = m_primaryInfrared ? ui->openGLWidget_RGB : ui->openGLWidget_IR;
    // Stop motion before reparenting OpenGL widgets or changing the control target.
    m_joystick->suspend();
    sender->sendUserUpdate(QJsonObject{{"rgb_cam", QJsonObject{{"focus_move", 0}}}});
    const QRect primaryGeometry = primaryVideo->geometry();
    const QRect secondaryGeometry = secondaryVideo->geometry();
    const QSize minimum = primaryVideo->minimumSize();
    const QSize maximum = primaryVideo->maximumSize();
    const QSizePolicy policy = primaryVideo->sizePolicy();
    primaryVideo->setMinimumSize(secondaryVideo->minimumSize());
    primaryVideo->setMaximumSize(secondaryVideo->maximumSize());
    primaryVideo->setSizePolicy(secondaryVideo->sizePolicy());
    secondaryVideo->setMinimumSize(minimum);
    secondaryVideo->setMaximumSize(maximum);
    secondaryVideo->setSizePolicy(policy);
    secondaryVideo->setGeometry(primaryGeometry);
    primaryVideo->setGeometry(secondaryGeometry);
    secondaryVideo->show();
    primaryVideo->show();
    m_primaryInfrared = infrared;
    ui->openGLWidget_RGB->setOverlaysEnabled(!infrared);
    ui->openGLWidget_IR->setOverlaysEnabled(infrared);
    ui->openGLWidget_RGB->setPreviewBorder(infrared);
    ui->openGLWidget_IR->setPreviewBorder(!infrared);
    scheduleRightBoxAlignment();
    emit activeCameraChanged(infrared ? 1 : 0);
}

void MainWindow::setZoomSelection(int value)
{
    const bool infrared = m_primaryInfrared;
    const int selected = selectableZoom(value, infrared);
    const QSignalBlocker spinBlocker(ui->zoom_spin);
    const QSignalBlocker sliderBlocker(ui->zoom_slider);
    ui->zoom_spin->setValue(selected);
    ui->zoom_slider->setValue(selected);
}

void MainWindow::syncActiveCameraZoom()
{
    const bool infrared = m_primaryInfrared;
    const int maximum = infrared ? 6 : 50;
    const int minimum = 1;
    const int step = 1;
    const QString cameraName = infrared ? "IR" : "RGB";
    const double reportedZoom = infrared ? m_irZoom : m_rgbZoom;
    const bool available = std::isfinite(reportedZoom) && reportedZoom >= 1
            && reportedZoom <= (infrared ? 6 : 50);
    const QSignalBlocker blocker(ui->zoom_slider);
    const QSignalBlocker spinBlocker(ui->zoom_spin);
    ui->zoom_slider->setRange(minimum, maximum);
    ui->zoom_slider->setSingleStep(1);
    ui->zoom_slider->setPageStep(1);
    ui->zoom_slider->setTickInterval(1);

    ui->zoom_spin->setRange(minimum, maximum);
    ui->zoom_spin->setSingleStep(step);
    ui->widget_cam_zoom->setTitle(QString("Zoom %1").arg(cameraName));
    // ui->widget_cam_zoom->setEnabled(available);
    // ui->zoom_slider->setEnabled(available);
    if (!available) {
        setZoomSelection(ui->zoom_spin->value());
        ui->zoom_info->setText(QString("%1: — / %2").arg(cameraName).arg(maximum));
        return;
    }
    const double cameraZoom = qBound(1.0, reportedZoom, infrared ? 6.0 : 50.0);
    if (!ui->zoom_slider->isSliderDown() && !ui->zoom_spin->hasFocus()
            && !ui->openGLWidget_RGB->mouseSteeringActive()
            && !ui->openGLWidget_IR->mouseSteeringActive()) {
        setZoomSelection(qRound(cameraZoom));
    }
    ui->zoom_info->setText(QString("%1: %2 | %3").arg(cameraName).arg(cameraZoom)
                         .arg(infrared ? "1–6" : "1–50"));
    emit update_zoom(cameraZoom);
}

void MainWindow::update_meta(quint64 frame, int zoom)
{
    Q_UNUSED(zoom);
    ui->l_fps->setText(formatFrameTimestamp(frame));
}

void MainWindow::update_meta_pos(QVector2D ang, QVector3D pos, QVector3D gyro, QVector3D mag)
{
    ui->l_angle->setText(QString("a = %0 b = %1").arg(ang.x()).arg(ang.y()));
    ui->l_position->setText((QString("x = %0 y = %1 z = %2").arg(pos.x()).arg(pos.y()).arg(pos.z())));
    ui->l_gyro->setText((QString("x = %0 y = %1 z = %2").arg(gyro.x(), 6,'d',3).arg(gyro.y(), 6,'d',3).arg(gyro.z(), 6,'d',3)));
    ui->l_magnit->setText((QString("x = %0 y = %1 z = %2").arg(mag.x(), 6,'d',3).arg(mag.y(), 6,'d',3).arg(mag.z(), 6,'d',3)));
    if(test_counter<0)return;
    test_counter++;
    if(test_counter>900){
        test_counter=0;
    }else if(test_counter==10){
        sender->sendMove(QString("%0|%1").arg(10).arg(0));
    }else if(test_counter==450){
        sender->sendMove(QString("%0|%1").arg(-10).arg(0));
    }
}

void MainWindow::update_distance(double dist)
{
    ui->l_dist->setText(QString::number(dist,'d',3)+" m");
}

void MainWindow::sendMoveCommand(const QString& cmd, double speed, bool pressed)
{
    qDebug()<<"sendMove"<<cmd<<speed<<pressed;
    if(cmd=="stop"){
        sender->sendCmd(cmd);
    }else if(cmd=="home"){
        sender->sendCmd(QString("%0|%1").arg(speed).arg(speed));
    }else if(cmd=="aim"){
        sender->sendCmd(cmd);
    }else{
        QString temp;
        if(!pressed)speed=0;
        temp_speed_x=cmd=="right"?speed:(cmd=="left"?-speed:temp_speed_x);
        temp_speed_y=cmd=="down"?speed:(cmd=="up"?-speed:temp_speed_y);
        temp= QString("%0|%1").arg(temp_speed_x).arg(temp_speed_y);
        sender->sendMove(temp);
//        qDebug()<<"sendMove"<<temp<<pressed<<speed<<temp_speed_x<<temp_speed_y;
    }
}

void MainWindow::sendMoveToCommandPos(QPointF pos)
{
    sendMoveToCommand(pos.x(),pos.y(),ui->controls->x_speed,ui->controls->y_speed);
}

void MainWindow::sendMoveToCommand(double pos_x, double pos_y, double speed_x, double speed_y)
{
    if(speed_x==0 && speed_y==0 ) sender->sendCmd("stop");
    else sender->sendMoveTo(QString("%0|%1|%2|%3").arg(pos_x).arg(-pos_y).arg(speed_x).arg(speed_y));
}

void MainWindow::update_list()
{
    foreach(QString key, storage_move.keys()){
        QList<QListWidgetItem*> items = ui->obj_list_move->findItems(key, Qt::MatchExactly);
        if (items.isEmpty())  ui->obj_list_move->addItem(key);
    }

    foreach(QString key, storage.keys()){
        QList<QListWidgetItem*> items = ui->obj_list->findItems(key, Qt::MatchExactly);
        if (items.isEmpty())  ui->obj_list->addItem(key);
    }


    for (int i = ui->obj_list_move->count() - 1; i >= 0; --i)
    {
        QListWidgetItem* item = ui->obj_list_move->item(i);
        if(!storage_move.contains(item->text())) delete ui->obj_list_move->takeItem(i);
    }

    for (int i = ui->obj_list->count() - 1; i >= 0; --i)
    {
        QListWidgetItem* item = ui->obj_list->item(i);
        if(!storage.contains(item->text())) delete ui->obj_list->takeItem(i);
    }

    if(ui->obj_list->findItems(focus_name, Qt::MatchExactly).length()==0){
//        ui->btn_follow->setEnabled(false);
//        ui->btn_follow->clicked(false);
//        on_btn_follow_clicked(false);
    }
}


void MainWindow::on_obj_list_itemClicked(QListWidgetItem *item)
{
    focus_name=item->text();
    ui->btn_follow->setEnabled(true);
//    sender->sendTarget(focus_name,focus_name.split(' ').last().toInt(),ui->f_count->value());
//    if(storage.contains(focus_name))emit update_focus(storage.value(focus_name));
}


void MainWindow::on_obj_list_move_itemClicked(QListWidgetItem *item)
{
    focus_name=item->text();
    ui->btn_follow->setEnabled(true);
    //    sender->sendTarget(focus_name.split(' ').last().toInt(),ui->f_count->value(),1,QPointF(5,0),QPoint(0,0));
    if(storage_move.contains(focus_name))emit update_focus(storage_move.value(focus_name));
}


void MainWindow::on_btn_follow_clicked(bool checked)
{
    if(focus_name==""){
        return;
    }
    if(checked){
        ui->btn_follow->setText(tr("unfollow object"));
        sender->sendTarget(focus_name.split(' ').last().toInt(),ui->f_count->value(),0,QPointF(0,0),QPoint(0,0));

        // follower->scenario="follow";
    }else{
        ui->btn_follow->setText(tr("follow object"));
        sender->sendCmd("stop");
        sender->sendTarget(-1,ui->f_count->value(),0,QPointF(0,0),QPoint(0,0));
        // follower->scenario="";
    }
}

void MainWindow::on_btn_search_clicked(bool checked)
{
    if(checked){
        ui->btn_search->setText(tr("stop search object"));
        // follower->scenario="search";
    }else{
        ui->btn_search->setText(tr("search objects"));
//        sender->sendCmd("stop");
        // follower->scenario="";
        // follower->pause();
    }
}


void MainWindow::on_btn_dist_clicked(bool checked)
{
    if(checked){
        if(ui->dist_freq->currentIndex()!=0){
            ui->btn_dist->setText(tr("stop calculate distance"));
        }else{
            ui->btn_dist->blockSignals(true);
            ui->btn_dist->click();
            ui->btn_dist->blockSignals(false);
        }
        sender->sendCmd("range_find",ui->dist_freq->currentIndex()+1);
    }else{
        ui->btn_dist->setText(tr("calculate distance"));
        sender->sendCmd("range_stop");
    }
}


bool MainWindow::startInterfacePart()
{
    const QString base = m_interfaceSessionBase
            + QString("_part%1").arg(++m_interfacePart, 3, 10, QLatin1Char('0'));
    if (!recorder->start(m_recordSize, 30, base)) return false;
    if (!openTelemetryFile(m_interfaceTelemetry, base + ".telemetry.json")) {
        recorder->stop();
        return false;
    }
    m_haveInterfacePartTimestamp = false;
    return true;
}

void MainWindow::rotateStreamPart()
{
    if (m_shutdownStarted || m_stoppingStream || !m_collectingStreamTelemetry) return;
    m_rotateStreamPending = true;
    stopStreamRecording();
}

bool MainWindow::bothVideoStreamsLive() const
{
    return rtcp_receiver_RGB->isRunning() && rtcp_receiver_IR->isRunning()
            && m_rgbFramesReceived >= 2 && m_irFramesReceived >= 2
            && m_rgbFrameAge.isValid() && m_irFrameAge.isValid()
            && m_rgbFrameAge.elapsed() <= 1500 && m_irFrameAge.elapsed() <= 1500;
}

void MainWindow::updateRecordingAvailability()
{
    if (m_shutdownStarted) return;
    const bool ready = bothVideoStreamsLive();
    ui->self_record->setEnabled(false);
    ui->stream_record->setEnabled(!m_streamChecker && !m_stoppingStream
                                 && (m_collectingStreamTelemetry || ready));
    const QString waiting = tr("Frames from both RGB and IR streams are required to start recording");
    for (auto *button : {ui->stream_record}) {
        if (!ready && !button->isChecked()) button->setToolTip(waiting);
        else if (button->toolTip() == waiting) button->setToolTip({});
    }
}

bool MainWindow::requireLiveVideo()
{
    if (bothVideoStreamsLive()) return true;
    statusBar()->showMessage(tr("Recording not started: wait for video from both RGB and IR cameras"), 6000);
    return false;
}

void MainWindow::on_self_record_clicked(bool checked)
{
    if (checked) {
        ui->self_record->setChecked(false);
        return;
    }
    if (checked && !requireLiveVideo()) {
        ui->self_record->setChecked(false);
        updateRecordingAvailability();
        return;
    }
    if(checked){
        m_recordSize=this->grab().size();
        m_recordSize.setWidth(m_recordSize.width() & ~1);
        m_recordSize.setHeight(m_recordSize.height() & ~1);
        m_interfaceSessionBase = QDir(QCoreApplication::applicationDirPath()).filePath(
            "video-telemetry/record_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"));
        m_interfacePart = 0;
        if (!startInterfacePart()) {
            ui->self_record->setChecked(false);
            return;
        }
        ui->self_record->setText(tr("stop self record"));
        m_haveRecordedSourceIndex = false;
        m_recordFrameConnection = connect(
            rtcp_receiver_RGB, &RtspReceiver::frameDecoded,
            this, &MainWindow::recordIncomingFrame,
            Qt::QueuedConnection);
    }else{
        ui->self_record->setText(tr("start self record"));
        if (m_recordFrameConnection) {
            disconnect(m_recordFrameConnection);
            m_recordFrameConnection = {};
        }
        closeTelemetryFile(m_interfaceTelemetry);
        qInfo() << "Interface recording frame count:" << recorder->frameCount();
        qInfo() << "Telemetry recording:" << recorder->outputBasePath()+".telemetry.json"
                << "missing packets:" << m_interfaceTelemetry.missingLines;
        recorder->stop();
    }
}


void MainWindow::checkAndStartStreamRecording()
{
    if (m_shutdownStarted || m_stoppingStream || m_collectingStreamTelemetry || m_streamChecker)
        return;
    if (!requireLiveVideo()) {
        ui->stream_record->setChecked(false);
        updateRecordingAvailability();
        return;
    }
    const QString rgbUrl="rtsp://"+main_ip+":"+QString::number(rgb_port)+"/rgb";
    const QString irUrl="rtsp://"+main_ip+":"+QString::number(ir_port)+"/ir";
    ui->stream_record->setEnabled(false);
    ui->stream_record->setText(tr("checking RGB / IR..."));
    ui->stream_record->setToolTip({});

    QPointer<MainWindow> window(this);
    QThread *checker=QThread::create([window,rgbUrl,irUrl]() {
        QString rgbError;
        QString irError;
        const bool rgbAvailable=StreamRecorder::checkStreamAvailable(rgbUrl,4000,&rgbError);
        if (QThread::currentThread()->isInterruptionRequested()) return;
        const bool irAvailable=StreamRecorder::checkStreamAvailable(irUrl,4000,&irError);
        if(!window)
            return;
        QMetaObject::invokeMethod(window,[window,rgbUrl,irUrl,rgbAvailable,irAvailable,
                                          rgbError,irError]() {
            if(!window || window->m_shutdownStarted)
                return;
            window->ui->stream_record->setEnabled(true);
            if(!rgbAvailable || !irAvailable) {                                     // Remove this comment when the thermal camera is available
                QStringList errors;
                if(!rgbAvailable) errors<<"RGB: "+rgbError;
                if(!irAvailable) errors<<"IR: "+irError;
                const QString message=errors.join("; ");
                qWarning()<<"Stream recording preflight failed:"<<message;
                window->ui->stream_record->setChecked(false);
                window->ui->stream_record->setText(tr("start stream record"));
                window->ui->stream_record->setToolTip(message);
                return;
            }
            if(window->ui->stream_record->isChecked())
                window->startVerifiedStreamRecording(rgbUrl,irUrl);
        },Qt::QueuedConnection);
    });
    m_streamChecker=checker;
    checker->setParent(this);
    connect(checker,&QThread::finished,this,[this,checker]() {
        if (m_streamChecker==checker) m_streamChecker=nullptr;
        checker->deleteLater();
    });
    checker->start();
}

void MainWindow::startVerifiedStreamRecording(const QString &rgbUrl,const QString &irUrl)
{
    if (m_shutdownStarted || m_stoppingStream || m_collectingStreamTelemetry) return;
    // The asynchronous RTSP probes may take seconds; recheck decoded video now.
    if (!requireLiveVideo()) {
        ui->stream_record->setChecked(false);
        ui->stream_record->setText(tr("start stream record"));
        updateRecordingAvailability();
        return;
    }
    const QString outputDirectory=
        QDir(QCoreApplication::applicationDirPath()).filePath("video-telemetry");
    if (!QDir().mkpath(outputDirectory)) {
        qWarning()<<"Cannot create recording directory:"<<outputDirectory;
        ui->stream_record->setChecked(false);
        ui->stream_record->setText(tr("start stream record"));
        return;
    }
    if (m_streamPart == 0)
        m_streamSessionBase = QDir(outputDirectory).filePath(
            "stream_"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"));
    const QString sessionBasePath = m_streamSessionBase
            + QString("_part%1").arg(++m_streamPart, 3, 10, QLatin1Char('0'));
    const QString telemetryFilename=sessionBasePath+".telemetry.json";
    m_streamTelemetry.file.setFileName(telemetryFilename);
    m_streamTelemetryPackets=m_telemetryPackets;
    m_collectingStreamTelemetry=true;
    if (!stream_rgb_recorder->start(rgbUrl,sessionBasePath)
        || !stream_ir_recorder->start(irUrl,sessionBasePath)) {                               // Remove this comment when the thermal camera is available
        if(m_streamTelemetryConnection) {
            disconnect(m_streamTelemetryConnection);
            m_streamTelemetryConnection={};
        }
        stopStreamRecording();
        ui->stream_record->setChecked(false);
        ui->stream_record->setText(tr("start stream record"));
        return;
    }
    ui->stream_record->setText(tr("stop stream record"));
    m_streamPartTimer.start();
}

void MainWindow::writeStreamTelemetry()
{
    if (!m_streamTelemetry.file.isOpen()) return;
    const auto timestamps=stream_rgb_recorder->recordedTimestamps();
    // Index keys remain unique for duplicate RTP timestamps and timestamp wrap.
    // The original telemetry TS is never replaced by the video timestamp.
    quint64 missing=0;
    bool first=true;
    for (qsizetype index=0; index<timestamps.size(); ++index) {
        const quint64 timestamp=timestamps[index];
        const bool available=timestamp!=0 && m_streamTelemetryPackets.contains(timestamp);
        QJsonObject packet=available?m_streamTelemetryPackets.value(timestamp):QJsonObject();
        const QString match=available?"exact":"missing";
        if (!available) ++missing;
        QJsonObject recording;
        recording.insert("frame_index",qint64(index));
        if (timestamp!=0) recording.insert("rtp_timestamp",qint64(timestamp));
        else recording.insert("rtp_timestamp",QJsonValue(QJsonValue::Null));
        recording.insert("telemetry_match",match);
        recording.insert("telemetry_source","UDP_rgb_ts");
        if (available) recording.insert("telemetry_timestamp",qint64(timestamp));
        recording.insert("frame_count_verified",stream_rgb_recorder->frameCountVerified());
        if (!stream_rgb_recorder->frameCountVerified())
            recording.insert("alignment_warning","MP4 sample count mismatch or incomplete finalization");
        packet.insert("_recording",recording);
        const QByteArray line=(first?QByteArray():QByteArray(",\n"))
            +"  \""+QByteArray::number(index)+"\":"
            +QJsonDocument(packet).toJson(QJsonDocument::Compact);
        if (m_streamTelemetry.file.write(line)!=line.size()) {
            const QString error=tr("Telemetry write failed: ")+m_streamTelemetry.file.errorString();
            qCritical().noquote()<<error;
            ui->stream_record->setToolTip(error);
            statusBar()->showMessage(error,30000);
            break;
        }
        first=false;
    }
    m_streamTelemetry.firstEntry=first;
    qInfo()<<"RGB MP4 / telemetry entries:"<<timestamps.size()
           <<"missing:"<<missing
           <<"verified:"<<stream_rgb_recorder->frameCountVerified();
}

void MainWindow::stopStreamRecording()
{
    if (m_stoppingStream) return;
    if (!m_collectingStreamTelemetry
            && !stream_rgb_recorder->isRecording() && !stream_ir_recorder->isRecording())
        return;
    m_streamPartTimer.stop();
    m_stoppingStream=true;
    ui->stream_record->setText(tr("finalizing recording..."));
    ui->stream_record->setEnabled(false);
    stream_rgb_recorder->stop();
    stream_ir_recorder->stop();
    // Accept delayed UDP packets after the final MP4 frame list is fixed.
    m_streamFinishElapsed.start();
    m_streamFinishTimer.start();
}



void MainWindow::checkStreamTelemetryTail()
{
    if (!m_stoppingStream) return;
    bool complete=true;
    const auto timestamps=stream_rgb_recorder->recordedTimestamps();
    for (quint64 timestamp : timestamps) {
        if (timestamp!=0 && !m_streamTelemetryPackets.contains(timestamp)) {
            complete=false;
            break;
        }
    }
    if (complete || m_streamFinishElapsed.elapsed()>=1500) finishStreamRecording();
}

void MainWindow::finishStreamRecording()
{
    if (!m_stoppingStream) return;
    m_streamFinishTimer.stop();
    const QString filename=m_streamTelemetry.file.fileName();
    if (openTelemetryFile(m_streamTelemetry,filename)) {
        writeStreamTelemetry();
        closeTelemetryFile(m_streamTelemetry);
        m_streamTelemetryPackets.clear();
    } else {
        const QString error=tr("Cannot save telemetry: ")+m_streamTelemetry.file.errorString();
        m_rotateStreamPending = false;
        qCritical().noquote()<<error;
        statusBar()->showMessage(error,30000);
        ui->stream_record->setToolTip(error);
        // Retain the RAM packets on a disk error; do not report success.
    }
    m_collectingStreamTelemetry=false;
    ui->stream_record->setChecked(false);
    ui->stream_record->setText(tr("start stream record"));
    ui->stream_record->setEnabled(!m_shutdownStarted);
    m_stoppingStream=false;
    const bool continueRecording = m_rotateStreamPending && !m_shutdownStarted && !m_closeAfterStream;
    m_rotateStreamPending = false;
    if (continueRecording) {
        ui->stream_record->setChecked(true);
        QTimer::singleShot(0, this, [this]() {
            if (m_shutdownStarted || !ui->stream_record->isChecked()) return;
            startVerifiedStreamRecording("rtsp://"+main_ip+":"+QString::number(rgb_port)+"/rgb",
                                         "rtsp://"+main_ip+":"+QString::number(ir_port)+"/ir");
        });
    }
    if (m_closeAfterStream && !m_shutdownStarted)
        QTimer::singleShot(0,this,[this]() { close(); });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_collectingStreamTelemetry || m_stoppingStream) {
        m_closeAfterStream=true;
        m_rotateStreamPending=false;
        stopStreamRecording();
        event->ignore();
        return;
    }
    shutdownAndSave();
    QMainWindow::closeEvent(event);
}

void MainWindow::on_stream_record_clicked(bool checked)
{
    if(checked) {
        m_streamPart = 0;
        m_rotateStreamPending = false;
        checkAndStartStreamRecording();
    } else {
        m_rotateStreamPending = false;
        stopStreamRecording();
    }
}
