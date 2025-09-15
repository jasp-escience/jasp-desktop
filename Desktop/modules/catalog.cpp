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

#include "Catalog.h"
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>

namespace Modules
{
    Catalog *Catalog::_singleton = nullptr;
    Catalog::Catalog(QObject *parent) : QObject(parent)
    {
        if (_singleton)
            throw std::runtime_error("Can only instantiate Catalog once!");
        _singleton = this;
        networkManager = new QNetworkAccessManager(this);
        connect(networkManager, &QNetworkAccessManager::finished, this, &Catalog::onCatalogDownloaded);

        // if catalog.json is older than 1 day, then fetch it
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QString filePath = cacheDir + "/catalog.json";
        QFileInfo fileInfo(filePath);
        bool fetch = false;
        if (!fileInfo.exists()) {
            fetch = true;
        } else {
            QDateTime lastModified = fileInfo.lastModified();
            if (lastModified < QDateTime::currentDateTimeUtc().addDays(-1))
                fetch = true;
        }
        if (fetch)
            fetchCatalog();
        loadCatalog();
    }

    void Catalog::fetchCatalog()
    {
        QUrl url("https://jasp-stats-modules.github.io/modules-app/index.json");
        QNetworkRequest request(url);
        networkManager->get(request);
    }

    void Catalog::onCatalogDownloaded(QNetworkReply *reply)
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            qWarning() << "Failed to download catalog:" << reply->errorString();
            reply->deleteLater();
            return;
        }
        QByteArray data = reply->readAll();

        // Catalog to cache dir as catalog.json
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir().mkpath(cacheDir); // Ensure cache dir exists
        QString filePath = cacheDir + "/catalog.json";
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(data);
            file.close();
            qDebug() << "Catalog saved to" << filePath;
            updateLastUpdatedAt();
        }
        else
        {
            qWarning() << "Failed to write catalog to cache:" << file.errorString();
        }
        reply->deleteLater();
        loadCatalog();
    }

    void Catalog::json2catalogModules(const QJsonArray &catalogArray)
    {
        auto platform = DynamicRuntimeInfo::getRuntimeEnvironment();
        auto arch = DynamicRuntimeInfo::getMicroArch();
        std::string myArchitecture;
        if (platform == RuntimeEnvironment::MAC)
            myArchitecture = arch == MicroArch::AARCH64 ? "MacOS_arm64" : "MacOS_x86_64";
        else if (platform == RuntimeEnvironment::FLATPAK)
            myArchitecture = arch == MicroArch::AARCH64 ? "Linux_aarch64" : "Linux_x86_64";
        else if (platform == RuntimeEnvironment::LINUX_LOCAL)
            myArchitecture = arch == MicroArch::AARCH64 ? "Linux_aarch64" : "Linux_x86_64";
        else
            myArchitecture = "Windows_x86-64";

        // TODO remove releases that are invalid for jaspVersionRange and current desktop version

        // TODO remove catalogModule which already has latest version installed
        auto installedModulesVersions = getInstalledModuleVersions();

        auto jaspVersion = AppInfo::version();
        QList<CatalogModule *> catalogModules;
        for (const QJsonValue &repoVal : catalogArray)
        {
            if (!repoVal.isObject())
                continue;
            QJsonObject repoObj = repoVal.toObject();
            CatalogModule *catalogModule = new CatalogModule();
            catalogModule->name = repoObj.value("name").toString();
            catalogModule->shortDescriptionHTML = repoObj.value("shortDescriptionHTML").toString();
            catalogModule->homepageUrl = repoObj.value("homepageUrl").toString();
            catalogModule->organization = repoObj.value("organization").toString();
            catalogModule->releaseSource = repoObj.value("releaseSource").toString();
            // channels
            QJsonArray channelsArr = repoObj.value("channels").toArray();
            for (const QJsonValue &ch : channelsArr)
                catalogModule->channels.append(ch.toString());
            // releases
            QJsonArray releasesArr = repoObj.value("releases").toArray();
            for (const QJsonValue &relVal : releasesArr)
            {
                if (!relVal.isObject())
                    continue;
                QJsonObject relObj = relVal.toObject();
                Release *rel = new Release();
                rel->version = relObj.value("version").toString();
                rel->publishedAt = relObj.value("publishedAt").toString();
                rel->jaspVersionRange = relObj.value("jaspVersionRange").toString();
                // assets
                QJsonArray assetsArr = relObj.value("assets").toArray();
                for (const QJsonValue &assetVal : assetsArr)
                {
                    if (!assetVal.isObject())
                        continue;
                    QJsonObject assetObj = assetVal.toObject();
                    QString assetArch = assetObj.value("architecture").toString();
                    if (assetArch != QString::fromStdString(myArchitecture))
                        continue; // skip asset if architecture does not match
                    Asset *asset = new Asset();
                    asset->downloadUrl = assetObj.value("downloadUrl").toString();
                    asset->downloadCount = assetObj.value("downloadCount").toInt();
                    asset->architecture = assetArch;
                    rel->assets.append(asset);
                }
                repo->releases.append(rel);
            }
            // TODO remove module with no releases or assets

            // only list prerelease when developer mode is on
            auto isDeveloperMode = PreferencesModel::prefs()->developerMode();
            if (isDeveloperMode)
            {
                //
                // preReleases
                QJsonArray preReleasesArr = repoObj.value("preReleases").toArray();
                for (const QJsonValue &relVal : preReleasesArr)
                {
                    if (!relVal.isObject())
                        continue;
                    QJsonObject relObj = relVal.toObject();
                    Release *rel = new Release();
                    rel->version = relObj.value("version").toString();
                    rel->publishedAt = relObj.value("publishedAt").toString();
                    rel->jaspVersionRange = relObj.value("jaspVersionRange").toString();
                    // assets
                    QJsonArray assetsArr = relObj.value("assets").toArray();
                    for (const QJsonValue &assetVal : assetsArr)
                    {
                        if (!assetVal.isObject())
                            continue;
                        QJsonObject assetObj = assetVal.toObject();
                        QString assetArch = assetObj.value("architecture").toString();
                        if (assetArch != QString::fromStdString(myArchitecture))
                            continue; // skip asset if architecture does not match
                        Asset *asset = new Asset();
                        asset->downloadUrl = assetObj.value("downloadUrl").toString();
                        asset->downloadCount = assetObj.value("downloadCount").toInt();
                        asset->architecture = assetArch;
                        rel->assets.append(asset);
                    }
                    catalogModule->preReleases.append(rel);
                }
            }
        }
        catalogModules.append(catalogModule);
    }
    m_catalogModules = catalogModules;
}

