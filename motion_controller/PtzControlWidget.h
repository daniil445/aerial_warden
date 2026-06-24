#pragma once

#include <QWidget>

class PtzControlWidget : public QWidget
{
    Q_OBJECT

public:
    enum Direction {
        Up,
        UpRight,
        Right,
        DownRight,
        Down,
        DownLeft,
        Left,
        UpLeft,
        Stop
    };
    Q_ENUM(Direction)

    explicit PtzControlWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void pressed(PtzControlWidget::Direction direction);
    void released(PtzControlWidget::Direction direction);
    void clicked(PtzControlWidget::Direction direction);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Direction directionAt(const QPointF &position) const;
    QRectF controlRect() const;
    QPainterPath sectorPath(int index) const;
    void drawArrow(QPainter &painter, Direction direction, const QPointF &center, qreal size) const;

    Direction m_pressedDirection = Stop;
    bool m_hasPress = false;
};
