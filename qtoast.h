#ifndef QTOAST_H
#define QTOAST_H

#include <QWidget>
#include <QLabel>

class QToast : public QWidget
{
    Q_OBJECT
public:
    explicit QToast(QWidget *parent = nullptr);
    explicit QToast(const QString& msg, QWidget *parent = nullptr);

    void showAnimation(int timeout = 2000);
    static void show(const QString& msg);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QLabel      * _labelToast{nullptr};
    QWidget     * _widget{ nullptr };

signals:

};

#endif // QTOAST_H
