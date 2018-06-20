#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QStringList>

#include <QByteArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

class Update {

    public:
        Update() {}
        void setVersionID(int newUpdate) { _versionID = newUpdate; }
        void setChangelog(QString changelog) {_changelog = changelog; }
        void setDownloadPath(QString path) {_path = path; }
        void setVersionString(QString newVersionString) { _versionString = newVersionString; }

        int versionID() {return _versionID; }
        QString path() {return _path; }
        QString changelog() { return _changelog; }
        QString versionString() { return _versionString; }

    private:
        int _versionID;
        QString _path;
        QString _changelog;
        QString _versionString;
};

class UpdateManager : public QObject {

	Q_OBJECT

	public:
		UpdateManager();
		void init();
		static UpdateManager *instance();
		static bool autoCheckForUpdates();
		static void setAutoCheckUpdatesEnabled(bool b);
		QString versionString();
		QString date();

	public slots:
		void checkForUpdates();
		void fileDownloaded(QNetworkReply*);

	signals:
		void updateDetected(Update *update);

	private:
		static UpdateManager *_instance;
		QStringList _mirrors;
        static bool _autoMode;
		QString _versionString, _date, _system;
        int _updateID;

		int listIndex;
		void tryNextMirror();

		QNetworkAccessManager _webCtrl;
};

#endif
