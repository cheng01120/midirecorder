#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVector>
#include <Windows.h>
#include <QDebug>

#define TIME_DIVISION 500

QVector<cxxmidi::Event> events;
ElapsedTimer elapsed_timer;
unsigned now = 0;;
unsigned bpm = 120;

#define Us2dt(us) cxxmidi::converters::Us2dt(us, 60000000/bpm, TIME_DIVISION)


void mycallback( double deltatime, std::vector< unsigned char > *recv, void *userData )
{
    Q_UNUSED(userData);
    Q_UNUSED(deltatime);

    cxxmidi::Message msg;
    for(int i = 0; i < recv->size(); i++) msg.push_back(recv->at(i));

    unsigned elapsed = elapsed_timer.elapsed();
    int delta = elapsed - now;
    if(delta < 0) delta = 0;
    events.push_back(cxxmidi::Event(Us2dt(delta), msg));

    now = elapsed;
}


ElapsedTimer::ElapsedTimer() : frequency_(0), start_(0)
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li)) qWarning("QueryPerformanceFrequency failed!");
    frequency_ = li.QuadPart;
    start_ = 0;
}

void ElapsedTimer::start()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    start_ = li.QuadPart;
}

UINT64 ElapsedTimer::elapsed()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    UINT64 elapsed = 1000000*(li.QuadPart - start_)/frequency_;
    return elapsed;
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
    	  this->statusBar()->showMessage("Ready.");
        ui->recordButton->setEnabled(true);
    }
		else {
    	  this->statusBar()->showMessage("No MIDI device found.");
        ui->recordButton->setEnabled(false);
		}

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
        bpm = ui->spinBox->value();
        if(events.size()) events.clear();
        // start recording.
        ui->recordButton->setIcon(QIcon(":/images/rec_red.png"));
        m_port->openPort(i);
        m_port->setCallback(&mycallback);
        elapsed_timer.start();
        now = 0;
        QString s;
        QTextStream(&s) << "Recording at BPM " << bpm << "...";
        this->statusBar()->showMessage(s);
    }
    else {
        ui->recordButton->setIcon(QIcon(":/images/rec_green.png"));
        m_port->closePort();
        QString path(ui->lineEdit->text());
        QString ts(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));
        path += "/" + ts + ".mid";

        cxxmidi::File my_file;
        cxxmidi::Track& track = my_file.AddTrack();

        // set bpm
        unsigned tempo = 60000000/bpm;
        unsigned char a = tempo >> 16, b = tempo >> 8, c = tempo;
        cxxmidi::Message message(0xff, 0x51);
        message.push_back(a);
        message.push_back(b);
        message.push_back(c);
        track.push_back(cxxmidi::Event(0, message));

        for(auto ev : events) track.push_back(ev);

        unsigned dd = elapsed_timer.elapsed() - now;
        track.push_back(cxxmidi::Event(Us2dt(dd), 0xff, cxxmidi::Event::kEndOfTrack));
        my_file.SaveAs(path.toLocal8Bit().data());
        this->statusBar()->showMessage("File saved to " + path);
    }
    m_isRecording = !m_isRecording;
}
