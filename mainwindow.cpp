#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcustomplot.h"
#include <qsettings.h>
#include <qmessagebox.h>
#include <iostream>
#include <QFileInfo>
#include <QSharedPointer>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    qRegisterMetaType<QVector<quint8>>("const QVector<cpx_type>&");
    qRegisterMetaType<QVector<quint8>>("const std::vector<cpx_type>&");
    qRegisterMetaType<QVector<quint8>>("const std::vector<cpx_typef>&");
    qRegisterMetaType<QSharedPointer<std::vector<cpx_typef>>>("QSharedPointer<std::vector<cpx_typef>>");
    qRegisterMetaType<QSharedPointer<std::vector<cpx_typef>>>("QSharedPointer<std::vector<short>>");
    qRegisterMetaType<uint32_t>("const uint32_t");


    biasT = false;
    tuner_gain = 496;
    bool dc = false;
    usb = false;
    enableFFT=true;

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



    if(settings.value("tuner_type").toString() == "sdrplay")
    {

        #ifndef HAVE_SDRPLAY
            QMessageBox msgBox;
            msgBox.setText("Non SDRPlay enabled build. SDRPlay not supported");
            msgBox.exec();
            exit(1);
        #endif


        if(!supportedSDRPlayampleRates.contains(Fs))
        {
            QMessageBox msgBox;
            QString tmpstr;for(int i=0;i<supportedSDRPlayampleRates.size();i++)tmpstr+=QString::number(supportedSDRPlayampleRates[i])+", ";
            tmpstr.chop(2);
            msgBox.setText("sample_rate setting in ini file is "+QString::number(Fs)+" and not supported.\nOnly rates "+tmpstr+" are supported");
            msgBox.exec();
            exit(1);
        }

        #ifdef HAVE_SDRPLAY
            pRadio = new sdrplay(this);
        #endif

    }else
    {
        if(!supportedRTLSDRSampleRates.contains(Fs))
        {
            QMessageBox msgBox;
            QString tmpstr;for(int i=0;i<supportedRTLSDRSampleRates.size();i++)tmpstr+=QString::number(supportedRTLSDRSampleRates[i])+", ";
            tmpstr.chop(2);
            msgBox.setText("sample_rate setting in ini file is "+QString::number(Fs)+" and not supported.\nOnly rates "+tmpstr+" are supported");
            msgBox.exec();
            exit(1);
        }

        pRadio = new rtlsdr(this);
    }

    QStringList vfo_str;

    center_frequency = settings.value("center_frequency").toInt();

    int auto_start = settings.value("auto_start").toInt();
    QString auto_start_tuner_serial = settings.value("auto_start_tuner_serial").toString();
    int auto_start_tuner_idx = settings.value("auto_start_tuner_idx").toInt();
    int auto_start_biast = settings.value("auto_start_biast").toInt();
    int disableFFT = settings.value("disable_fft").toInt();
    int gain = settings.value("tuner_gain").toInt();
    int remote_gain_idx =  settings.value("remote_rtl_gain_idx").toInt();

    remote_rtl=settings.value("remote_rtl").toString();

    int mix_offset = settings.value("mix_offset").toInt();

    bool vfo_threads = settings.value("vfo_threads").toString() == "0" ? false : true;

    pRadio->setVFOThreads(vfo_threads);

    bool sb = settings.value("small_buffers").toString() == "1" ? true : false;

    // usually 4/16 buffers per Fs but in some cases 5/20 due to multiple of 512
    bufsplit = sb ? 32 : 4;

    if(double((int((2*Fs)/bufsplit))%512) > 0)
    {

        bufsplit = sb ? 20 : 5;
        buflen = int((2*Fs)/bufsplit);

    }
    else
    {

        buflen = int((2*Fs)/bufsplit);
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

    // bind publisher setup
    pZmqPub = new ZmqPublisher();

    pZmqPub->setBind(true);
    pZmqPub->setAddress(zmq_address);


    QThread * pworkerThreadZmq = new QThread();
    pZmqPub->moveToThread(pworkerThreadZmq);


    dc = settings.value("correct_dc_bias").toString() == "1" ? true : false;

    int msize = settings.beginReadArray("main_vfos");

    // Read main vfos
    for (int i = 0; i < msize; ++i) {

        settings.setArrayIndex(i);

        int taps =settings.value("halfband_taps").toInt();

        if(taps == 0||(taps!=11&&taps!=23&&taps!=51))
        {
            taps = 11;
        }
        vfo * pVFO = new vfo();
        int vfo_freq = settings.value("frequency").toInt();
        if(!(vfo_freq>0))
        {

            QMessageBox msgBox;
            msgBox.setText("Invalid frequency for main VFO "+QString::number(i+1));
            msgBox.exec();
            exit(1);

        }

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
        pVFO->setHalfbandTaps(taps);
        pVFO->setCenterFreq(vfo_freq);
        pVFO->setMixerFreq(center_frequency - vfo_freq);
        pVFO->setDemodUSB(false);
        pVFO->init(buflen/2, false, vfo_threads);
        pVFO->setVFOs(&VFOsub[i]);


        // move each man vfo to a worker thread if using threads
        if(vfo_threads){

            QThread * pworkerThread = new QThread();
            pVFO->moveToThread(pworkerThread);

            workers.push_back(pworkerThread);

            connect(pRadio, SIGNAL(demodVFOData(const QSharedPointer<std::vector<cpx_typef>>,int )),pVFO,SLOT(demodVFOData(const QSharedPointer<std::vector<cpx_typef>>,int)),Qt::QueuedConnection);

            pworkerThread->start();
        }

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

        if(vfo_freq<=mix_offset)
        {

            QMessageBox msgBox;
            msgBox.setText("Invalid frequency for VFO "+QString::number(i+1));
            msgBox.exec();
            exit(1);

        }

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
            case 8400:
                out_rate = 48000;
                break;
            case 10500:
                out_rate = 48000;
                break;
            case 21000:
                out_rate = 96000;
                break;
            default:
                out_rate = 48000;
                break;

            }
        }

        int taps =settings.value("halfband_taps").toInt();

        if(taps == 0||(taps!=11&&taps!=23&&taps!=51))
        {
            taps = 11;
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
        pVFO->setZmqTopicLSB(settings.value("topic_lsb").toString());
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
        pVFO->setHalfbandTaps(taps);
        pVFO->setGain(pRadio->adjustGain(settings.value("gain").toInt()));

        pVFO->setMixerFreq((center_frequency-main_vfo_freq) - vfo_freq);
        pVFO->setCenterFreq(vfo_freq);
        pVFO->setFs(main_vfo_out_rate);
        pVFO->setCompressonStyle(1);
        pVFO->init(main_vfo_out_rate/bufsplit,true,vfo_threads, lateDecimate);

        VFOsub[main_idx].push_back(pVFO);

        if(vfo_threads){

            QThread * pworkerThread = new QThread();
            pVFO->moveToThread(pworkerThread);

            connect(pVFO, SIGNAL(fftData(const QSharedPointer<std::vector<cpx_typef>>)), this, SLOT(fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>>)),Qt::QueuedConnection);
            connect(this, SIGNAL(fftVFOSlot(QString)), pVFO, SLOT(fftVFOSlot(QString)));
            connect(VFOmain.at(main_idx), SIGNAL(demodSubVFOData(const QSharedPointer<std::vector<cpx_typef>>,int )),pVFO,SLOT(demodVFOData(const QSharedPointer<std::vector<cpx_typef>>,int)),Qt::QueuedConnection);
            connect(pVFO, SIGNAL(transmitData(const QSharedPointer<std::vector<short>>, uint32_t,QString,uint32_t)), pZmqPub, SLOT(transmitData(const QSharedPointer<std::vector<short>>, uint32_t,QString,uint32_t)),Qt::QueuedConnection);

            workers.push_back(pworkerThread);
            pworkerThread->start();

        }else{

            connect(pVFO, SIGNAL(fftData(const QSharedPointer<std::vector<cpx_typef>>)), this, SLOT(fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>>)),Qt::QueuedConnection);
            connect(this, SIGNAL(fftVFOSlot(QString)), pVFO, SLOT(fftVFOSlot(QString)));
            connect(pVFO, SIGNAL(transmitData(const QSharedPointer<std::vector<short>>, uint32_t,QString,uint32_t)), pZmqPub, SLOT(transmitData(const QSharedPointer<std::vector<short>>, uint32_t,QString,uint32_t)),Qt::QueuedConnection);


        }

        vfo_str.push_back(settings.value("topic").toString());


    }

    settings.endArray();

    pRadio->setVFOs(&VFOmain);
    pRadio->setDCCorrection(dc);

    pZmqPub->connect();
    pworkerThreadZmq->start();

    Fs2 = Fs;
    nFFT = 8192;
    fftr = new FFTr(nFFT,false);
    fft = new FFT(nFFT, false);

    pAvgMain = new MovingAverage(5);
    pAvgVfo = new MovingAverage(25);

    out.resize(nFFT);
    in.resize(nFFT);
    inr.resize(nFFT);
    pwr.resize(nFFT);
    smooth_pwr.resize(nFFT);

    pFileReader = new filereader();

    connect(pRadio, SIGNAL(fftData(const QSharedPointer<std::vector<cpx_typef>>)), this, SLOT(fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>>)),Qt::UniqueConnection);
    connect(pRadio, SIGNAL(audio_signal_out(const float *, int )),pRadio,SLOT(demodData(const float*,int)));
    connect(pFileReader, SIGNAL(audio_signal_out(const float *, int )),pRadio,SLOT(demodData(const float*,int)));
    connect(this, SIGNAL(fftVFOSlot(QString)), pRadio, SLOT(fftVFOSlot(QString)));


    ui->setupUi(this);
    ui->biasTee->setText("Enable BiasTee");
    ui->processFile->setVisible(false);
    ui->spinBox->setRange(center_frequency-100000,center_frequency+100000 );
    ui->spinBox->setValue(center_frequency);
    ui->spinBox->setSingleStep(200);

    MainWindow::makePlot();

    QStringList devices = pRadio->deviceNames();
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

        if(auto_start_tuner_serial.length() > 0)
        {
           int index = pRadio->indexBySerial(auto_start_tuner_serial.toStdString().c_str());

           switch(index){

               case -1 :{
                   QMessageBox msgBoxDev1;
                   msgBoxDev1.setText("Auto start device serial number from ini file is null");
                   msgBoxDev1.exec();
                   break;
               }
               case -2 : {
                   QMessageBox msgBoxDev2;
                   msgBoxDev2.setText("No devices found for auto start by device serial");
                   msgBoxDev2.exec();
                   break;
               }
               case -3 : {
                   QMessageBox msgBoxDev3;
                   msgBoxDev3.setText("No matching devices found for auto start by device serial: " +auto_start_tuner_serial ) ;
                   msgBoxDev3.exec();
                   break;
               }

               default:
                   auto_start_tuner_idx = index;
                   break;
               }

        }

        ui->comboSDRs->setCurrentIndex(auto_start_tuner_idx);

        bool result = on_startSDR_clicked();

        if(auto_start_biast == 1 && result){

            on_biasTee_clicked();
        }
        if(disableFFT == 1) {
            on_radioFFT_clicked(false);
            ui->radioFFT->setChecked(false);
        } else {
            ui->radioFFT->setChecked(true);
        }

    }else{

        ui->startSDR->setEnabled(true);
        ui->stopSDR->setEnabled(false);

        if(disableFFT == 1) {
            ui->radioFFT->setChecked(false);
        } else {
            ui->radioFFT->setChecked(true);
        }
    }

}

