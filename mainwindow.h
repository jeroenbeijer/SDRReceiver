#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedPointer>

#include "jonti/fftrwrapper.h"
#include "jonti/fftwrapper.h"
#include "rtlsdr.h"

#ifdef HAVE_SDRPLAY
#include "sdrplay.h"
#endif

#include "filereader.h"
#include "zmqpublisher.h"

typedef FFTrWrapper<float> FFTr;
typedef FFTWrapper<float> FFT;

typedef std::complex<double> cpx_type;
typedef std::complex<float> cpx_typef;

const int MAXVFO=50;



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static QString settings_filename;

    const QList<int> supportedRTLSDRSampleRates={288000,1536000,1920000,3072000,3840000,7680000};
    const QList<int> supportedSDRPlayampleRates={3072000,3840000,6144000,7680000};

signals:

   void fftData(const QSharedPointer<std::vector<cpx_typef>> data);
   void fftVFOSlot(QString topic);

public slots:
     void fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>> data);

private slots:
    void makePlot();
    void on_stopSDR_clicked();
    bool on_startSDR_clicked();
    void on_processFile_clicked();
    void on_comboVFO_currentIndexChanged(const QString &arg1);
    void on_spinBox_valueChanged(int arg1);
    void on_biasTee_clicked();
    void on_radioFFT_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    radio * pRadio;

    QVector<double> spec_freq_vals;
    QVector<float> hann_window;

    FFTr *fftr;
    FFT* fft;

    QVector<float> in;
    QVector<cpx_typef> inr;
    QVector<cpx_typef> out;

    int N;
    int nFFT;

    MovingAverage * pAvgMain;
    MovingAverage * pAvgVfo;

    int Fs;
    int Fs2;
    int buflen;

    QVector<double> pwr;
    QVector<double> smooth_pwr;

    int counter;
    int nVFO;
    int center_frequency;
    int tuner_gain;
    int tuner_gain_idx;


    // this is arbitrary for now
    QVector<vfo*> VFOsub[10];
    QVector<vfo*> VFOmain;
    QVector<QThread*> workers;

    QString remote_rtl;

    bool biasT;
    bool usb;
    bool enableFFT;
    int bufsplit;
    filereader * pFileReader;

    ZmqPublisher * pZmqPub;

    vfo* getVFO(QString name);

};
#endif // MAINWINDOW_H
