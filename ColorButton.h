#ifndef _COLOR_BUTTON_H
#define _COLOR_BUTTON_H

#include <QtGui/QPushButton>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QSlider>

class ColorButton : public QPushButton
{
    Q_OBJECT

public:
    ColorButton(QWidget *parent = 0);
    ~ColorButton(); 

    QColor getColor();
    void setColor(QColor color);

signals:
    void colorChanged();

protected:
    virtual void paintEvent(QPaintEvent *e);

private slots:
    void btnColor();

private:
    QColor mColor;
};

class SliderClick : public QSlider
{
public:
    SliderClick(QWidget *parent = 0) : QSlider(parent)
    {
    }
protected:
    virtual void mousePressEvent(QMouseEvent *ev)
    {
        //注意应先调用父类的鼠标点击处理事件，这样可以不影响拖动的情况
        QSlider::mousePressEvent(ev);

        if(!isSliderDown())
        {
            //获取鼠标的位置，这里并不能直接从ev中取值（因为如果是拖动的话，鼠标开始点击的位置没有意义了）
            double pos = ev->pos().x() / (double)width();
            setValue(pos * (maximum() - minimum()) + minimum());
        }
    }
};

#endif