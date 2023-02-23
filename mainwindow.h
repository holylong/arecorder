#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QFont>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;
class QLabel;
class AudioRecorderThread;
class QSystemTrayIcon;
class QMenu;
class QListWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    void InitView();
    void InitSigSlot();
    void InitTrayIcon();
    void UpdateTrayIcon();

private slots:
    void RecvMsgSlot(const QString &s);

private:
    Ui::MainWindow *ui;

    QLabel          *_labelTime;
    QPushButton     *_btnSwitch;
    QSystemTrayIcon *_trayIcon;
    QMenu           *_trayMenu;
    QListWidget     *_listDev;

    QVector<QString> _devs;


    AudioRecorderThread *_recThread;
    QPoint        _lastPos;
    qulonglong    _step{0};
    QFont         _font;
};
#endif // MAINWINDOW_H
