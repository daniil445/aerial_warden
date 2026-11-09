#include "motion_controller.h"
#include "ui_motion_controller.h"
#include "PtzControlWidget.h"
#include <QKeyEvent>
#include <QHeaderView>
#include <QSignalBlocker>
#include <cmath>

motion_controller::motion_controller(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::motion_controller)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);
    auto *table = ui->positions_table;
    connect(table->itemDelegate(), &QAbstractItemDelegate::closeEditor, this,
            [table](QWidget *, QAbstractItemDelegate::EndEditHint hint) {
        if (hint != QAbstractItemDelegate::SubmitModelCache) return;
        // Wait until the view has committed the name and closed its editor.
        QTimer::singleShot(0, table, [table]() {
            QWidget *focused = QApplication::focusWidget();
            if (focused && (focused == table || table->isAncestorOf(focused)))
                focused->clearFocus();
        });
    });
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(table, &QTableWidget::itemSelectionChanged, this, [this, table]() {
        const bool selected = !table->selectedItems().isEmpty();
        ui->delete_position->setEnabled(selected);
        ui->go_position->setEnabled(selected && m_havePosition);
    });
    connect(table, &QTableWidget::itemChanged, this, [this]() { emit preferencesChanged(); });
    connect(ui->add_position, &QPushButton::clicked, this, [this, table]() {
        if (!m_havePosition) return;
        appendPosition(tr("Position %1").arg(table->rowCount() + 1), aim_x, aim_y);
        table->selectRow(table->rowCount() - 1);
        emit preferencesChanged();
    });
    connect(ui->delete_position, &QPushButton::clicked, this, [this, table]() {
        if (table->selectedItems().isEmpty()) return;
        table->removeRow(table->currentRow());
        emit preferencesChanged();
    });
    connect(ui->go_position, &QPushButton::clicked, this, [this, table]() {
        if (!m_havePosition || table->selectedItems().isEmpty()) return;
        const int row = table->currentRow();
        emit moveToCommand(table->item(row, 1)->data(Qt::UserRole).toDouble(),
                           table->item(row, 2)->data(Qt::UserRole).toDouble(), x_speed, y_speed);
    });


    connect(ui->round_widget , &PtzControlWidget::pressed, this, [this](PtzControlWidget::Direction direction) {
        switch (static_cast<int>((int)direction)) {
        case 0:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_8, Qt::KeypadModifier));
            break;
        case 1:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_8, Qt::KeypadModifier));
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_6, Qt::KeypadModifier));
            break;
        case 2:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_6, Qt::KeypadModifier));
            break;
        case 3:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_2, Qt::KeypadModifier));
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_6, Qt::KeypadModifier));
            break;
        case 4:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_2, Qt::KeypadModifier));
            break;
        case 5:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_2, Qt::KeypadModifier));
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_4, Qt::KeypadModifier));
            break;
        case 6:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_4, Qt::KeypadModifier));
            break;
        case 7:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_8, Qt::KeypadModifier));
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_4, Qt::KeypadModifier));
            break;
        case 8:
            emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_5, Qt::KeypadModifier));
            break;
        default:
            break;
        }
    });

    connect(ui->round_widget, &PtzControlWidget::released, this, [this](PtzControlWidget::Direction direction) {
        switch (static_cast<int>((int)direction)) {
        case 0:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_8, Qt::KeypadModifier));
            break;
        case 1:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_8, Qt::KeypadModifier));
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_6, Qt::KeypadModifier));
            break;
        case 2:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_6, Qt::KeypadModifier));
            break;
        case 3:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_2, Qt::KeypadModifier));
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_6, Qt::KeypadModifier));
            break;
        case 4:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_2, Qt::KeypadModifier));
            break;
        case 5:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_2, Qt::KeypadModifier));
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_4, Qt::KeypadModifier));
            break;
        case 6:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_4, Qt::KeypadModifier));
            break;
        case 7:
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_8, Qt::KeypadModifier));
            emit keyReleaseEvent(new QKeyEvent(QEvent::KeyRelease, Qt::Key_4, Qt::KeypadModifier));
            break;
        default:
            break;
        }
    });

    connect(ui->pb_home,       &QPushButton::pressed,[=]() {emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier));});
    connect(ui->speed_slider, &QSlider::valueChanged, [=](int val) {ui->sb_x_speed->setValue(val/10.0); ui->sb_y_speed->setValue(val/10.0);});
    connect(ui->sb_x_speed, &QDoubleSpinBox::valueChanged, [=](double val) {x_speed=(ui->zoom_ratio->isChecked())?val/cam_zoom:val;
                                                                            emit update_speed_x_y(x_speed,y_speed);});
    connect(ui->sb_y_speed, &QDoubleSpinBox::valueChanged, [=](double val) {y_speed=(ui->zoom_ratio->isChecked())?val/cam_zoom:val;
                                                                            emit update_speed_x_y(x_speed,y_speed);});
    connect(ui->screen_click, &QCheckBox::stateChanged, this, &motion_controller::update_screen_click);
    connect(ui->stabilization_click, &QCheckBox::stateChanged, this, &motion_controller::update_stabilization_click);
    connect(ui->zoom_ratio, &QCheckBox::toggled, this, [this]() {
        const double divisor = ui->zoom_ratio->isChecked() ? qMax(1.0, cam_zoom) : 1.0;
        x_speed = ui->sb_x_speed->value() / divisor;
        y_speed = ui->sb_y_speed->value() / divisor;
        emit update_speed_x_y(x_speed, y_speed);
    });
    for (auto *box : {ui->zoom_ratio, ui->screen_click, ui->stabilization_click})
        connect(box, &QCheckBox::toggled, this, &motion_controller::preferencesChanged);
    for (auto *spin : {ui->sb_x_speed, ui->sb_y_speed})
        connect(spin, &QDoubleSpinBox::valueChanged, this, &motion_controller::preferencesChanged);

    QTimer::singleShot(200, this, [=]()
    {
        emit ui->sb_x_speed->valueChanged(ui->sb_x_speed->value());
        emit ui->sb_y_speed->valueChanged(ui->sb_y_speed->value());
    });

}

