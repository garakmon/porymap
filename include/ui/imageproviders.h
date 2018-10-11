#ifndef IMAGEPROVIDERS_H
#define IMAGEPROVIDERS_H

#include "block.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>

QImage getCollisionMetatileImage(Block);
QImage getCollisionMetatileImage(int, int);
QImage getMetatileImage(uint16_t, Tileset*, Tileset*);
//QImage getMetatileImage(uint16_t, Tileset*);
QImage getTileImage(uint16_t, Tileset*, Tileset*);
QImage getTileImage(uint8_t, Tileset*);
QImage getColoredTileImage(uint16_t, Tileset*, Tileset*, int);

#endif // IMAGEPROVIDERS_H
