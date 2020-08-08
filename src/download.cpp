#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "settings.h"
#include "s3access.h"

#include "download.h"

Download::Download(QObject *parent) : QObject(parent)
  , m_s3Access(new S3Access(this))
  , m_fileQueue()
  , m_latest()
  , m_downloading(false)
  , m_filesReceived(0)
  , m_filesTotal(0)
  , m_status(StatusIdle)
{
    m_s3Access->setId("accessKey1");
    m_s3Access->setSecret("verySecretKey1");
    m_s3Access->setBaseUrl(Settings::getInstance().downloadServer());
    m_s3Access->setBucket("cwa");
}

Q_INVOKABLE void Download::downloadLatest()
{
    qint64 daysTotal;
    qDebug() << "Requesting keys";

    if (!m_downloading) {
        m_s3Access->setBaseUrl(Settings::getInstance().downloadServer());
        m_filesReceived = 0;
        if (m_latest.isValid()) {
            daysTotal = m_latest.daysTo(QDate::currentDate());
        }
        else{
            daysTotal = 14;
        }
        if (daysTotal > 14) {
            daysTotal = 14;
        }
        m_filesTotal = daysTotal * 24;
        emit progressChanged();
        setStatus(Status::StatusDownloading);
        startNextDateDownload();
    }
}

QDate Download::nextDownloadDay() const
{
    QDate today = QDate::currentDate();
    QDate date = today.addDays(-14);
    QDate next = m_latest.addDays(1);
    if (next > date) {
        date = next;
    }
    if (date >= today) {
        date = QDate();
    }

    return date;
}

void Download::addToFileQueue(QDate const &date, QStringList const &download)
{
    int size = m_fileQueue.size();
    if (download.size() > 0) {
        m_fileQueue[date] += download;
    }
    if (size == 0) {
        startNextFileDownload();
    }
}

QDate Download::oldestDateInQueue()
{
    qDebug() << "Queue size before: " << m_fileQueue.size();
    QDate oldest;
    for (QDate const &key : m_fileQueue.keys()) {
        if (m_fileQueue[key].size() == 0) {
            m_fileQueue.remove(key);
        }
        else {
            if (!oldest.isValid() || key < oldest) {
                oldest = key;
            }
        }
    }

    qDebug() << "Queue size after: " << m_fileQueue.size();
    qDebug() << "Oldest date: " << oldest;

    return oldest;
}

void Download::startNextFileDownload() {
    qDebug() << "Starting file download";
    QDate date = oldestDateInQueue();
    if (date.isValid()) {
        QString key = m_fileQueue[date].first();

        // FIXME: Use the actual date rather than the URL to figure out the filename
        int dateEnd = key.lastIndexOf("/", key.lastIndexOf("/") - 1) - 1;
        int dateStart = key.lastIndexOf("/", dateEnd) + 1;
        QString file = key.mid(dateStart, dateEnd - dateStart + 1) + "/" + key.mid(key.lastIndexOf("/") + 1);

        QString filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/download/" + file;
        qDebug() << "Saving to:" << filename;
        S3GetFileResult *result = m_s3Access->getFile(key, filename);
        connect(result, &S3Result::finished, this, [this, result, date, filename]() {
            qDebug() << "Finished downloading:" << m_fileQueue[date].first();

            ++m_filesReceived;
            emit downloadComplete(filename);
            emit progressChanged();

            m_fileQueue[date].removeFirst();
            if (m_fileQueue[date].size() == 0) {
                // We've downloaded all the files for this day
                qDebug() << "All files downloaded for:" << date;
                m_fileQueue.remove(date);
                m_latest = date;
                emit latestChanged();
            }
            startNextFileDownload();
            result->deleteLater();
        });
    }
    else {
        qDebug() << "File queue empty";
        if (m_downloading) {
            startNextDateDownload();
        }
    }
}

void Download::startNextDateDownload()
{
    QDate date = nextDownloadDay();
    bool downloading = m_downloading;

    if (date.isValid()) {
        m_downloading = true;
        startDateDownload(date);
    }
    else {
        qDebug() << "All dates downloaded";
        finalise();
        setStatus(Status::StatusIdle);
    }

    if (m_downloading != downloading) {
        emit downloadingChanged();
    }
}

void Download::startDateDownload(QDate const &date)
{
    qDebug() << "Starting date download:" << date;

    QString url = "version/v1/diagnosis-keys/country/DE/date/" + date.toString("yyyy-MM-dd") + "/hour/";
    S3ListResult *result = m_s3Access->list(url);
    connect(result, &S3ListResult::finished, this, [this, result, date]() {
        if (result->error() == QNetworkReply::NoError) {
            QStringList keys = result->keys();
            if (keys.size() > 0){
                qDebug() << "Key list completed: " << keys.size();
                createDateFolder(date);
                for (QString key : keys) {
                    qDebug() << "Key: " << key;
                }
                addToFileQueue(date, keys);
            }
            else {
                m_latest = date;
                emit latestChanged();
                startNextDateDownload();
            }
        }
        else {
            qDebug() << "Error";
            finalise();
            setStatus(Status::StatusError);
        }
        result->deleteLater();
    });
}

QDate Download::latest() const
{
    return m_latest;
}

void Download::createDateFolder(QDate const &date) const
{
    QDir dir;
    QString folder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/download/" + date.toString("yyyy-MM-dd");
    qDebug() << "Creating directory:" << folder;
    dir.mkpath(folder);
}

bool Download::downloading() const
{
    return m_downloading;
}

float Download::progress() const
{
    float progress = 0.0f;
    if (m_filesTotal > 0) {
        progress = static_cast<float>(m_filesReceived) / static_cast<float>(m_filesTotal);
    }

    return progress;
}

void Download::finalise()
{
    if (m_downloading) {
        m_downloading = false;
        m_filesTotal = 0;
        m_filesReceived = 0;

        emit downloadingChanged();
    }
    emit progressChanged();
}

Download::Status Download::status() const
{
    return m_status;
}

void Download::setStatus(Status status)
{
    if (m_status != status) {
        qDebug() << "Setting status: " << status;
        m_status = status;
        emit statusChanged();
    }
}
