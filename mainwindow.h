#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <RtMidi.h>
#include <cxxmidi/file.hpp>

#pragma comment(lib, "rtmidi")

class ElapsedTimer {
public:
    ElapsedTimer();
    void start();
    UINT64 elapsed();

private:
    UINT64 frequency_;
    UINT64 start_;
};


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_recordButton_clicked();

private:
    Ui::MainWindow *ui;
    RtMidiIn *m_port;

    unsigned findMidiPorts();
    bool m_isRecording;
};
#endif // MAINWINDOW_H