motion_controller::~motion_controller()
{
    delete ui;
}

void motion_controller::restorePreferences(QSettings &settings)
{
    const QSignalBlocker blocker(ui->positions_table);
    ui->positions_table->setRowCount(0);
    const int count = settings.beginReadArray("pref/positions");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        bool xOk = false, yOk = false;
        const double x = settings.value("x").toDouble(&xOk);
        const double y = settings.value("y").toDouble(&yOk);
        if (xOk && yOk && std::isfinite(x) && std::isfinite(y))
            appendPosition(settings.value("name").toString(), x, y);
    }
    settings.endArray();
    for (auto *box : {ui->zoom_ratio, ui->screen_click, ui->stabilization_click})
        box->setChecked(settings.value("pref/" + box->objectName(), box->isChecked()).toBool());
    for (auto *spin : {ui->sb_x_speed, ui->sb_y_speed})
        spin->setValue(settings.value("pref/" + spin->objectName(), spin->value()).toDouble());
}

void motion_controller::savePreferences(QSettings &settings) const
{
    settings.remove("pref/positions");
    settings.beginWriteArray("pref/positions", ui->positions_table->rowCount());
    for (int row = 0; row < ui->positions_table->rowCount(); ++row) {
        settings.setArrayIndex(row);
        settings.setValue("name", ui->positions_table->item(row, 0)->text());
        settings.setValue("x", ui->positions_table->item(row, 1)->data(Qt::UserRole));
        settings.setValue("y", ui->positions_table->item(row, 2)->data(Qt::UserRole));
    }
    settings.endArray();
    for (auto *box : {ui->zoom_ratio, ui->screen_click, ui->stabilization_click})
        settings.setValue("pref/" + box->objectName(), box->isChecked());
    for (auto *spin : {ui->sb_x_speed, ui->sb_y_speed})
        settings.setValue("pref/" + spin->objectName(), spin->value());
}

bool motion_controller::screenClickEnabled() const { return ui->screen_click->isChecked(); }
bool motion_controller::stabilizationEnabled() const { return ui->stabilization_click->isChecked(); }

QPointF motion_controller::mouseSpeedLimits() const
{
    const double divisor = ui->zoom_ratio->isChecked() ? qMax(1.0, cam_zoom) : 1.0;
    return QPointF(ui->sb_x_speed->value() / divisor, ui->sb_y_speed->value() / divisor);
}

QPointF motion_controller::joystickSpeedLimits() const
{
    return QPointF(ui->sb_x_speed->value(), ui->sb_y_speed->value());
}

bool motion_controller::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent*>(event);
        auto *widget = qobject_cast<QWidget*>(obj);
        QWidget *focus = QApplication::focusWidget();
        const bool editing = focus && (focus->inherits("QLineEdit")
                || focus->inherits("QTextEdit") || focus->inherits("QPlainTextEdit")
                || focus->inherits("QAbstractSpinBox") || focus->inherits("QComboBox"));
        if (widget && widget->window() == window() && !editing
                && !(key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
                && (key->key() == Qt::Key_Plus || key->key() == Qt::Key_Minus)) {
            keyPressEvent(key);
            return true;
        }
    }
    if(obj->objectName() == "MainWindow"){
        if (event->type() == QEvent::KeyPress){
            qDebug()<<"eventFilter"<<obj->objectName();
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            keyPressEvent(keyEvent);
        }else if (event->type() == QEvent::KeyRelease){
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            keyReleaseEvent(keyEvent);
        }
    }
    return QObject::eventFilter(obj, event);
}

