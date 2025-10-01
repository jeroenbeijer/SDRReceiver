#include "filereader.h"
#include <utility>
#include "QDebug"

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000) // usleep takes microseconds
#endif


void filereader::Start(QString fileName, int sampleRate, uint32_t bucketSize)
{

    Stop();


    future = QtConcurrent::run([=]() {
        process(fileName, sampleRate, bucketSize);
        return;
    });

}

void filereader::Stop()
{
    running = false;
    if(!future.isFinished())future.waitForFinished();
    emit finished();
}

void filereader::process(QString fileName, int sampleRate, uint32_t bucketSize)
{

    QReadWriteLock lock;
    running = true;
    QFile * pInFile = new QFile(fileName);
    if (!pInFile->open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open" << fileName << ":" << pInFile->errorString();
        return; // or handle error
    }

    QDataStream in(pInFile);
    int len = pInFile->bytesAvailable();
    int loop = len/bucketSize;
    unsigned char data[bucketSize];
    samples.resize(bucketSize);

    in.device()->reset();

    while(running)
    {

          if(!running)break;

          for(int z = 0; z<loop; z++)
          {

              qint64 lineLength = in.readRawData((char*)&data, bucketSize);

              if (lineLength <= 0) {
                  // End of file or error
                  if (in.status() != QDataStream::Ok)
                      qWarning() << "Stream error:" << in.status();
                  break;
              }
              for(uint32_t i=0;i<bucketSize;++i)
              {

                  samples[i] = floats.at(data[i]);
              }

              emit audio_signal_out(&samples[0], bucketSize);
              int sleep = 1000/((sampleRate*2)/bucketSize);

              SLEEP_MS(sleep);
          }
     }
     pInFile->close();
     delete pInFile;

     running = false;

}
filereader::filereader(QObject *parent):
    QObject(parent),
    running(false)
{

    floats.resize(256);
    const float scale = 1.0;// /127.0;

    for (int i = 0; i < 256; i++)
    {
          floats[i] = (i - 127)*scale;
    }

}

filereader::~filereader()
{
    Stop();
}




