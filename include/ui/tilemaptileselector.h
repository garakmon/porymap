#ifndef TILEMAPTILESELECTOR_H
#define TILEMAPTILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

/*    TilemapItemSelector class for the region map background image tiles.
 *
 */
class TilemapTileSelector : public SelectablePixmapItem {
    Q_OBJECT
public:
    TilemapTileSelector(int numMetatilesWide, Tileset *tileset) : SelectablePixmapItem(16, 16) {
        this->numMetatilesWide = numMetatilesWide;
        this->tileset = tileset;
        setAcceptHoverEvents(true);
    }
    QPoint getSelectionDimensions();
    void draw();
    void select(uint8_t);// tile
    void setTileset(Tileset*);
    uint8_t getSelectedTile();
    void setExternalSelection(int, int, QList<uint8_t>*);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
private:
    //bool externalSelection;
    int numMetatilesWide; // make it constant?
    Tileset *tileset;
    //Tileset *secondaryTileset;
    uint8_t selectedTile;
    //int externalSelectionWidth;
    //int externalSelectionHeight;
    //QList<uint8_t> *externalSelectedMetatiles;

    void updateSelectedTile();
    uint8_t getTileId(int x, int y);
    QPoint getTileIdCoords(uint8_t);

signals:
    void hoveredTilemapTileSelectionChanged(uint8_t);
    void hoveredTilemapTileSelectionCleared();
    void selectedTilemapTilesChanged();
};

#endif // TILEMAPTILESELECTOR_H
