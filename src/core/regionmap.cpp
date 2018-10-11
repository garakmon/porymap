#include "regionmap.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>



void RegionMap::init(QString path) {
    //
    // TODO: in the future, allow these to be adjustable (and save values)
    // possibly use a config file?
    width_ = 28;
    height_ = 15;

    img_width_ = width_ + 4;
    img_height_ = height_ + 5;

    //city_map_squares_path = QString();
    temp_path = path;// delete this
    region_map_bin_path = path + "/graphics/pokenav/region_map_map.bin";
    region_map_tilemap_path = path + "/graphics/pokenav/region_map.png";
    region_map_layout_path = path + "/src/data/region_map_layout.h";

    //readBkgImgBin();
    //readLayout();

    //loadBkgImgTileset();

    //saveBkgImgBin();
    //saveLayout();
}

void RegionMap::readBkgImgBin() {
    QFile binFile(region_map_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    QList<uint8_t> *bkgimg_tiles = new QList<uint8_t>;

    // the two is because lines are skipped for some reason
    // (maybe that is because there could be multiple layers?)
    // background image is also 32x20
    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            bkgimg_tiles->append(mapBinData.at(n + m * img_width_ * 2));
        }
    }
    background_image_tiles = bkgimg_tiles;
}

void RegionMap::saveBkgImgBin() {
    QByteArray data(4096,0);

    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            data[n + m * img_width_ * 2] = (*background_image_tiles)[n + m * img_width_];
        }
    }

    QFile file(region_map_bin_path);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(data);
    file.close();
}

/*
    QString name;
    QString is_compressed;
    QString is_secondary;
    QString padding;
    QString tiles_label;
    QString palettes_label;
    QString metatiles_label;
    QString metatiles_path;
    QString callback_label;
    QString metatile_attrs_label;
    QString metatile_attrs_path;
    QString tilesImagePath;
    QImage tilesImage;
    QList<QString> palettePaths;

    QList<QImage> *tiles = nullptr;
    QList<Metatile*> *metatiles = nullptr;
    QList<QList<QRgb>> *palettes = nullptr;
*/
void RegionMap::loadBkgImgTileset() {
    //
    // path: region_map_tilemap_path
    // set values of tileset_bkg_img
    //QImage *image = new QImage("region_map_tilemap_path");
    //image.load();
    //QList<QImage> *tiles = new QList<QImage>;
    //int w = 1;
    //int h = 1;
    //for (int y = 0; y < 2; y+=h) {//image.height(); y += h) {
    //    for (int x = 0; x < 2; x += w) {//image.width(); x += w) {
        //    QImage tile = image.copy(x, y, w, h);
        //    tiles->append(tile);
    //    }
    //}
    //tileset->tilesImage = image;
    //tileset_bkg_img->tiles = tiles;
}

// done
void RegionMap::readLayout() {
    QFile file(region_map_layout_path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QMap<QString, QString> * abbr = new QMap<QString, QString>;

    QString line, text;
    QStringList *captured = new QStringList;

    QTextStream in(&file);
    while (!in.atEnd()) {
        line = in.readLine();
        if (line.startsWith("#define")) {
            QStringList split = line.split(QRegularExpression("\\s+"));
            abbr->insert(split[2].replace("MAPSEC_",""), split[1]);
        } else {
            text += line.remove(" ");
        }
    }
    QRegularExpression re("{(.*?)}");
    *captured = re.match(text).captured(1).split(",");
    captured->removeAll({});

    // replace abbreviations with names
    for (int i = 0; i < captured->length(); i++) {
        QString value = (*captured)[i];
        if (value.startsWith("R(")) {// routes are different
            captured->replace(i, QString("ROUTE_%1").arg(value.mid(2,3)));
        } else {
            captured->replace(i, abbr->key(value));
        }
    }
    mapname_abbr = abbr;
    layout_map_names = captured;
    file.close();
}

// does it matter that it doesn't save in order?
// do i need to use a QList<Pair> ??
void RegionMap::saveLayout() {
    //
    QString layout_text = "";
    QString mapsec      = "MAPSEC_";
    QString define      = "#define ";
    QString array_start = "static const u8 sRegionMapLayout[] =\n{";
    QString array_close = "\n};\n";
    QString tab         = "    ";

    for (QString key : mapname_abbr->keys()) {
        layout_text += define + mapname_abbr->value(key) + tab + mapsec + key + "\n";
    }

    layout_text += "\n" + array_start;// +  + array_close;//oops

    //qDebug() << *layout_map_names;
    int cnt = 0;
    for (QString s : *layout_map_names) {
        //
        if (!(cnt % width_)) {
            layout_text += "\n" + tab;
        }
        if (s.startsWith("ROUTE_")) {
            layout_text += QString("R(%1)").arg(s.replace("ROUTE_","")) + ", ";
        } else {
            layout_text += mapname_abbr->value(s) + ", ";
        }
        cnt++;
    }

    //layout_text.
    layout_text += array_close;
    
    QFile file(region_map_layout_path);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(layout_text.toUtf8());
    file.close();
}

//done
QString RegionMap::newAbbr(QString mapname) {
    QString abbr;
    QStringList words = mapname.split("_");

    if (words.length() == 1) {
        abbr = (words[0] + "_____X").left(6);
    } else {
        abbr = (words.front() + "___X").left(4) + "_" + words.back().at(0);
    }

    // to guarantee unique abbreviations (up to 14)
    QString extra_chars = "23456789BCDEF";
    int count = 0;
    while ((*mapname_abbr).values().contains(abbr)) {
        abbr.replace(5,1,extra_chars[count]);
        count++;
    }
    return abbr;
}

// only use for layout! image will need to add 4 to width
int RegionMap::index_(int x, int y) {
    return (x + y * width_ * 2);
}











































