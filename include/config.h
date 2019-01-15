#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QObject>
#include <QSize>

enum MapSortOrder {
    Group   =  0,
    Area    =  1,
    Layout  =  2,
};

class KeyValueConfigBase : public QObject
{
public:
    void save();
    void load();
    virtual ~KeyValueConfigBase();
protected:
    virtual QString getConfigFilepath() = 0;
    virtual void parseConfigKeyValue(QString key, QString value) = 0;
    virtual QMap<QString, QString> getKeyValueMap() = 0;
    virtual void onNewConfigFileCreated() = 0;
};

class PorymapConfig: public KeyValueConfigBase
{
public:
    PorymapConfig() {
        this->recentProject = "";
        this->recentMap = "";
        this->mapSortOrder = MapSortOrder::Group;
        this->prettyCursors = true;
        this->collisionOpacity = 50;
        this->regionMapDimensions = QSize(32, 20);
    }
    void setRecentProject(QString project);
    void setRecentMap(QString map);
    void setMapSortOrder(MapSortOrder order);
    void setPrettyCursors(bool enabled);
    void setCollisionOpacity(int opacity);
    void setRegionMapDimensions(int width, int height);
    QString getRecentProject();
    QString getRecentMap();
    MapSortOrder getMapSortOrder();
    bool getPrettyCursors();
    int getCollisionOpacity();
    QSize getRegionMapDimensions();
protected:
    QString getConfigFilepath();
    void parseConfigKeyValue(QString key, QString value);
    QMap<QString, QString> getKeyValueMap();
    void onNewConfigFileCreated() {}
private:
    QString recentProject;
    QString recentMap;
    MapSortOrder mapSortOrder;
    bool prettyCursors;
    int collisionOpacity;
    QSize regionMapDimensions;
};

extern PorymapConfig porymapConfig;

enum BaseGameVersion {
    pokeruby,
    pokeemerald,
};

class ProjectConfig: public KeyValueConfigBase
{
public:
    ProjectConfig() {
        this->baseGameVersion = BaseGameVersion::pokeemerald;
    }
    void setBaseGameVersion(BaseGameVersion baseGameVersion);
    BaseGameVersion getBaseGameVersion();
    void setProjectDir(QString projectDir);
protected:
    QString getConfigFilepath();
    void parseConfigKeyValue(QString key, QString value);
    QMap<QString, QString> getKeyValueMap();
    void onNewConfigFileCreated();
private:
    BaseGameVersion baseGameVersion;
    QString projectDir;
};

extern ProjectConfig projectConfig;

#endif // CONFIG_H
