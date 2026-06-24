#include "PtzControlWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtGlobal>
#include <QtMath>

static QPointF mousePosition(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->pos();
#endif
}

PtzControlWidget::PtzControlWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_Hover, true);
    setCursor(Qt::PointingHandCursor);
}

QSize PtzControlWidget::sizeHint() const
{
    return QSize(220, 220);
}

QSize PtzControlWidget::minimumSizeHint() const
{
    return QSize(140, 140);
}

void PtzControlWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = controlRect();
    const QPointF center = rect.center();
    const qreal radius = rect.width() / 2.0;
    const qreal innerRadius = radius * 0.36;

    const QColor border("#cfd6e3");
    const QColor sectorFill("#f8fafc");
    const QColor sectorPressed("#e8f1ff");
    const QColor centerFill("#202838");
    const QColor icon("#111827");
    const QColor temp("#1b2230");

    const Direction directions[8] = {
        Up, UpRight, Right, DownRight, Down, DownLeft, Left, UpLeft
    };

    for (int i = 0; i < 8; ++i) {
        const Direction direction = directions[i];
        QPainterPath path = sectorPath(i);

        painter.setPen(QPen(border, 1.4));
        painter.setBrush(direction == m_pressedDirection && m_hasPress ? sectorPressed : sectorFill);
        painter.drawPath(path);

        const qreal angleDeg = -90.0 + i * 45.0;
        const qreal angleRad = qDegreesToRadians(angleDeg);
        const QPointF arrowCenter(
            center.x() + qCos(angleRad) * radius * 0.64,
            center.y() + qSin(angleRad) * radius * 0.64
        );

        painter.setPen(QPen(icon, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        drawArrow(painter, direction, arrowCenter, radius * 0.16);
    }

    painter.setPen(QPen(temp, 1.5));
    painter.setBrush(QColor("#232b3a"));
    painter.drawEllipse(center, innerRadius, innerRadius);

    const qreal stopSize = innerRadius * 0.34;
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(
        QRectF(center.x() - stopSize / 2.0, center.y() - stopSize / 2.0, stopSize, stopSize),
        1.5,
        1.5
    );
}

void PtzControlWidget::mousePressEvent(QMouseEvent *event)
{
    m_pressedDirection = directionAt(mousePosition(event));
    m_hasPress = true;
    emit pressed(m_pressedDirection);
    update();
}

void PtzControlWidget::mouseReleaseEvent(QMouseEvent *event)
{
    const Direction releasedDirection = directionAt(mousePosition(event));
    const Direction pressedDirection = m_pressedDirection;

    m_hasPress = false;
    update();

    emit released(pressedDirection);
    if (releasedDirection == pressedDirection) {
        emit clicked(pressedDirection);
    }
}

void PtzControlWidget::leaveEvent(QEvent *)
{
    if (!m_hasPress) {
        return;
    }

    const Direction pressedDirection = m_pressedDirection;
    m_hasPress = false;
    update();
    emit released(pressedDirection);
}

PtzControlWidget::Direction PtzControlWidget::directionAt(const QPointF &position) const
{
    const QRectF rect = controlRect();
    const QPointF center = rect.center();
    const qreal dx = position.x() - center.x();
    const qreal dy = position.y() - center.y();
    const qreal distance = qSqrt(dx * dx + dy * dy);
    const qreal radius = rect.width() / 2.0;

    if (distance <= radius * 0.36) {
        return Stop;
    }

    if (distance > radius) {
        return Stop;
    }

    qreal angle = qRadiansToDegrees(qAtan2(dy, dx));
    angle = angle + 90.0 + 22.5;
    while (angle < 0.0) {
        angle += 360.0;
    }
    while (angle >= 360.0) {
        angle -= 360.0;
    }

    switch (static_cast<int>(angle / 45.0)) {
    case 0:
        return Up;
    case 1:
        return UpRight;
    case 2:
        return Right;
    case 3:
        return DownRight;
    case 4:
        return Down;
    case 5:
        return DownLeft;
    case 6:
        return Left;
    default:
        return UpLeft;
    }
}

QRectF PtzControlWidget::controlRect() const
{
    const qreal side = qMin(width(), height()) - 8.0;
    return QRectF((width() - side) / 2.0, (height() - side) / 2.0, side, side);
}

QPainterPath PtzControlWidget::sectorPath(int index) const
{
    const QRectF rect = controlRect();
    const QPointF center = rect.center();
    const qreal radius = rect.width() / 2.0;
    const qreal innerRadius = radius * 0.39;

    const qreal start = 112.5 - index * 45.0;
    const qreal span = -42.0;
    const QRectF outerRect(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0);
    const QRectF innerRect(center.x() - innerRadius, center.y() - innerRadius, innerRadius * 2.0, innerRadius * 2.0);

    QPainterPath path;
    path.arcMoveTo(outerRect, start);
    path.arcTo(outerRect, start, span);
    path.arcTo(innerRect, start + span, -span);
    path.closeSubpath();
    return path;
}

void PtzControlWidget::drawArrow(QPainter &painter, Direction direction, const QPointF &center, qreal size) const
{
    qreal rotation = 0.0;
    switch (direction) {
    case Up:
        rotation = 0.0;
        break;
    case UpRight:
        rotation = 45.0;
        break;
    case Right:
        rotation = 90.0;
        break;
    case DownRight:
        rotation = 135.0;
        break;
    case Down:
        rotation = 180.0;
        break;
    case DownLeft:
        rotation = 225.0;
        break;
    case Left:
        rotation = 270.0;
        break;
    case UpLeft:
        rotation = 315.0;
        break;
    case Stop:
        return;
    }

    painter.save();
    painter.translate(center);
    painter.rotate(rotation);
    painter.drawLine(QPointF(0, size), QPointF(0, -size));
    painter.drawLine(QPointF(0, -size), QPointF(-size * 0.55, -size * 0.35));
    painter.drawLine(QPointF(0, -size), QPointF(size * 0.55, -size * 0.35));
    painter.restore();
}
