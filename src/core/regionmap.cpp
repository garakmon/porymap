#include "regionmap.h"
#include "log.h"
#include "config.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QImage>



void RegionMap::init(Project *pro) {
    QString path = pro->root;
    this->project = pro;

    QSize dimensions = porymapConfig.getRegionMapDimensions();
    img_width_ = dimensions.width();
    img_height_ = dimensions.height();

    layout_width_  = img_width_  - this->padLeft - this->padRight;
    layout_height_ = img_height_ - this->padTop  - this->padBottom;

    region_map_bin_path        = path + "/graphics/pokenav/region_map_map.bin";
    region_map_png_path        = path + "/graphics/pokenav/region_map.png";
    region_map_entries_path    = path + "/src/data/region_map/region_map_entries.h";
    region_map_layout_bin_path = path + "/graphics/pokenav/region_map_section_layout.bin";
    city_map_tiles_path        = path + "/graphics/pokenav/zoom_tiles.png";

    readBkgImgBin();
    readLayout();
    readCityMaps();
}

// TODO: if the tileId is not valid for the provided image, make sure it does not crash
void RegionMap::readBkgImgBin() {
    QFile binFile(region_map_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    if (mapBinData.size() < img_height_ * img_width_) {
        logError(QString("The region map tilemap at %1 is too small.").arg(region_map_bin_path));
        return;
    }
    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            RegionMapSquare square;
            square.tile_img_id = mapBinData.at(n + m * img_width_ * 2);
            map_squares.append(square);
        }
    }
}

void RegionMap::saveBkgImgBin() {
    QByteArray data(4096,0);// use a constant here? maybe read the original size?

    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            data[n + m * img_width_ * 2] = map_squares[n + m * img_width_].tile_img_id;
        }
    }

    QFile file(region_map_bin_path);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(data);
    file.close();
}

