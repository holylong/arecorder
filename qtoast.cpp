#include "qtoast.h"
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPainter>
#include <QHBoxLayout>

QToast::QToast(QWidget *parent) : QWidget(parent)
{

}

QToast::QToast(const QString& msg, QWidget* parent) : QWidget(parent)
{
    if (_labelToast == nullptr) {
      QHBoxLayout* layout = new QHBoxLayout();
       _labelToast = new QLabel();
       QFont font;
       font.setPixelSize(15);
       font.setBold(true);
       _labelToast->setFont(font);

       _widget = new QWidget();
       QHBoxLayout* tlayout = new QHBoxLayout();
       tlayout->addWidget(_labelToast, 0, Qt::AlignHCenter);
       _widget->setStyleSheet("border-radius:15px");
       _widget->setLayout(tlayout);
       layout->addWidget(_widget);
       layout->setSpacing(20);
       layout->setMargin(20);
       setLayout(layout);
    }
    _labelToast->setText(msg);
   
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void QToast::showAnimation(int timeout)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(1000);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start();
    QWidget::show();

    QTimer::singleShot(timeout, [&]
    {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
        animation->setDuration(1000);
        animation->setStartValue(1);
        animation->setEndValue(0);
        animation->start();
        connect(animation, &QPropertyAnimation::finished, [&]
        {
            close();
            deleteLater();
        });
    });
}

void QToast::paintEvent(QPaintEvent *event)
{
    QPainter paint(this);
    paint.begin(this);
    auto kBackgroundColor = QColor(255, 255, 255);
    kBackgroundColor.setAlpha(0.0 * 255);
    paint.setRenderHint(QPainter::Antialiasing, true);
    paint.setPen(Qt::NoPen);
    paint.setBrush(QBrush(kBackgroundColor, Qt::SolidPattern));
    paint.drawRect(0, 0, width(), height());
    paint.end();
}


void QToast::show(const QString& msg)
{
    QToast* toast = new QToast(msg);
    toast->setWindowFlags(toast->windowFlags() | Qt::WindowStaysOnTopHint);
    toast->adjustSize();

    toast->setFixedSize(toast->width() + 40, toast->height()+20);
    
    toast->showAnimation();
}
