#include "joystickcontroller.h"
#include <QCoreApplication>
#include <algorithm>
#include <cmath>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <QLibrary>
#endif

namespace {
class NativeJoystickBackend : public JoystickBackend {
public:
#ifdef Q_OS_WIN
    NativeJoystickBackend() : m_library(QStringLiteral("winmm")) {
        m_caps = reinterpret_cast<Caps>(m_library.resolve("joyGetDevCapsW"));
        m_read = reinterpret_cast<Read>(m_library.resolve("joyGetPosEx"));
        if (!m_caps || !m_read) m_error = QCoreApplication::translate("JoystickController", "Could not load the Windows Joystick API.");
    }
    QVector<JoystickDevice> devices() override {
        QVector<JoystickDevice> found;
        if (!m_caps || !m_read) return found;
        for (UINT id = 0; id < 16; ++id) {
            JOYCAPSW caps{};
            JoystickSample sample;
            if (m_caps(id, &caps, sizeof(caps)) != JOYERR_NOERROR || !read(id, sample)) continue;
            JoystickDevice device;
            device.id = int(id);
            device.name = QString::fromWCharArray(caps.szPname);
            device.buttons = int(std::min(caps.wNumButtons, UINT(32)));
            device.hasPov = (caps.wCaps & JOYCAPS_HASPOV) != 0;
            device.axes = {{true, true, bool(caps.wCaps & JOYCAPS_HASZ), bool(caps.wCaps & JOYCAPS_HASR),
                             bool(caps.wCaps & JOYCAPS_HASU), bool(caps.wCaps & JOYCAPS_HASV)}};
            device.minimum = {{caps.wXmin, caps.wYmin, caps.wZmin, caps.wRmin, caps.wUmin, caps.wVmin}};
            device.maximum = {{caps.wXmax, caps.wYmax, caps.wZmax, caps.wRmax, caps.wUmax, caps.wVmax}};
            found.append(device);
        }
        return found;
    }
    bool read(int id, JoystickSample& sample) override {
        if (!m_read) return false;
        JOYINFOEX value{};
        value.dwSize = sizeof(value);
        value.dwFlags = JOY_RETURNALL | JOY_RETURNPOVCTS;
        const MMRESULT result = m_read(UINT(id), &value);
        if (result != JOYERR_NOERROR) {
            m_error = QCoreApplication::translate("JoystickController", "Device unavailable (Windows error %1). Check USB and joy.cpl.").arg(result);
            return false;
        }
        sample.axes = {{value.dwXpos, value.dwYpos, value.dwZpos, value.dwRpos, value.dwUpos, value.dwVpos}};
        sample.buttons = value.dwButtons;
        sample.pov = value.dwPOV;
        return true;
    }
private:
    using Caps = MMRESULT (WINAPI*)(UINT_PTR, LPJOYCAPSW, UINT);
    using Read = MMRESULT (WINAPI*)(UINT, LPJOYINFOEX);
    QLibrary m_library;
    Caps m_caps = nullptr;
    Read m_read = nullptr;
#else
    QVector<JoystickDevice> devices() override { return {}; }
    bool read(int, JoystickSample&) override { return false; }
#endif
public:
    QString error() const override {
#ifdef Q_OS_WIN
        return m_error;
#else
        return QCoreApplication::translate("JoystickController", "This diagnostic panel supports Windows.");
#endif
    }
private:
    QString m_error;
};
}

std::unique_ptr<JoystickBackend> makeJoystickBackend()
{
    return std::make_unique<NativeJoystickBackend>();
}

JoystickController::JoystickController(QObject* parent, std::unique_ptr<JoystickBackend> backend)
    : QObject(parent), m_backend(std::move(backend))
{
    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &JoystickController::poll);
    disarm();
    m_device = {};
    m_scan.invalidate();
    m_timer.start(); poll();
}

void JoystickController::suspend() { disarm(); }

void JoystickController::setEnabled(bool enabled)
{
    disarm();
    m_enabled = enabled;
}

void JoystickController::setContinuousZoom(bool continuous)
{
    if (m_continuousZoom == continuous) return;
    if (m_zoomDirection) emit zoomMotion(0);
    m_zoomDirection = 0;
    m_zoomReady = false;
    m_continuousZoom = continuous;
}

void JoystickController::disarm()
{
    if (m_focusDirection) emit focusMotion(0);
    m_focusDirection = 0;
    m_focusReady = false;
    m_focusHeartbeat.invalidate();
    if (m_zoomDirection) emit zoomMotion(0);
    m_zoomDirection = 0;
    m_zoomReady = false;
    const bool wasControlling = m_controlling;
    m_controlling = false;
    m_ready = false;
    m_haveSample = false;
    m_haveVelocity = false;
    m_previousButtons = 0;
    if (wasControlling) emit controlStopped();
}

void JoystickController::status(const QString& text)
{
    if (m_status == text) return;
    m_status = text;
    emit statusChanged(text);
}

double JoystickController::normalize(quint32 value, quint32 minimum, quint32 maximum)
{
    return std::clamp(2.0 * (double(value) - minimum) / (double(maximum) - minimum) - 1.0, -1.0, 1.0);
}

