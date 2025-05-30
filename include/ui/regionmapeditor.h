#ifndef REGIONMAPEDITOR_H
#define REGIONMAPEDITOR_H

#include "regionmappixmapitem.h"
#include "citymappixmapitem.h"
#include "regionmaplayoutpixmapitem.h"
#include "regionmapentriespixmapitem.h"
#include "regionmap.h"
#include "orderedjson.h"
#include "project.h"

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QCloseEvent>
#include <QResizeEvent>

namespace Ui {
class RegionMapEditor;
}

class RegionMapEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit RegionMapEditor(QWidget *parent = 0, Project *pro = nullptr);
    ~RegionMapEditor();

    bool load(bool silent = false);
    bool setupErrored() const { return setupError; }

    void onRegionMapTileSelectorSelectedTileChanged(unsigned id);
    void onRegionMapTileSelectorHoveredTileChanged(unsigned id);
    void onRegionMapTileSelectorHoveredTileCleared();

    void onRegionMapLayoutSelectedTileChanged(int index);
    void onRegionMapLayoutHoveredTileChanged(int index);
    void onRegionMapLayoutHoveredTileCleared();

    void onRegionMapEntriesSelectedTileChanged(QString) {};
    void onRegionMapEntryDragged(int, int);

    void resizeTilemap(int width, int height);

    bool reconfigure();

    QObjectList shortcutableObjects() const;

public slots:
    void applyUserShortcuts();

private:
    Ui::RegionMapEditor *ui;
    Project *project;

    RegionMap *region_map = nullptr;
    OrderedMap<QString, RegionMap *> region_maps;

    QString configFilepath;

    poryjson::Json rmConfigJson;

    bool configSaved = false;
    bool setupError = false;

    QUndoGroup history;

    int currIndex = 0;
    unsigned selectedImageTile = 0;
    QString activeEntry;

    bool cityMapFirstDraw = true;
    bool regionMapFirstDraw = true;
    bool entriesFirstDraw = true;

    double scaleUpFactor = 2.0;
    double initialScale = 30.0;

    QGraphicsScene *scene_region_map_image   = nullptr;
    QGraphicsScene *scene_region_map_layout  = nullptr;
    QGraphicsScene *scene_region_map_entries = nullptr;
    QGraphicsScene *scene_region_map_tiles   = nullptr;

    TilemapTileSelector *mapsquare_selector_item = nullptr;

    RegionMapEntriesPixmapItem *region_map_entries_item = nullptr;
    RegionMapLayoutPixmapItem *region_map_layout_item = nullptr;
    RegionMapPixmapItem *region_map_item = nullptr;

    bool reload();
    bool setup();
    void clear();

    bool saveRegionMap(RegionMap *map);
    void saveConfig();
    bool loadRegionMapEntries();
    bool saveRegionMapEntries();
    QHash<QString, MapSectionEntry> region_map_entries;

    bool buildConfigDialog();
    poryjson::Json configRegionMapDialog();
    poryjson::Json buildDefaultJson();
    bool verifyConfig(poryjson::Json cfg);

    bool modified();

    void initShortcuts();
    void displayRegionMap();
    void displayRegionMapImage();
    void displayRegionMapLayout();
    void displayRegionMapEntriesImage();
    void displayRegionMapLayoutOptions();
    void updateRegionMapLayoutOptions(int index);
    void displayRegionMapTileSelector();
    void updateLayerDisplayed();
    void displayRegionMapEntryOptions();
    void updateRegionMapEntryOptions(QString);
    void setRegionMap(RegionMap *map);
    void setLocations(const QStringList &locations);

    void restoreWindowState();
    void closeEvent(QCloseEvent* event);

    void setTileHFlip(bool enabled);
    void setTileVFlip(bool enabled);

private slots:
    void on_action_RegionMap_Save_triggered();
    void on_actionSave_All_triggered();
    void on_action_RegionMap_Resize_triggered();
    void on_action_RegionMap_ClearImage_triggered();
    void on_action_RegionMap_ClearLayout_triggered();
    void on_action_RegionMap_ClearEntries_triggered();
    void on_action_Swap_triggered();
    void on_action_Replace_triggered();
    void on_action_Configure_triggered();
    void on_tabWidget_Region_Map_currentChanged(int);
    void on_pushButton_RM_Options_delete_clicked();
    void on_comboBox_RM_ConnectedMap_textActivated(const QString &);
    void on_comboBox_RM_Entry_MapSection_currentTextChanged(const QString &);
    void on_comboBox_regionSelector_textActivated(const QString &);
    void on_comboBox_layoutLayer_textActivated(const QString &);
    void on_spinBox_RM_Entry_x_valueChanged(int);
    void on_spinBox_RM_Entry_y_valueChanged(int);
    void on_spinBox_RM_Entry_width_valueChanged(int);
    void on_spinBox_RM_Entry_height_valueChanged(int);
    void on_pushButton_entryActivate_clicked();
    void on_spinBox_RM_LayoutWidth_valueChanged(int);
    void on_spinBox_RM_LayoutHeight_valueChanged(int);
    void on_spinBox_tilePalette_valueChanged(int);
    void on_verticalSlider_Zoom_Map_Image_valueChanged(int);
    void on_verticalSlider_Zoom_Image_Tiles_valueChanged(int);
    void onHoveredRegionMapTileChanged(int x, int y);
    void onHoveredRegionMapTileCleared();
    void mouseEvent_region_map(QGraphicsSceneMouseEvent *event, RegionMapPixmapItem *item);
};

#endif // REGIONMAPEDITOR_H
