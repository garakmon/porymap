#include "imageproviders.h"
#include "tilemaptileselector.h"
#include "project.h"
#include <QPainter>

QPoint TilemapTileSelector::getSelectionDimensions() {
    //
}

void TilemapTileSelector::draw() {
    //*
    if (!this->tileset || !this->tileset->metatiles) {
        this->setPixmap(QPixmap());
    }

    int length_ = 64;//this->tileset->metatiles->length();
    int height_ = 8;//length_ / this->numMetatilesWide;

    QImage image(8 * 16, height_ * 16, QImage::Format_RGBA8888);
    QPainter painter(&image);
    
    for (int i = 0; i < length_; i++) {
        int tile = i;
        //if (i >= le) {
        //    tile += Project::getNumMetatilesPrimary() - primaryLength;
        //}
        QImage metatile_image = getTileImage(tile, this->tileset);//getMetatileImage(tile, this->, this->);
        int map_y = i / 8;//this->numMetatilesWide;
        int map_x = i % 8;//this->numMetatilesWide;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));
    //*/
}

void TilemapTileSelector::select(uint8_t tile) {
    //
}

void TilemapTileSelector::setTileset(Tileset* tileset) {
    //
    this->tileset = tileset;
    //this->draw();
}

uint8_t TilemapTileSelector::getSelectedTile() {
    //
    //return tileId;
}

void TilemapTileSelector::setExternalSelection(int, int, QList<uint8_t>*) {
    //
}

void TilemapTileSelector::updateSelectedTile() {
    //
}

uint8_t TilemapTileSelector::getTileId(int x, int y) {
    //
}

QPoint TilemapTileSelector::getTileIdCoords(uint8_t) {
    //
}

void TilemapTileSelector::mousePressEvent(QGraphicsSceneMouseEvent*) {
    //
}

void TilemapTileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent*) {
    //
}

void TilemapTileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent*) {
    //
}

void TilemapTileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent*) {
    //
}

void TilemapTileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    //
}

/*
void TilemapTileSelector::hoveredTilemapTileSelectionChanged(uint8_t) {
    //
}

void TilemapTileSelector::hoveredTilemapTileSelectionCleared() {
    //
}

void TilemapTileSelector::selectedTilemapTilesChanged() {
    //
}
*/
