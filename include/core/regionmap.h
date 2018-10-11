#ifndef REGIONMAP_H
#define REGIONMAP_H


#include "map.h"
#include "tilemaptileselector.h"
//#include "block.h"

#include <QStringList>
#include <QString>
#include <QList>
#include <QMap>
#include <QGraphicsScene>
#include <QGraphicsView>

// if editing map bins, will need to remake the graphics when editing
// are the scenes set in the editor / project / mainwindow files?

/*
 *   - display the region map background image
 *   - edit the region_map_layout.h layout
 *   - edit city maps metatile layout and JUST save the mapname_num.bin
 *   - edit 
 *   who edits pokenav_city_maps 1 and 2?
 *   users can: - add the incbins probably themselves
 *              - add
 *              - edit region map background image
 * 
 * 
 * 
 * 
 *    Editor:
 *        - void displayCityMapMetatileSelector
 *        - void displayRegionMapTileSelector
 *        - void selectRegionMapTile(QString mapname)
 *        - QGraphicsScene *scene_city_map_metatiles
 *        - TilemapTileSelector *city_map_metatile_selector_item
 *        - Tileset *city_map_squares (or tileset_city_map?)
 *        - Tileset *tileset_region_map
 * 
 *    MainWindow:
 * 
 * 
 *    Project:
 *    
 */

class RegionMap : public QObject
{
    Q_OBJECT
//public:
//    explicit Map(QObject *parent = nullptr);

public:
    RegionMap() = default;

    ~RegionMap() {
        delete mapname_abbr;
        delete layout_map_names;
        delete background_image_tiles;
        delete background_image_selector_item;
    };

    QString temp_path;// delete this
    QString city_map_squares_path;
    QString region_map_tilemap_path;
    QString region_map_bin_path;// = QString::null;
    QString city_map_header_path;//dafuq is this?
    QString region_map_layout_path;

    Tileset *tileset_bkg_img;
    Tileset *tileset_city_map;

    //QMap<QString, somthing> something;// name of map : info about city map, position in layoit, etc.
    //QMap<QString, TilemapTile*> regionMapLayoutTng; // mapName : tilemaptileselector
    // maybe position data to select correct square when changing map on side but only if map is a valid
    QList<uint8_t>         *background_image_tiles;// the visible ones anyways // using list because replace
    TilemapTileSelector    *background_image_selector_item;// ?
    QMap<QString, QString> *mapname_abbr;// layout shortcuts mapname:region_map_layout defines (both ways)
    // make this a QHash?? <-- no because something
    QStringList            *layout_map_names;

    bool hasUnsavedChanges();

    void init(QString);

    // parseutil.cpp ?
    void readBkgImgBin();
    void loadBkgImgTileset();
    void readCityMaps();// more complicated
    void loadCityMapTileset();
    void readLayout();

    QString newAbbr(QString);// makes a *unique* 5 character abbreviation from mapname to add to mapname_abbr

    // editing functions
    // if they are booleans, returns true if successful?
    bool placeTile(char, int, int);// place tile at x, y
    bool removeTile(char, int, int);// replaces with 0x00 byte at x,y
    bool placeMap(QString, int, int);
    bool removeMap(QString, int, int);
    bool removeMap(QString);// remove all instances of map

    void save();
    void saveBkgImgBin();
    void saveLayout();
    void saveCityMap();

    void resize(int, int);
    void setWidth(int);
    void setHeight(int);
    int  width();
    int  height();

    // implement these here?
    void undo();
    void redo();

private:
    //
    int width_;
    int height_;
    int img_width_;
    int img_height_;
    int index_(int, int);// returns index int at x,y args (x + y * width_ * 2) // 2 because 

//protected:
    //

//signals:
    //
};

//TilemapTileSelector *city_map_metatile_selector_item = nullptr;

#endif // REGIONMAP_H
