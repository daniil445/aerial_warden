#pragma once

#include <QObject>
#include <QPointF>
#include <QElapsedTimer>
#include <functional>
#include <QTimer>
struct JoystickDevice {
    int id = -1;
    QString name;
    int buttons = 0;
    bool hasPov = false;
    std::array<bool, 6> axes{{false, false, false, false, false, false}};
    std::array<quint32, 6> minimum{};
    std::array<quint32, 6> maximum{};
};

struct JoystickSample {
    std::array<quint32, 6> axes{};
    quint32 buttons = 0;
    quint32 pov = 0xffff;
};

class JoystickBackend {
public:
    virtual ~JoystickBackend() = default;
    virtual QVector<JoystickDevice> devices() = 0;
    virtual bool read(int id, JoystickSample& sample) = 0;
    virtual QString error() const = 0;
};
std::unique_ptr<JoystickBackend> makeJoystickBackend();

class JoystickController : public QObject {
    Q_OBJECT
public:
    explicit JoystickController(QObject* parent = nullptr,
                                std::unique_ptr<JoystickBackend> backend = makeJoystickBackend());
    void suspend();
    void setEnabled(bool enabled);
    void setContinuousZoom(bool continuous);
    void poll();
    std::function<QPointF()> speedLimits;
    std::function<bool()> inputAllowed;

signals:
    void precisionModeChanged(bool enabled);
    void controlStarted();
    void velocityChanged(QPointF velocity);
    void controlStopped();
    void zoomStep(int direction);
    void zoomMotion(int direction);
    void focusMotion(int direction);
    void statusChanged(QString text);

private:
    void disarm();
    void status(const QString& text);
    static double normalize(quint32 value, quint32 minimum, quint32 maximum);
    static int sector(double value);
    std::unique_ptr<JoystickBackend> m_backend;
    JoystickDevice m_device;
    QTimer m_timer;
    QElapsedTimer m_scan;
    bool m_enabled = true;
    bool m_continuousZoom = false;
    bool m_precisionEnabled = false;
    int m_zoomDirection = 0;
    bool m_zoomReady = false;
    int m_focusDirection = 0;
    bool m_focusReady = false;
    QElapsedTimer m_focusHeartbeat;
    bool m_invertY = true;
    bool m_ready = false;
    bool m_haveSample = false;
    bool m_controlling = false;
    bool m_haveVelocity = false;
    quint32 m_previousButtons = 0;
    QPointF m_lastVelocity;
    QString m_status;
};