void Catalog::loadCatalog()
{

    // Load catalog.json from cache dir
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString filePath = cacheDir + "/catalog.json";
    QFile file(filePath);
    if (!file.exists())
    {
        qWarning() << "No cached catalog found in" << cacheDir;
        return;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open cached catalog:" << file.errorString();
        return;
    }
    QByteArray data = file.readAll();
    file.close();
    updateLastUpdatedAt();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "Failed to parse catalog JSON:" << parseError.errorString();
        return;
    }
    if (!doc.isArray())
    {
        qWarning() << "Catalog JSON is not an array!";
        return;
    }
    QJsonArray catalogArray = doc.array();
    qDebug() << "Loaded catalog from cache with" << catalogArray.size() << "entries.";

    json2catalogModules(catalogArray);
    emit catalogModulesChanged();
QList<CatalogModule*> Catalog::catalogModules() const
{
    return m_catalogModules;
}
}

QString Catalog::lastUpdatedAt() const
{
    return m_lastUpdatedAt;
}

void Catalog::updateLastUpdatedAt()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString filePath = cacheDir + "/catalog.json";
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists())
    {
        m_lastUpdatedAt = fileInfo.lastModified().toString(Qt::ISODate);
    }
    else
    {
        m_lastUpdatedAt.clear();
    }
    emit lastUpdatedAtChanged();
}

std::map<std::string, std::string> Catalog::getInstalledModuleVersions()
{
	std::map<std::string, std::string> moduleVersionMap;

	auto parseManifests = [&](const std::string& path) {
		auto dir = QDir(path.c_str());
		dir.cdUp(); dir.cd("manifests");
		if(!dir.exists())
			return;

		auto manifests = dir.entryList({"jasp*.json"}, QDir::Files);
		for(auto& manifest : manifests) {
			std::ifstream in(dir.absoluteFilePath(manifest).toStdString());
			Json::Value root;
			Json::Reader().parse(in, root);
			Json::Value version = root["version"][0];
			std::string strVersion = "";
			for(int i = 0; i < version.size(); i++) {
				auto x = version[i];
				strVersion =  strVersion + version[i].asString() + ".";
			}
			strVersion.pop_back();
			moduleVersionMap[root["name"].asString()] = strVersion;
		}

	};

	parseManifests(AppDirs::bundledModulesLibDir().toStdString());
	parseManifests(AppDirs::userModulesLibDir().toStdString());

	return moduleVersionMap;
}