int JoystickController::sector(double value)
{
    constexpr double deadZone = 0.10;
    const double magnitude = std::abs(value);
    if (magnitude <= deadZone) return 0;
    const int step = std::min(10, 1 + int((magnitude - deadZone) / (1.0 - deadZone) * 10));
    return value < 0 ? -step : step;
}

void JoystickController::poll()
{
    if (!m_enabled) return;
    if (inputAllowed && !inputAllowed()) { disarm(); return; }
    if (m_device.id < 0) {
        if (m_scan.isValid() && m_scan.elapsed() < 1000) return;
        m_scan.start();
        // ID 0 was verified by the user's diagnostic capture. Do not silently
        // switch to a different controller if several USB devices are present.
        for (const auto& device : m_backend->devices()) {
            if (device.id == 0 && device.axes[0] && device.axes[1] && device.buttons >= 8
                    && device.maximum[0] > device.minimum[0] && device.maximum[1] > device.minimum[1]) {
                m_device = device;
                break;
            }
        }
        if (m_device.id < 0) { status(QCoreApplication::translate("JoystickController", "Joystick ID 0 was not found or does not support X/Y and buttons 1–8")); return; }
    }
    JoystickSample sample;
    if (!m_backend->read(m_device.id, sample)) {
        disarm();
        m_device = {};
        status(QCoreApplication::translate("JoystickController", "Joystick disconnected. Movement stopped."));
        return;
    }
    const bool trigger = (sample.buttons & 1u) != 0;
    if (!m_haveSample) {
        m_haveSample = true;
        m_previousButtons = sample.buttons;
        m_ready = !trigger;
        status(m_ready ? QCoreApplication::translate("JoystickController", "Joystick ready: hold trigger 1 to move")
                       : QCoreApplication::translate("JoystickController", "Release trigger 1, then press it again"));
        return;
    }
    const quint32 pressed = sample.buttons & ~m_previousButtons;
    m_previousButtons = sample.buttons;
    if (!m_ready) {
        if (!trigger) { m_ready = true; status(QCoreApplication::translate("JoystickController", "Joystick ready: hold trigger 1 to move")); }
        return;
    }
    if (pressed & (1u << 1)) {
        m_precisionEnabled = !m_precisionEnabled;
        emit precisionModeChanged(m_precisionEnabled);
        status(m_precisionEnabled ? QCoreApplication::translate("JoystickController", "Precision mode enabled (20% speed)")
                                  : QCoreApplication::translate("JoystickController", "Precision mode disabled"));
    }
    const bool nearer = (sample.buttons & (1u << 8)) != 0; // Button 9
    const bool farther = (sample.buttons & (1u << 10)) != 0; // Button 11
    if (!nearer && !farther) m_focusReady = true;
    const int focusDirection = m_focusReady && nearer != farther ? (farther ? 1 : -1) : 0;
    if (focusDirection != m_focusDirection
            || (focusDirection && (!m_focusHeartbeat.isValid() || m_focusHeartbeat.elapsed() >= 250))) {
        m_focusDirection = focusDirection;
        m_focusHeartbeat.start();
        emit focusMotion(focusDirection);
    }
    const bool minus = (sample.buttons & (1u << 4)) != 0;
    const bool plus = (sample.buttons & (1u << 5)) != 0;
    if (!minus && !plus) m_zoomReady = true;
    if (m_continuousZoom) {
        const int direction = trigger && m_zoomReady && minus != plus ? (plus ? 1 : -1) : 0;
        if (direction != m_zoomDirection) {
            m_zoomDirection = direction;
            emit zoomMotion(direction);
        }
    } else if (trigger && m_zoomReady && minus != plus) {
        if (minus && (pressed & (1u << 4))) emit zoomStep(-1);
        if (plus && (pressed & (1u << 5))) emit zoomStep(1);
    }
    if (!trigger) {
        if (m_controlling) {
            m_controlling = false;
            m_haveVelocity = false;
            emit controlStopped();
            status(QCoreApplication::translate("JoystickController", "Trigger released. Movement stopped."));
        }
        return;
    }
    const QPointF maximum = speedLimits ? speedLimits() : QPointF();
    if (!std::isfinite(maximum.x()) || !std::isfinite(maximum.y()) || maximum.x() < 0 || maximum.y() < 0) {
        disarm(); status(QCoreApplication::translate("JoystickController", "Invalid Velocity values")); return;
    }
    if (!m_controlling) {
        m_controlling = true;
        emit controlStarted();
        status(QCoreApplication::translate("JoystickController", "Manual joystick control"));
    }
    const double x = normalize(sample.axes[0], m_device.minimum[0], m_device.maximum[0]);
    const double y = normalize(sample.axes[1], m_device.minimum[1], m_device.maximum[1]) * (m_invertY ? -1.0 : 1.0);
    const double precision = m_precisionEnabled ? 0.2 : 1.0;
    const QPointF velocity(maximum.x() * sector(x) / 10.0 * precision,
                           maximum.y() * sector(y) / 10.0 * precision);
    if (!m_haveVelocity || velocity != m_lastVelocity) {
        m_haveVelocity = true;
        m_lastVelocity = velocity;
        emit velocityChanged(velocity);
    }
}