// TODO: reorganize this into project? the i/o stuff. use regionMapSections
void RegionMap::readLayout() {
    //
    QFile file(region_map_entries_path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QString line;

    QMap<QString, QString> *qmap = new QMap<QString, QString>;

    QTextStream in(&file);
    while (!in.atEnd()) {
        line = in.readLine();
        if (line.startsWith("static const u8")) {
            QRegularExpression reBefore("sMapName_(.*)\\[");
            QRegularExpression reAfter("_\\(\"(.*)\"");
            QString const_name = reBefore.match(line).captured(1);
            QString full_name = reAfter.match(line).captured(1);
            sMapNames.append(const_name);
            sMapNamesMap.insert(const_name, full_name);
        } else if (line.contains("MAPSEC")) {
            QRegularExpression reBefore("\\[(.*)\\]");
            QRegularExpression reAfter("{(.*)}");
            QStringList entry =  reAfter.match(line).captured(1).remove(" ").split(",");
            QString mapsec = reBefore.match(line).captured(1);
            QString insertion = entry[4].remove("sMapName_");
            qmap->insert(mapsec, sMapNamesMap.value(insertion));
            mapSecToMapEntry[mapsec] = {
            //  x                 y                 width             height            name
                entry[0].toInt(), entry[1].toInt(), entry[2].toInt(), entry[3].toInt(), insertion
            };
        }
    }
    file.close();

    project->mapSecToMapHoverName = qmap;

    QFile binFile(region_map_layout_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) return;
    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    // TODO: improve this?
    for (int m = 0; m < layout_height_; m++) {
        for (int n = 0; n < layout_width_; n++) {
            int i = img_index_(n,m);
            map_squares[i].secid = static_cast<uint8_t>(mapBinData.at(layout_index_(n,m)));
            QString secname = (*(project->regionMapSections))[static_cast<uint8_t>(mapBinData.at(layout_index_(n,m)))];
            if (secname != "MAPSEC_NONE") map_squares[i].has_map = true;
            map_squares[i].mapsec = secname;
            map_squares[i].map_name = sMapNamesMap.value(mapSecToMapEntry.value(secname).name);// TODO: this is atrocious
            map_squares[i].x = n;
            map_squares[i].y = m;
        }
    }
}

void RegionMap::saveLayout() {
    QString entries_text;
    QString layout_text;

    entries_text += "#ifndef GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n";
    entries_text += "#define GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n\n";

    for (auto sName : sMapNames) {
        entries_text += "static const u8 sMapName_" + sName + "[] = _(\"" + sMapNamesMap.value(sName) + "\");\n";
    }

    entries_text += "\nconst struct RegionMapLocation gRegionMapEntries[] = {\n";

    for (auto sec : *(project->regionMapSections)) {
        if (!mapSecToMapEntry.contains(sec)) continue;
        struct RegionMapEntry entry = mapSecToMapEntry.value(sec);
        entries_text += "    [" + sec + "] = {" + QString::number(entry.x) + ", " + QString::number(entry.y) + ", " 
            +  QString::number(entry.width) + ", " + QString::number(entry.height) + ", sMapName_" + entry.name + "},\n";
    }
    entries_text += "};\n\n#endif // GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n";

    project->saveTextFile(region_map_entries_path, entries_text);

    QByteArray data;
    for (int m = 0; m < layout_height_; m++) {
        for (int n = 0; n < layout_width_; n++) {
            int i = img_index_(n,m);
            data.append(map_squares[i].secid);
        }
    }
    QFile bfile(region_map_layout_bin_path);
    if (!bfile.open(QIODevice::WriteOnly)) return;
    bfile.write(data);
    bfile.close();
}

void RegionMap::readCityMaps() {}

// layout coords to image index
int RegionMap::img_index_(int x, int y) {
    return ((x + this->padLeft) + (y + this->padTop) * img_width_);
}

// layout coords to layout index
int RegionMap::layout_index_(int x, int y) {
    return (x + y * layout_width_);
}

int RegionMap::width() {
    return this->img_width_;
}

int RegionMap::height() {
    return this->img_height_;
}

QSize RegionMap::imgSize() {
    return QSize(img_width_ * 8 + 2, img_height_ * 8 + 2);
}

// TODO: rename to getTileIdAt()?
unsigned RegionMap::getTileId(int x, int y) {
    return map_squares[x + y * img_width_].tile_img_id;
}

void RegionMap::save() {
    logInfo("Saving region map info.");
    saveBkgImgBin();
    saveLayout();
    porymapConfig.setRegionMapDimensions(this->img_width_, this->img_height_);
}

void RegionMap::saveOptions(int id, QString sec, QString name, int x, int y) {
    resetSquare(id);
    int index = getMapSquareIndex(x + this->padLeft, y + this->padTop);
    if (!sec.isEmpty()) {
        this->map_squares[index].has_map = true;
        this->map_squares[index].secid = static_cast<uint8_t>(project->regionMapSections->indexOf(sec));
        this->map_squares[index].mapsec = sec;
        if (!name.isEmpty()) {
            this->map_squares[index].map_name = name;
            this->project->mapSecToMapHoverName->insert(sec, name);
            if (!mapSecToMapEntry.keys().contains(sec)) {
                QString sName = fix_case(sec);
                sMapNames.append(sName);
                sMapNamesMap.insert(sName, name);
                struct RegionMapEntry entry = {x, y, 1, 1, sName};// TODO: change width, height?
                mapSecToMapEntry.insert(sec, entry);
            }
        }
        this->map_squares[index].x = x;
        this->map_squares[index].y = y;
        this->map_squares[index].duplicated = false;
    }
    //resetSquare(id);
}

// from x, y of image
int RegionMap::getMapSquareIndex(int x, int y) {
    int index = (x + y * img_width_);
    return index < map_squares.length() ? index : 0;
}

// For turning a MAPSEC_NAME into a unique identifier sMapName-style variable.
QString RegionMap::fix_case(QString caps) {
    bool big = true;
    QString camel;

    for (auto ch : caps.remove(QRegularExpression("({.*})")).remove("MAPSEC")) {
        if (ch == '_' || ch == ' ') {
            big = true;
            continue;
        }
        if (big) {
            camel += ch.toUpper();
            big = false;
        }
        else camel += ch.toLower();
    }
    return camel;
}

void RegionMap::resetSquare(int index) {
    this->map_squares[index].mapsec = "MAPSEC_NONE";
    this->map_squares[index].map_name = QString();
    this->map_squares[index].has_map = false;
    this->map_squares[index].secid = static_cast<uint8_t>(project->regionMapSections->indexOf("MAPSEC_NONE"));
    this->map_squares[index].has_city_map = false;
    this->map_squares[index].city_map_name = QString();
    this->map_squares[index].duplicated = false;
    logInfo(QString("Reset map square at (%1, %2).").arg(this->map_squares[index].x).arg(this->map_squares[index].y));
}

void RegionMap::resize(int newWidth, int newHeight) {
    //
    QVector<RegionMapSquare> new_squares;

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            RegionMapSquare square;
            if (x < img_width_ && y < img_height_) {
                square = map_squares[getMapSquareIndex(x, y)];
            } else if (x < newWidth - this->padRight && y < newHeight - this->padBottom) {
                square.tile_img_id = 0;
                square.x = x;
                square.y = y;
                square.mapsec = "MAPSEC_NONE";
            } else {
                square.tile_img_id = 0;
            }
            new_squares.append(square);
        }
    }

    this->map_squares = new_squares;
    this->img_width_ = newWidth;
    this->img_height_ = newHeight;
    this->layout_width_ = newWidth - this->padLeft - this->padRight;
    this->layout_height_ = newHeight - this->padTop - this->padBottom;
}
