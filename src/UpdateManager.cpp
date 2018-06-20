#include "UpdateManager.h"

#include <QSettings>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

// Directives to obtain the definitions passed on compile time as strings.
#define STRINGIZER(arg) #arg
#define STR_VALUE(arg) STRINGIZER(arg)
#define MIDIEDITOR_RELEASE_VERSION_STRING_EVAL STR_VALUE(MIDIEDITOR_RELEASE_VERSION_STRING_DEF)
#define MIDIEDITOR_RELEASE_DATE_EVAL STR_VALUE(MIDIEDITOR_RELEASE_DATE_DEF)

UpdateManager* UpdateManager::_instance = NULL;
bool UpdateManager::_autoMode = false;

UpdateManager::UpdateManager()
    : QObject()
{
    _updateID = MIDIEDITOR_RELEASE_VERSION_ID_DEF;
#ifdef __WINDOWS_MM__
    _system = "win32";
#else
#ifdef __ARCH64__
    _system = "linux64";
#else
    _system = "linux32";
#endif
#endif

  _versionString = MIDIEDITOR_RELEASE_VERSION_STRING_EVAL;
  _date = MIDIEDITOR_RELEASE_DATE_EVAL;
    connect(&_webCtrl, SIGNAL(finished(QNetworkReply*)), this,
        SLOT(fileDownloaded(QNetworkReply*)));
}

void UpdateManager::init()
{
    _mirrors.append("https://midieditor.org/update");
}

QString UpdateManager::versionString() { return _versionString; }

QString UpdateManager::date() { return _date; }

UpdateManager* UpdateManager::instance()
{
    if (_instance == NULL)
        _instance = new UpdateManager();
    return _instance;
}

void UpdateManager::checkForUpdates()
{
    listIndex = 0;
    tryNextMirror();
}

bool UpdateManager::autoCheckForUpdates() { return _autoMode; }

void UpdateManager::setAutoCheckUpdatesEnabled(bool b) { _autoMode = b; }

void UpdateManager::tryNextMirror()
{

    if (listIndex >= _mirrors.size()) {
        return;
    }

    QString mirror = _mirrors.at(listIndex);
    listIndex++;

    QNetworkRequest request(QUrl(mirror + "/update.php?version=" + QString::number(_updateID) + "&system=" + _system));
    request.setRawHeader("User-Agent", "MidiEditor UpdateManager");
    _webCtrl.get(request);
}

void UpdateManager::fileDownloaded(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        // QUrl possibleRedirectUrl =
        // reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        // if(!possibleRedirectUrl.isEmpty()){
        //	QNetworkRequest request(possibleRedirectUrl);
        //	_webCtrl.get(request);
        //	reply->deleteLater();
        //	return;
        //}
        qWarning("Network error: %s", reply->errorString().toUtf8().constData());
        tryNextMirror();
        return;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();

    QDomDocument doc("update");
    QString error;
    if (!doc.setContent(data, &error)) {
        qWarning("Error: UpdateManager failed to parse downloaded xml");
        qWarning("%s", error.toUtf8().constData());
        tryNextMirror();
        return;
    } else {
        QDomElement element = doc.documentElement();
        if (element.tagName() != "update") {
            qWarning("Error: UpdateManager failed to parse downloaded xml "
                     "(unexpected root element)");
            tryNextMirror();
            return;
        } else {
            int thisVersion = element.attribute("your_version").toInt();
            if (thisVersion != _updateID) {
                qWarning("Error: UpdateManager failed to parse downloaded xml "
                         "(unexpected own version ID)");
                tryNextMirror();
                return;
            }
            int newUpdate = element.attribute("latest_version").toInt();

            qWarning("Latest version code: %d", newUpdate);

            if (newUpdate > _updateID && _updateID >= 0) {
                QString path = element.attribute("download_path");
                QString changelog = element.firstChildElement("changelog").text();
                QString newVersionString = element.attribute("latest_version_string");
                Update* update = new Update();
                update->setVersionID(newUpdate);
                update->setChangelog(changelog);
                update->setDownloadPath(path);
                update->setVersionString(newVersionString);

                emit updateDetected(update);
            }
        }
    }
}
