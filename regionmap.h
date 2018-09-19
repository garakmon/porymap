#ifndef REGIONMAP_H
#define REGIONMAP_H

#include "map.h"
//#include "block.h"

#include <QGraphicsScene>
#include <QGraphicsView>

// if editing map bins, will need to remake the graphics when editing

/*
    - display the region map background image
    - edit the region_map_layout.h layout
    - edit city maps metatile layout and JUST save the mapname_num.bin
    - edit 

    who edits pokenav_city_maps 1 and 2?

    users can: - add the incbins probably themselves
               - add
               - edit region map background image
*/

// this is the background image tile
class RegionMapBackgroundTile; // : public QObject, public QGraphicsPixmapItem
// x, y, metatile
// metatile image?

// this is the basic city map 
class CityMap;
// the

// this is the connections part with the maps and locations, etc.
// better name for this?
class RegionMapLayoutTile // : public QObject, public QGraphicsPixmapItem
// bool has something
// bool is flyable
// heallocation, 
// zoomed view: NULL or image bin
// region map width height (28x15) defines
{
    //
    bool has_data;// rename
    bool is_flyabe;// if pokenav overlay paints over
    bool is_city;// has a city map (TRUE)
    QString hover_over_name;// rename (just map_name?)
};

// the region map - complete map with *RegionMapPixmapItem and *RegionMapLayoverItem
class RegionMap 
{
public:
    QList<QString> cities; // names of maps with cities / zoom view
    QMap<QString, QString> shortcuts; // defines in region_map_layout.h
    //QGraphicsScene *scene = NULL;

    QGraphicsScene *scene_region_map_background = NULL; // nullptr
    QGraphicsScene *scene_region_map_overlay = NULL;
    QGraphicsScene *scene_region_map = NULL; // background with overlay on top

    QGraphicsScene *scene_city_map_zoomed_view = NULL;

public:
    void render();
    void update_something();

private:
    void private_member();
};
// width, height, *layout_tiles, background_tiles
// QImage background_image

#endif // REGIONMAP_H
