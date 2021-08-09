#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qsettings.h>
#include <iostream>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    biasT = false;
    tuner_gain = 496;
    bool dc = false;
    usb = false;

    QFileInfo fileinfo(settings_filename);
    if(!fileinfo.exists()||!fileinfo.isFile())
    {
        QMessageBox msgBox;
        msgBox.setText("Given settings ini file doesn't exist");
        msgBox.exec();
        exit(1);
    }

    QSettings settings(settings_filename, QSettings::IniFormat);

    Fs = settings.value("sample_rate").toInt();

    if(Fs == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("sample_rate ini file key not found or equal to zero, check ini file name and key values");
        msgBox.exec();
        exit(1);
    }

    if(!supportedRTLSDRSampleRates.contains(Fs))
    {
        QMessageBox msgBox;
        QString tmpstr;for(int i=0;i<supportedRTLSDRSampleRates.size();i++)tmpstr+=QString::number(supportedRTLSDRSampleRates[i])+", ";
        tmpstr.chop(2);
        msgBox.setText("sample_rate setting in ini file is "+QString::number(Fs)+" and not supported.\nOnly rates "+tmpstr+" are supported");
        msgBox.exec();
        exit(1);
    }

    QStringList vfo_str;

    center_frequency = settings.value("center_frequency").toInt();

    int auto_start = settings.value("auto_start").toInt();
    int auto_start_tuner_idx = settings.value("auto_start_tuner_idx").toInt();



    int gain = settings.value("tuner_gain").toInt();
    int remote_gain_idx =  settings.value("remote_rtl_gain_idx").toInt();

    remote_rtl=settings.value("remote_rtl").toString();

    int mix_offset = settings.value("mix_offset").toInt();

    // usually 4 buffers per Fs but in some cases 5 due to multiple of 512
    int bufsplit = 4;

    if(double((int((2*Fs)/4))%512) > 0)
    {

        buflen = int((2*Fs)/5);
        bufsplit = 5;
    }
    else
    {

        buflen = int((2*Fs)/4);
    }


    if(gain > 0)
    {

        tuner_gain=gain;
    }
    if(remote_gain_idx > 0)
    {

        tuner_gain_idx=remote_gain_idx;
    }

    QString zmq_address = settings.value("zmq_address").toString();

    dc = settings.value("correct_dc_bias").toString() == "1" ? true : false;

    int msize = settings.beginReadArray("main_vfos");

    // Read main vfos
    for (int i = 0; i < msize; ++i) {

        settings.setArrayIndex(i);

        vfo * pVFO = new vfo();
        int vfo_freq = settings.value("frequency").toInt();
        int vfo_out_rate = settings.value("out_rate").toInt();

        QString output_connect = settings.value("zmq_address").toString();
        QString out_topic =  settings.value("zmq_topic").toString();

        int compscale = settings.value("compress_scale").toInt();

        if(compscale > 0)
        {

            pVFO->setScaleComp(compscale);
        }

        if(output_connect != "" && out_topic != "")
        {

            pVFO->setZmqAddress(output_connect);
            pVFO->setZmqTopic(out_topic);

        }


        pVFO->setFs(Fs);
        pVFO->setDecimationCount(Fs/vfo_out_rate == 1 ? 0 : int(log2(Fs/vfo_out_rate)));
        pVFO->setMixerFreq(center_frequency - vfo_freq);
        pVFO->setDemodUSB(false);
        pVFO->setCompressonStyle(1);
        pVFO->init(buflen/2, false);
        pVFO->setVFOs(&VFOsub[i]);
        VFOmain.push_back(pVFO);

    }

    settings.endArray();
    int size = settings.beginReadArray("vfos");
    nVFO = size;
    vfo_str.push_back("Main");

    // read regular vfos
    for (int i = 0; i < size; ++i) {

        settings.setArrayIndex(i);

        vfo * pVFO = new vfo();
        int vfo_freq = settings.value("frequency").toInt() + mix_offset;
        int data_rate = settings.value("data_rate").toInt();
        int out_rate = settings.value("out_rate").toInt();

        if(out_rate == 0 && data_rate > 0)
        {

            switch(data_rate){

            case 600:
                out_rate = 12000;
                break;
            case 1200:
                out_rate = 24000;
                break;
            default:
                out_rate = 48000;
                break;

            }
        }

        int filterbw=settings.value("filter_bandwidth").toInt();
        int main_vfo_freq = 0;
        int main_vfo_out_rate = Fs;
        int main_idx = 0;

        // find main VFO
        for(int a = 0; a<VFOmain.length(); a++)
        {

            int diff = std::abs((center_frequency-VFOmain.at(a)->getMixerFreq())-vfo_freq);
            if(diff < VFOmain.at(a)->getOutRate() && !VFOmain.at(a)->getDemodUSB() )
            {
                main_idx=a;
                main_vfo_freq=VFOmain.at(a)->getMixerFreq();
                main_vfo_out_rate=VFOmain.at(a)->getOutRate();
                break;
            }

        }

        pVFO->setZmqTopic(settings.value("topic").toString());
        pVFO->setZmqAddress(zmq_address);

        int lateDecimate = 0;
        if((main_vfo_out_rate/48000)==5)
        {

            pVFO->setDecimationCount(int(log2(main_vfo_out_rate/(5*out_rate))));
            lateDecimate=5;

        }
        else if((main_vfo_out_rate/48000)==6)
        {

            pVFO->setDecimationCount(int(log2(main_vfo_out_rate/(6*out_rate))));
            lateDecimate=6;

        }

        else
        {
            pVFO->setDecimationCount(int(log2(Fs/out_rate))-int(log2(Fs/main_vfo_out_rate)));

        }

        pVFO->setFilterBandwidth(filterbw);
        pVFO->setGain((float)settings.value("gain").toInt()/100);
        pVFO->setMixerFreq((center_frequency-main_vfo_freq) - vfo_freq);
        pVFO->setFs(main_vfo_out_rate);
        pVFO->setCompressonStyle(1);
        pVFO->init(main_vfo_out_rate/bufsplit,true, lateDecimate);

            VFOsub[main_idx].push_back(pVFO);

        connect(pVFO, SIGNAL(fftData(const std::vector<cpx_typef>&)), this, SLOT(fftHandlerSlot(const std::vector<cpx_typef>&)));
        connect(this, SIGNAL(fftVFOSlot(QString)), pVFO, SLOT(fftVFOSlot(QString)));

        vfo_str.push_back(settings.value("topic").toString());


    }

    settings.endArray();
    radio = new sdrj(this);

    radio->setVFOs(&VFOmain);
    radio->setDCCorrection(dc);

    Fs2 = Fs;

    nFFT = 8192;

    fftr = new FFTr(nFFT,false);
    fft = new FFT(nFFT, false);

    out.resize(nFFT);
    in.resize(nFFT);
    inr.resize(nFFT);
    pwr.resize(nFFT);
    smooth_pwr.resize(nFFT-10);


    qRegisterMetaType< QVector<quint8> >("const QVector<cpx_type>&");
    qRegisterMetaType< QVector<quint8> >("const std::vector<cpx_type>&");
    qRegisterMetaType< QVector<quint8> >("const std::vector<cpx_typef>&");

    connect(radio, SIGNAL(fftData(const std::vector<cpx_typef>&)), this, SLOT(fftHandlerSlot(const std::vector<cpx_typef>&)));
    connect(radio, SIGNAL(audio_signal_out(const float *, int )),radio,SLOT(demodData(const float*,int)));
    connect(this, SIGNAL(fftVFOSlot(QString)), radio, SLOT(fftVFOSlot(QString)));

    ui->setupUi(this);
    ui->biasTee->setText("Enable BiasTee");
    ui->processFile->setVisible(false);

    ui->spinBox->setRange(center_frequency-100000,center_frequency+100000 );
    ui->spinBox->setValue(center_frequency);
    ui->spinBox->setSingleStep(200);

    MainWindow::makePlot();

    QStringList devices = radio->deviceNames();
    ui->comboSDRs->addItems(devices);

    if(remote_rtl != "")
    {
        ui->comboSDRs->addItem(remote_rtl);

    }

    ui->comboVFO->addItems(vfo_str);

    hann_window.resize(nFFT);
    for(int i=0;i<hann_window.size();i++)
    {
        hann_window[i]=0.5*(1.0-cos(2*M_PI*((float)i)/(nFFT-1.0)));
    }

    if(auto_start == 1)
    {
        ui->comboSDRs->setCurrentIndex(auto_start_tuner_idx);

        on_startSDR_clicked();
    }else{

        ui->startSDR->setEnabled(true);
        ui->stopSDR->setEnabled(false);
    }

}

