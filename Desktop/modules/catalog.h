//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

#ifndef CATALOG_H
#define CATALOG_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>

namespace Modules
{

// Repository type and its children are modelled after
// https://github.com/jasp-stats-modules/modules-app/blob/main/src/types.ts
// Any time
// https://jasp-stats-modules.github.io/modules-app/index.json
// changes, this code may need to be updated
class Asset : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString downloadUrl MEMBER downloadUrl)
    Q_PROPERTY(int downloadCount MEMBER downloadCount)
    Q_PROPERTY(QString architecture MEMBER architecture)
public:
    QString downloadUrl;
    int downloadCount = 0;
    QString architecture;
};

class Release : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version MEMBER version)
    Q_PROPERTY(QString publishedAt MEMBER publishedAt)
    Q_PROPERTY(QString jaspVersionRange MEMBER jaspVersionRange)
    Q_PROPERTY(QList<QObject*> assets MEMBER assets)
public:
    QString version;
    QString publishedAt;
    QString jaspVersionRange;
    QList<QObject*> assets; // QList<Asset*>
};

class CatalogModule : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString shortDescriptionHTML MEMBER shortDescriptionHTML)
    Q_PROPERTY(QString homepageUrl MEMBER homepageUrl)
    Q_PROPERTY(QList<QObject*> releases MEMBER releases)
    Q_PROPERTY(QList<QObject*> preReleases MEMBER preReleases)
    Q_PROPERTY(QString organization MEMBER organization)
    Q_PROPERTY(QString releaseSource MEMBER releaseSource)
    Q_PROPERTY(QList<QString> channels MEMBER channels)
public:
    QString name;
    QString shortDescriptionHTML;
    QString homepageUrl;
    QList<QObject*> releases; // QList<Release*>
    QList<QObject*> preReleases; // QList<Release*>
    QString organization;
    QString releaseSource;
    QList<QString> channels;
};

class Catalog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastUpdatedAt READ lastUpdatedAt NOTIFY lastUpdatedAtChanged)
    Q_PROPERTY(QList<CatalogModule*> catalogModules READ catalogModules NOTIFY catalogModulesChanged)
public:
    explicit Catalog(QObject *parent);
    Q_INVOKABLE void fetchCatalog();
    void loadCatalog();
    QString lastUpdatedAt() const;
    QList<CatalogModule*> catalogModules() const;
    
   
signals:
    void lastUpdatedAtChanged();
    void catalogModulesChanged();
    
private:
    QNetworkAccessManager *networkManager = nullptr;
    QString m_lastUpdatedAt;
    QList<CatalogModule*> m_catalogModules;
    void json2catalogModules(const QJsonArray &catalogArray);
    void updateLastUpdatedAt();
    private slots:
    void onCatalogDownloaded(QNetworkReply *reply);
    static std::map<std::string, std::string> getInstalledModuleVersions();
};

#endif // CATALOG_H