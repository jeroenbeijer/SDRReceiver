#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "jonti/fftrwrapper.h"
#include "jonti/fftwrapper.h"
#include "sdrj.h"

typedef FFTrWrapper<float> FFTr;
typedef FFTWrapper<float> FFT;

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

    const QList<int> supportedRTLSDRSampleRates={288000,1536000,1920000};

signals:

    void fftData(const std::vector<cpx_typef> &data);
    void fftVFOSlot(QString topic);

public slots:
     void fftHandlerSlot(const std::vector<cpx_typef>& data);

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

    sdrj * radio;

    QVector<double> spec_freq_vals;
    QVector<float> hann_window;

    FFTr *fftr;
    FFT* fft;

    QVector<float> in;
    QVector<cpx_typef> inr;
    QVector<cpx_typef> out;

    int N;
    int nFFT;

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

    QVector<vfo*> VFOs;
    QVector<vfo*> VFOsub[3];
    QVector<vfo*> VFOmain;

    QString remote_rtl;

    bool biasT;
    bool usb;
    bool enableFFT;



};
#endif // MAINWINDOW_H
