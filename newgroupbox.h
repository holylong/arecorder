#ifndef NEWCOMBOBOX_H
#define NEWCOMBOBOX_H

#include <QGroupBox>

class NewGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    explicit NewGroupBox(QWidget *parent = nullptr);

    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:


private:
    QPoint _lastPos;
};

#endif // NEWCOMBOBOX_H
