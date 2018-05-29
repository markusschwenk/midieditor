#include "UpdateManager.h"

#include <QSettings>

#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

UpdateManager *UpdateManager::_instance = NULL;
bool UpdateManager::_autoMode = false;

UpdateManager::UpdateManager() : QObject(){
	_versionString = "Failed to determine Version";
	_updateID = -1;
#ifdef __WINDOWS_MM__
	_system="win32";
#else
	#ifdef __ARCH64__
		_system="linux64";
	#else
		_system="linux32";
	#endif
#endif
	connect(&_webCtrl, SIGNAL (finished(QNetworkReply*)), this, SLOT (fileDownloaded(QNetworkReply*)));
}

void UpdateManager::init(){
	_mirrors.append("https://greric.de/midieditor");
	_mirrors.append("http://midieditor.sourceforge.net/update");

	// read own configuration
	QDomDocument doc("version_info");
	QFile file("version_info.xml");
	if(file.open(QIODevice::ReadOnly)){
		QString error;
		if(doc.setContent(&file, &error)){
			QDomElement element = doc.documentElement();
			if(element.tagName() != "version_info"){
				_inited = false;
				qWarning("Error: UpdateManager failed to parse version_info.xml (unexpected root element)");
			} else {
				QDomElement version = element.firstChildElement("version");
				_versionString = version.attribute("string");
				_updateID = version.attribute("id").toInt();
				_date = version.attribute("date_published");
				_inited = true;
			}
		} else {
			_inited = false;
			qWarning("Error: UpdateManager failed to parse version_info.xml");
			qWarning("%s", error.toUtf8().constData());
		}
		file.close();
	} else {
		qWarning("Error: UpdateManager failed to open version_info.xml");
	}
}

QString UpdateManager::versionString(){
	return _versionString;
}

QString UpdateManager::date(){
	return _date;
}

UpdateManager *UpdateManager::instance() {
	if(_instance == NULL)
		_instance = new UpdateManager();
	return _instance;
}

void UpdateManager::checkForUpdates(){

	if(!_inited){
		return;
	}

	listIndex = 0;
	tryNextMirror();
}

bool UpdateManager::autoCheckForUpdates(){
	return _autoMode;
}

void UpdateManager::setAutoCheckUpdatesEnabled(bool b){
	_autoMode = b;
}

void UpdateManager::tryNextMirror(){

	if(listIndex >= _mirrors.size()){
		return;
	}

	QString mirror = _mirrors.at(listIndex);
	listIndex++;

	QNetworkRequest request(QUrl(mirror+"/update.php?version="+QString::number(_updateID)+"&system="+_system));
	request.setRawHeader("User-Agent", "MidiEditor UpdateManager");
	_webCtrl.get(request);
}

void UpdateManager::fileDownloaded(QNetworkReply *reply){
	if(reply->error()!=QNetworkReply::NoError){
		//QUrl possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		//if(!possibleRedirectUrl.isEmpty()){
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
	if(!doc.setContent(data, &error)){
		qWarning("Error: UpdateManager failed to parse downloaded xml");
		qWarning("%s", error.toUtf8().constData());
		tryNextMirror();
		return;
	} else {
		QDomElement element = doc.documentElement();
		if(element.tagName() != "update"){
			_inited = false;
			qWarning("Error: UpdateManager failed to parse downloaded xml (unexpected root element)");
			tryNextMirror();
			return;
		} else {
			int thisVersion = element.attribute("your_version").toInt();
			if(thisVersion != _updateID){
				qWarning("Error: UpdateManager failed to parse downloaded xml (unexpected own version ID)");
				tryNextMirror();
				return;
			}
			int newUpdate = element.attribute("latest_version").toInt();

			if(newUpdate > _updateID){
				QString path = element.attribute("download_path");
				QString changelog = element.firstChildElement("changelog").text();
				QString newVersionString = element.attribute("latest_version_string");
				Update *update = new Update();
				update->setVersionID(newUpdate);
				update->setChangelog(changelog);
				update->setDownloadPath(path);
				update->setVersionString(newVersionString);

				emit updateDetected(update);
			}
		}
	}
}
