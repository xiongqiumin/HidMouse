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
        //ע��Ӧ�ȵ��ø��������������¼����������Բ�Ӱ���϶������
        QSlider::mousePressEvent(ev);

        if(!isSliderDown())
        {
            //��ȡ����λ�ã����ﲢ����ֱ�Ӵ�ev��ȡֵ����Ϊ������϶��Ļ�����꿪ʼ�����λ��û�������ˣ�
            double pos = ev->pos().x() / (double)width();
            setValue(pos * (maximum() - minimum()) + minimum());
        }
    }
};

#endif