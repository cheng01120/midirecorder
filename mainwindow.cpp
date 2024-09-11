#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVector>
#include <QElapsedTimer>

#define TIME_DIVISION 500

QVector<cxxmidi::Event> events;
QElapsedTimer elapsed_timer;
unsigned now = 0;;
unsigned bpm = 120;

#define Us2dt(us) cxxmidi::converters::Us2dt(us, 60000000/bpm, TIME_DIVISION)


void mycallback( double deltatime, std::vector< unsigned char > *recv, void *userData )
{
    cxxmidi::Message msg;
    for(int i = 0; i < recv->size(); i++) msg.push_back(recv->at(i));

    size_t elapsed = elapsed_timer.nsecsElapsed()/1000;
    int64_t delta = elapsed - now;
    if(delta < 0) delta = 0;
    events.push_back(cxxmidi::Event(Us2dt(delta), msg));

    now = elapsed;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_isRecording(false)
{
    ui->setupUi(this);
    ui->recordButton->setEnabled(false);
    ui->recordButton->setIcon(QIcon(":/images/rec_green.png"));

    unsigned n = findMidiPorts();
    if(n!=0) {
        ui->recordButton->setEnabled(true);
    }

    this->statusBar()->showMessage("Ready.");
    setWindowTitle("MIDI Recorder");
}

MainWindow::~MainWindow()
{
    delete ui;
    if(m_port) delete m_port;
}

unsigned MainWindow::findMidiPorts()
{
    m_port = new RtMidiIn();
    unsigned n = m_port->getPortCount();
    if(n) {
        for(int i = 0; i < n; i++) {
            std::string portName = m_port->getPortName(i);
            ui->deviceComboBox->addItem(portName.c_str());
        }
    }
    return n;
}

void MainWindow::on_recordButton_clicked()
{
    int i = ui->deviceComboBox->currentIndex();
    if(i < 0 || !m_port) return;

    if(!m_isRecording) {
        if(events.size()) events.clear();
        // start recording.
        ui->recordButton->setIcon(QIcon(":/images/rec_red.png"));
        m_port->openPort(i);
        m_port->setCallback(&mycallback);
        elapsed_timer.start();
        now = 0;
        this->statusBar()->showMessage("Recording...");
    }
    else {
        ui->recordButton->setIcon(QIcon(":/images/rec_green.png"));
        m_port->closePort();
        QString path(ui->lineEdit->text());
        QString ts(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));
        path += "/" + ts + ".mid";

        cxxmidi::File my_file;
        cxxmidi::Track& track = my_file.AddTrack();
        for(auto ev : events) track.push_back(ev);

        unsigned dd = elapsed_timer.nsecsElapsed()/1000 - now;
        track.push_back(cxxmidi::Event(Us2dt(dd), 0xff, cxxmidi::Event::kEndOfTrack));
        my_file.SaveAs(path.toLocal8Bit().data());
        this->statusBar()->showMessage("File saved to " + path);
    }
    m_isRecording = !m_isRecording;
}