MainWindow::~MainWindow()
{
    if(pRadio)
    {
        pRadio->StopAndClose();

    }
    delete ui;
    delete fft;
    delete fftr;
    delete pRadio;
    delete pFileReader;

    delete pAvgMain;
    delete pAvgVfo;

    for(int a = 0; a < workers.length(); a++)
    {

        QThread* poThread = workers.at(a);
        poThread->quit();
        poThread->wait();
        delete poThread;
    }
}

void MainWindow::makePlot()
{

    ui->spectrum->addGraph();

    ui->spectrum->xAxis->setLabel("X");
    ui->spectrum->yAxis->setLabel("Y");

    spec_freq_vals.resize(nFFT);

    double  hzperbin=(double)Fs2/nFFT;
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
    ui->spectrum->yAxis->setRange(0,1.0);
    ui->spectrum->yAxis->setLabel("Power");

}

void MainWindow::fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>> data)
{
    double maxval = 0;
    double aveval = 0;

    float fgain = pRadio->getFFTGain();

    int lenth = (int)data->size();

    for(int a = 0; a<nFFT; a++)
    {

        if(a < lenth)
        {
            inr[a] = data->at(a) * hann_window[a]*fgain;
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

        if(!usb)
        {
            pwr[b]= pwr[b]*0.95+0.05*10*log10(fmax(100000.0*abs((1.0/nFFT)*val),1));
        }
        else
        {
            pwr[b]= pwr[b]*0.8+0.2*10*log10(fmax(100000.0*abs((1.0/nFFT)*val),1));

        }

        if(pwr[b]>maxval)
        {
            maxval=pwr[b];
        }
        aveval+=pwr[b];
    }



    for(int i=0;i<smooth_pwr.size();i++)
    {
            if(!usb)
            {
               smooth_pwr[i] = pAvgMain->Update(pwr[i]);

            }else
            {
              smooth_pwr[i] = pAvgVfo->Update(pwr[i]);
            }

    }

    aveval/=pwr.size();
    if((maxval-aveval)<10)
    {
        maxval=aveval+10.0;
    }

    ui->spectrum->graph(0)->setData(spec_freq_vals,smooth_pwr);

    ui->spectrum->yAxis->setRange(aveval-10 < 0 ? 0 : aveval-10, ui->spectrum->yAxis->range().upper*0.5+0.5*(maxval+20));

    ui->spectrum->replot();
}



void MainWindow::on_stopSDR_clicked()
{

   pRadio->StopAndClose();

   ui->startSDR->setEnabled(true);
   ui->stopSDR->setEnabled(false);


}

bool MainWindow::on_startSDR_clicked()
{

    bool result = false;
    bool remote = ui->comboSDRs->currentText().contains(":") ? true : false;

    if(remote)
    {

        result = pRadio->StartRemote(ui->comboSDRs->currentText(),Fs, center_frequency, tuner_gain_idx );

    }

    else{

        result = pRadio->Open(ui->comboSDRs->currentIndex());

    }
    if(!result)
    {

        QMessageBox msgBox;
        msgBox.setText("Could not open the selected device, check if it is use");
        msgBox.exec();
    }
    else if(!remote)
    {
        pRadio->Start(Fs,center_frequency, buflen, tuner_gain);
    }

    if(result)
    {

        ui->startSDR->setEnabled(false);
        ui->stopSDR->setEnabled(true);
    }

    return result;
}


void MainWindow::on_processFile_clicked()
{

    QString fileName = QFileDialog::getOpenFileName(this);

    pFileReader->Start(fileName, Fs, (uint32_t)(2*Fs)/bufsplit);
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

    if(arg1.compare("Main") == 0)
    {
        double  hzperbin=(double)Fs2/nFFT;

        for(int i=0;i<spec_freq_vals.size();i++)
        {
            spec_freq_vals[i]=((int)i)*hzperbin;
        }

        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        double start = (center_frequency-(Fs/2))/1000;
        int step = (Fs/5)/1000;

        for(int a = 1; a < 6; a++)
        {
            start+=step;
            textTicker->addTick(1000*a*step, QString::number(start/1000));
        }
        ui->spectrum->xAxis->setTicker(textTicker);

        ui->spectrum->xAxis->setTickLabels(true);
        ui->spectrum->xAxis->setRange(0,Fs);

        ui->spectrum->xAxis->setLabel("Frequency");

        usb = false;

    }else
    {
        // find the right VFO
        vfo * pVFO = getVFO(arg1);

        if(pVFO != 0)
        {

            usb = true;

            int bw = pVFO->getOutRate();

            double  hzperbin=(double)bw/nFFT;

            for(int i=0;i<spec_freq_vals.size();i++)
            {
                spec_freq_vals[i]=((int)i)*hzperbin;
            }

            ui->spectrum->xAxis->setRange(0,bw);
            double center = (pVFO->getCenterFreq());


            QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);

            double start = (center-((bw)/2))/1000;
            int step = (bw/6)/1000;

            for(int a = 1; a <= 5; a++)
            {
                start+=step;
                textTicker->addTick(a*1000*step, QString::number(start/1000, 'f', 4));
            }

            ui->spectrum->xAxis->setTicker(textTicker);

            ui->spectrum->xAxis->setTickLabels(true);

            ui->spectrum->xAxis->setLabel("VFO Bandwidth- " +  QString::number(bw) + " Hz - Full IQ spectrum is shown");

        }
    }
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    if(pRadio != 0 && arg1 != center_frequency)
    {

      int result =  pRadio->setCenterFreq(arg1);

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
            pRadio->biasTee(0, ui->comboSDRs->currentIndex());
            biasT = false;
        }else
        {
            ui->biasTee->setText("Disable BiasTee");
            pRadio->biasTee(1, ui->comboSDRs->currentIndex());
            biasT = true;

        }
    }else{
        QMessageBox msgBox;
        msgBox.setText("Remote device selected, bias tee not supported");
        msgBox.exec();
    }

}

void MainWindow::on_radioFFT_clicked(bool checked)
{
    if(checked){
        connect(pRadio, SIGNAL(fftData(const QSharedPointer<std::vector<cpx_typef>>)), this, SLOT(fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>>)),Qt::UniqueConnection);

    }else{
        disconnect(pRadio, SIGNAL(fftData(const QSharedPointer<std::vector<cpx_typef>>)), this, SLOT(fftHandlerSlot(const QSharedPointer<std::vector<cpx_typef>>)));

    }

}

vfo* MainWindow::getVFO(QString name)
{
    int size = sizeof(VFOsub);
    for(int a = 0; a < size; a++)
    {

        for(int b = 0; b < VFOsub[a].length(); b++)
        {

           vfo * pVFO = VFOsub[a].at(b);

           if(pVFO->getZmqTopic().compare(name) == 0)
           {
               return pVFO;
           }
        }
    }

    return 0;

}