MainWindow::~MainWindow()
{
    if(radio)
    {
        radio->StopAndCloseRtl();

    }
    delete ui;

    delete fft;
    delete fftr;
    delete radio;
}

void MainWindow::makePlot()
{


    ui->spectrum->addGraph();


    ui->spectrum->xAxis->setLabel("X");
    ui->spectrum->yAxis->setLabel("Y");

    spec_freq_vals.resize(nFFT-10);

    double  hzperbin=Fs2/nFFT;
    for(int i=0;i<spec_freq_vals.size();i++)
    {
        spec_freq_vals[i]=((int)i)*hzperbin;
    }


    ui->spectrum->xAxis->setRange(0,Fs);

    ui->spectrum->graph()->setLineStyle(QCPGraph::lsLine);
    ui->spectrum->graph()->setPen(QPen(QPen(Qt::blue)));
    ui->spectrum->graph()->setBrush(QBrush(QColor(0, 0, 255, 20)));

    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    double start = (center_frequency-(Fs/2))/1000;
    int step = (Fs/5)/1000;

    for(int a = 1; a < 6; a++)
    {
        start+=step;
        textTicker->addTick(1000*a*step, QString::number(start/1000));
    }
    ui->spectrum->xAxis->setTicker(textTicker);

    ui->spectrum->xAxis->setNumberFormat("gb");
    ui->spectrum->xAxis->setLabel("Frequency");
    ui->spectrum->yAxis->setRange(0,100);
    ui->spectrum->yAxis->setLabel("Power");

}

