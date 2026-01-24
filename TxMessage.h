#ifndef TXMESSAGE_H
#define TXMESSAGE_H

#endif // TXMESSAGE_H
// TxMessage.h
#pragma once
#include <QSharedPointer>
#include <QString>
#include <vector>

struct TxMessage
{
    QSharedPointer<std::vector<short>> samples;
    uint32_t sampleRate;
    QString topic;
};
