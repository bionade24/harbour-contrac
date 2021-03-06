#ifndef PROVIDEDIAGNOSTICKEYS_H
#define PROVIDEDIAGNOSTICKEYS_H

#include <QObject>
#include <QRunnable>
#include <QVector>
#include <QMutex>
#include <QDateTime>

#include "exposureinformation.h"
#include "contactmatch.h"

#include "exposureconfiguration.h"

class ExposureNotificationPrivate;

class ProvideDiagnosticKeys : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ProvideDiagnosticKeys(ExposureNotificationPrivate * exposureNotificationPrivate, QVector<QString> const &keyFiles, ExposureConfiguration const &configuration, QString &token, qint8 rssiCorrection);

    void run() override;

    bool shouldTerminate();
    QList<ExposureInformation> aggregateExposureData(quint32 dayNumber, ExposureConfiguration const &configuration, QList<ContactMatch> matches, qint32 const days_ago) const;
    QDateTime const startTime() const;
    QList<ExposureInformation> const &exposureInfoList() const;

public slots:
    void requestTerminate();

signals:
    void actionExposureStateUpdated(QString const token);
    void taskFinished(QString const token);

private:
    ExposureNotificationPrivate * m_d;
    QVector<QString> const m_keyFiles;
    ExposureConfiguration const m_configuration;
    QString const m_token;
    qint64 m_currentDayNumber;
    qint8 m_rssiCorrection;
    QMutex m_terminateMutex;
    bool m_terminate;
    QDateTime m_startTime;
    QList<ExposureInformation> m_exposureInfoList;
};

#endif // PROVIDEDIAGNOSTICKEYS_H
