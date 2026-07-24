#include "motion_controller.h"
#include "ui_motion_controller.h"
#include "PtzControlWidget.h"
#include <QKeyEvent>

motion_controller::motion_controller(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::motion_controller)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);


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
    connect(ui->pb_aim,        &QPushButton::pressed,[=]() {emit keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Q, Qt::NoModifier));});
    connect(ui->speed_slider, &QSlider::valueChanged, [=](int val) {ui->sb_x_speed->setValue(val/10.0); ui->sb_y_speed->setValue(val/10.0);});
    connect(ui->sb_x_speed, &QDoubleSpinBox::valueChanged, [=](double val) {x_speed=(ui->zoom_ratio->isChecked())?val/cam_zoom:val;
                                                                            emit update_speed_x_y(x_speed,y_speed);});
    connect(ui->sb_y_speed, &QDoubleSpinBox::valueChanged, [=](double val) {y_speed=(ui->zoom_ratio->isChecked())?val/cam_zoom:val;
                                                                            emit update_speed_x_y(x_speed,y_speed);});
    connect(ui->screen_click, &QCheckBox::stateChanged, this, &motion_controller::update_screen_click);
    connect(ui->stabilization_click, &QCheckBox::stateChanged, this, &motion_controller::update_stabilization_click);

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

bool motion_controller::eventFilter(QObject *obj, QEvent *event)
{

    if (event->type() == QEvent::KeyPress){
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        keyPressEvent(keyEvent);
    }else if (event->type() == QEvent::KeyRelease){
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        keyReleaseEvent(keyEvent);
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
        if (isNumpad) emit send_zoom(cam_zoom+1);
        break;

    case Qt::Key_Minus:
        if (isNumpad) emit send_zoom(cam_zoom-1);
        break;

    case Qt::Key_Q:
        if(!target_chosed){
            target_chosed=true;
            target_x=aim_x;
            target_y=aim_y;
            ui->pb_aim->setIcon(QIcon(":/img/image/arrows/aim_ready.png"));
        }else
            emit moveToCommand(target_x,target_y,x_speed,y_speed);
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

void motion_controller::update_aim(QVector2D ang,QVector3D pos)
{
    aim_x=ang.x();
    aim_y=ang.y();
}

void motion_controller::update_zoom(int zoom)
{
    connected=true;
    cam_zoom=zoom;
    this->setEnabled(connected);
    ui->l_mot_status->setText(connected?"Connected":"Disconnected");
    if(!connected) return;
    x_speed=(ui->zoom_ratio->isChecked())?ui->sb_x_speed->value()/cam_zoom:ui->sb_x_speed->value();
    y_speed=(ui->zoom_ratio->isChecked())?ui->sb_y_speed->value()/cam_zoom:ui->sb_y_speed->value();
}

void motion_controller::on_pb_move_x_clicked()
{
    emit moveToCommand(ui->ds_move_x->value(),0,x_speed,0);
}


void motion_controller::on_pb_move_y_clicked()
{
    emit moveToCommand(0,ui->ds_move_y->value(),0,y_speed);
}


void motion_controller::on_pb_all_axis_move_clicked()
{
    emit moveToCommand(ui->ds_move_x->value(),ui->ds_move_y->value(),x_speed,y_speed);
}