void motion_controller::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    bool isNumpad = event->modifiers() & Qt::KeypadModifier;

    switch (event->key())
    {
    case Qt::Key_8:
        if (isNumpad) emit moveCommand("up",y_speed, true);
        break;

    case Qt::Key_2:
        if (isNumpad) emit moveCommand("down",y_speed, true);
        else emit change_cam(1);
        break;

    case Qt::Key_4:
        if (isNumpad) emit moveCommand("left",x_speed, true);
        break;

    case Qt::Key_6:
        if (isNumpad) emit moveCommand("right",x_speed, true);
        break;
    case Qt::Key_7:
        if (isNumpad) emit moveCommand("up",y_speed, true);
        if (isNumpad) emit moveCommand("left",x_speed, true);
        break;
    case Qt::Key_9:
        if (isNumpad) emit moveCommand("up",y_speed, true);
        if (isNumpad) emit moveCommand("right",x_speed, true);
        break;
    case Qt::Key_1:
        if (isNumpad){
            emit moveCommand("left",x_speed, true);
            emit moveCommand("down",y_speed, true);
        }else{
            emit change_cam(0);
        }
        break;

    case Qt::Key_3:
        if (isNumpad) emit moveCommand("right",x_speed, true);
        if (isNumpad) emit moveCommand("down",y_speed, true);
        break;
    case Qt::Key_Plus:
        emit send_zoom(1);
        break;

    case Qt::Key_Minus:
        emit send_zoom(-1);
        break;


    case Qt::Key_H:
        emit moveToCommand(0,0,x_speed,y_speed);
        break;

    case Qt::Key_5:
        emit moveCommand("stop",0, true);
        break;
    }
}

void motion_controller::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;
    bool isNumpad = event->modifiers() & Qt::KeypadModifier;

    switch (event->key())
    {
    case Qt::Key_8:
        if (isNumpad) emit moveCommand("up",y_speed, false);
        break;

    case Qt::Key_2:
        if (isNumpad) emit moveCommand("down",y_speed, false);
        break;

    case Qt::Key_4:
        if (isNumpad) emit moveCommand("left",x_speed, false);
        break;

    case Qt::Key_6:
        if (isNumpad) emit moveCommand("right",x_speed, false);
        break;

    case Qt::Key_7:
        if (isNumpad) emit moveCommand("up",y_speed, false);
        if (isNumpad) emit moveCommand("left",x_speed, false);
        break;

    case Qt::Key_9:
        if (isNumpad) emit moveCommand("down",y_speed, false);
        if (isNumpad) emit moveCommand("right",x_speed, false);
        break;

    case Qt::Key_1:
        if (isNumpad) emit moveCommand("left",x_speed, false);
        if (isNumpad) emit moveCommand("down",y_speed, false);
        break;

    case Qt::Key_3:
        if (isNumpad) emit moveCommand("right",x_speed, false);
        if (isNumpad) emit moveCommand("down",y_speed, false);
        break;
    case Qt::Key_Plus:
        // ui->sb_x_speed->setValue(ui->sb_x_speed->value()+1);
//        emit moveCommand("rgb_zoom",0, false);
        break;

    case Qt::Key_Minus:
        // ui->sb_x_speed->setValue(ui->sb_x_speed->value()-1);
        // emit moveCommand("rgb_zoom",0, false);
        break;
    }
}

void motion_controller::update_aim(QVector2D ang,QVector3D pos,QVector3D gyro,QVector3D mag)
{
    if (!std::isfinite(ang.x()) || !std::isfinite(ang.y())) return;
    aim_x=ang.x();
    aim_y=ang.y();
    m_havePosition = true;
    ui->add_position->setEnabled(true);
    ui->go_position->setEnabled(!ui->positions_table->selectedItems().isEmpty());
}

void motion_controller::update_zoom(int zoom)
{
    connected=true;
    cam_zoom=zoom;
    this->setEnabled(connected);
    ui->l_mot_status->setText(connected ? tr("Connected") : tr("Disconnected"));
    if(!connected) return;
    x_speed=(ui->zoom_ratio->isChecked())?ui->sb_x_speed->value()/cam_zoom:ui->sb_x_speed->value();
    y_speed=(ui->zoom_ratio->isChecked())?ui->sb_y_speed->value()/cam_zoom:ui->sb_y_speed->value();
}

void motion_controller::appendPosition(const QString &name, double x, double y)
{
    const QSignalBlocker blocker(ui->positions_table);
    const int row = ui->positions_table->rowCount();
    ui->positions_table->insertRow(row);
    ui->positions_table->setItem(row, 0, new QTableWidgetItem(name));
    for (int column = 1; column <= 2; ++column) {
        const double value = column == 1 ? x : y;
        auto *item = new QTableWidgetItem(QString::number(value, 'f', 3));
        item->setData(Qt::UserRole, value);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->positions_table->setItem(row, column, item);
    }
}
