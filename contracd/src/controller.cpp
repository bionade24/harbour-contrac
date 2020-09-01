/*
 * Copyright 2020, David Llewellyn-Jones <david@flypig.co.uk>
 * Released under the GPLv2
 *
 * More info at: https://www.flypig.co.uk/contrac
 *
 */

#include <QDebug>

#include "controller.h"

Controller::Controller(QObject *parent) : QObject(parent)
  , m_registered(false)
{
    m_bleadvert = new BleAdvertisement(this);
    m_bleadvert->setServiceUUIDs(QStringList("0000fd6f-0000-1000-8000-00805f9b34fb"));
    QVariantMap data;
    data.insert("0000fd6f-0000-1000-8000-00805f9b34fb", QByteArray("Hello"));
    m_bleadvert->setServiceData(data);
    //m_bleadvert->setLocalName(QStringLiteral("flypig"));
    m_bleadvert->setDuration(10 * 60);
    m_bleadvert->setTimeout(10 * 60);

    m_bleadvert->registerDBus(QStringLiteral("/uk/co/flypig/advert1"));

    m_manager = new BleAdvertisementManager(this);
    m_manager->connectDBus();
}

Controller::~Controller()
{
    qDebug() << "~Controller() start";
    m_bleadvert->unRegisterDBus();
    delete m_bleadvert;
    qDebug() << "~Controller() finish";
}

void Controller::registerAdvert()
{
    if (!m_registered) {
        qDebug() << "Registering advertisement";
        m_manager->registerAdvertisement(m_bleadvert);
        m_registered = true;
        emit activeChanged();
    }
}

void Controller::unRegisterAdvert()
{
    if (m_registered) {
        qDebug() << "Unregistering advertisement";
        m_manager->unRegisterAdvertisement(m_bleadvert);
        m_registered = false;
        emit activeChanged();
    }
}

void Controller::setAdvertData(QByteArray const &rpi, QByteArray const &metadata)
{
    bool registered = m_registered;
    QByteArray serviceData;

    if (registered) {
        qDebug() << "CONTRAC: unregistering";
        unRegisterAdvert();
    }
    serviceData = rpi;
    serviceData.append(metadata);

    QVariantMap data;
    data.insert("0000fd6f-0000-1000-8000-00805f9b34fb", metadata);
    m_bleadvert->setServiceData(data);
//    QString rpiName(rpi.toHex());
//    rpiName.truncate(16);
//    m_bleadvert->setLocalName(rpiName);
    if (registered) {
        registerAdvert();
    }
}

QString Controller::binaryToHex(QByteArray binary, int lineLength)
{
    QString result;
    QString hex = binary.toHex();
    for (int i = 0; i < hex.length(); ++i) {
        result += hex[i];
        if (i != hex.length() - 1) {
            if ((i + 1) % lineLength == 0) {
                result += "\n";
            }
            else {
                if ((i + 1) % 2 == 0) {
                    result += " ";
                }
            }
        }
    }

    return result;
}

bool Controller::active() const
{
    return m_registered;
}