void MainWindow::fftHandlerSlot(const std::vector<cpx_typef> &data)
{
    double maxval = 0;
    double aveval = 0;

    int lenth = (int)data.size();

    for(int a = 0; a<nFFT; a++)
    {

        if(a < lenth)
        {
            inr[a] = data[a] * hann_window[a];
        }
    }

    fft->transform(inr, out);

    for(int i=0;i<pwr.size();i++)
    {

        int b = i + pwr.size()/2;

        if(b >= pwr.size()){
            b = b - pwr.size();
        }

        double val = 0;
        val =  sqrt(out[i].imag()*out[i].imag() + out[i].real()*out[i].real());

        pwr[b]= pwr[b]*0.95+0.05*10*log10(fmax(100000.0*abs((1.0/nFFT)*val),1));

        if(pwr[b]>maxval)
        {
            maxval=pwr[b];
        }
        aveval+=pwr[b];
    }

    for(int i=0;i<smooth_pwr.size();i++)
    {

        smooth_pwr[i] = (pwr[i+4] + pwr[i+3] + pwr[i+2] + pwr[i+1] + pwr[i])/5;
    }


    aveval/=pwr.size();
    if((maxval-aveval)<10)
    {
        maxval=aveval+10.0;
    }


    if(!usb)
    {

        ui->spectrum->graph(0)->setData(spec_freq_vals,smooth_pwr);


    }else
    {

        ui->spectrum->graph(0)->setData(spec_freq_vals.mid(0,(nFFT-10)/2),smooth_pwr.mid((nFFT-10)/2, (nFFT-10)/2));
    }
    ui->spectrum->yAxis->setRange(aveval-2, ui->spectrum->yAxis->range().upper*0.5+0.5*(maxval+1));

    ui->spectrum->replot();
}



void MainWindow::on_stopSDR_clicked()
{

    radio->StopAndCloseRtl();

    ui->startSDR->setEnabled(true);
    ui->stopSDR->setEnabled(false);


}

void MainWindow::on_startSDR_clicked()
{

    bool result = false;
    bool remote = ui->comboSDRs->currentText().contains(":") ? true : false;

    if(remote)
    {

        result = radio->start_tcp_rtl(ui->comboSDRs->currentText(),Fs, center_frequency, tuner_gain_idx );

    }

    else{

        result = radio->OpenRtl(ui->comboSDRs->currentIndex());

    }
    if(!result)
    {

        QMessageBox msgBox;
        msgBox.setText("Could not open the selected device, check if it is use");
        msgBox.exec();
    }
    else if(!remote)
    {
        radio->StartRtl(Fs,center_frequency, buflen, tuner_gain);
    }

    if(result)
    {

        ui->startSDR->setEnabled(false);
        ui->stopSDR->setEnabled(true);
    }
}


void MainWindow::on_processFile_clicked()
{
    //radio->process_file(Fs);
}

void MainWindow::on_comboVFO_currentIndexChanged(const QString &arg1)
{
    emit fftVFOSlot(arg1);
    for(int i=0;i<pwr.size();i++)
    {
        pwr[i] = 0;
    }

    for(int a = 0; a<nFFT; a++)
    {
        inr[a] = 0;
    }

    if(arg1.compare("Main")){

        usb = true;
        ui->spectrum->xAxis->setTickLabels(false);
        ui->spectrum->xAxis->setRange(0,Fs/2);

    }else
    {
        usb = false;
        ui->spectrum->xAxis->setTickLabels(true);
        ui->spectrum->xAxis->setRange(0,Fs);
    }


}



void MainWindow::on_spinBox_valueChanged(int arg1)
{
    if(radio != 0 && arg1 != center_frequency)
    {

        int result =  radio->setCenterFreq(arg1);

        if(result == 0)
        {
            center_frequency = arg1;
        }
    }

}



void MainWindow::on_biasTee_clicked()
{

    bool remote = ui->comboSDRs->currentText().contains(":") ? true : false;

    if(!remote)
    {

        if(biasT)
        {

            ui->biasTee->setText("Enable BiasTee");
            radio->biasTee(0, ui->comboSDRs->currentIndex());
            biasT = false;
        }else
        {
            ui->biasTee->setText("Disable BiasTee");
            radio->biasTee(1, ui->comboSDRs->currentIndex());
            biasT = true;

        }
    }else{
        QMessageBox msgBox;
        msgBox.setText("Remote device selected, bias tee not supported");
        msgBox.exec();
    }

}
