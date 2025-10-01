#ifndef FILEREADER_H
#define FILEREADER_H

#include <complex>
#include <QObject>
#include <QByteArray>
#include <QtConcurrent/QtConcurrent>

typedef std::complex<float> cpx_typef;

class filereader : public QObject
{
    Q_OBJECT

public:

    explicit filereader(QObject *parent = 0);
    ~filereader();

public slots:

    void Stop();
    void Start(QString fileName, int sampleRate, uint32_t bucketSize);

signals:

    void finished();

private:

    volatile bool running;

    void process(QString fileName, int sampleRate, uint32_t bucketSize);

    QFuture<void> future;

    QVector<float> floats;

    std::vector<float> samples;



signals:

    void audio_signal_out(const float *inputBuffer, int size);

};

#endif // FILEREADER_H
