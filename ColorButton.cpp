#include "ColorButton.h"
#include <QPainter>
#include <QColorDialog>

ColorButton::ColorButton(QWidget *parent)
    :QPushButton(parent)
{
    connect(this,SIGNAL(clicked()),this, SLOT(btnColor()));
}

ColorButton::~ColorButton()
{

}

QColor ColorButton::getColor()
{
    return mColor;
}

void ColorButton::setColor(QColor color)
{
    mColor = color;
    update();
}

void ColorButton::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);
    QPen pen(Qt::NoPen);

    QPoint triangle[3] = {QPoint(27,8),QPoint(36,8),QPoint(31,13)};

    paint.setPen(pen);

    QBrush bk_brush(QColor(51,51,51));
    QBrush fg_brush(mColor);
    QBrush white_brush(Qt::white);

    QRect bk = rect();
    QRect fg = QRect(2,2,19,19);
    QRect back = rect();
    
    paint.setBrush(bk_brush);
    paint.drawRect(bk);

    paint.setBrush(fg_brush);
    paint.drawRect(fg);

    paint.setBrush(white_brush);
    paint.drawPolygon(triangle,3);
}

void ColorButton::btnColor()
{    
    QColor color = QColorDialog::getColor(mColor,0,tr("Ñ¡ÔñÑÕÉ«"));   
    setColor(color);

    emit colorChanged();
}