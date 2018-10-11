#include "editor.h"
#include "event.h"
#include "imageproviders.h"
#include "mapconnection.h"
#include "currentselectedmetatilespixmapitem.h"
#include "mapsceneeventfilter.h"
#include <QCheckBox>
#include <QPainter>
#include <QMouseEvent>
#include <math.h>

static bool selectingEvent = false;

Editor::Editor(Ui::MainWindow* ui)
{
    this->ui = ui;
    this->selected_events = new QList<DraggablePixmapItem*>;
    this->settings = new Settings();
}

void Editor::saveProject() {
    if (project) {
        project->saveAllMaps();
        project->saveAllDataStructures();
    }
}

void Editor::save() {
    if (project && map) {
        project->saveMap(map);
        project->saveAllDataStructures();
    }
}

void Editor::undo() {
    if (current_view) {
        map->undo();
        map_item->draw();
        collision_item->draw();
    }
}

void Editor::redo() {
    if (current_view) {
        map->redo();
        map_item->draw();
        collision_item->draw();
    }
}

void Editor::setEditingMap() {
    current_view = map_item;
    if (map_item) {
        displayMapConnections();
        map_item->draw();
        map_item->setVisible(true);
        map_item->setEnabled(true);
        setConnectionsVisibility(ui->checkBox_ToggleBorder->isChecked());
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
    setConnectionItemsVisible(false);
}

void Editor::setEditingCollision() {
    current_view = collision_item;
    if (collision_item) {
        displayMapConnections();
        collision_item->draw();
        collision_item->setVisible(true);
        setConnectionsVisibility(ui->checkBox_ToggleBorder->isChecked());
    }
    if (map_item) {
        map_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
    setConnectionItemsVisible(false);
}

void Editor::setEditingObjects() {
    current_view = map_item;
    if (events_group) {
        events_group->setVisible(true);
    }
    if (map_item) {
        map_item->setVisible(true);
        map_item->setEnabled(false);
        setConnectionsVisibility(ui->checkBox_ToggleBorder->isChecked());
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
    setConnectionItemsVisible(false);
}

void Editor::setEditingConnections() {
    current_view = map_item;
    if (map_item) {
        map_item->draw();
        map_item->setVisible(true);
        map_item->setEnabled(false);
        populateConnectionMapPickers();
        ui->label_NumConnections->setText(QString::number(map->connections.length()));
        setConnectionsVisibility(false);
        setDiveEmergeControls();
        bool controlsEnabled = selected_connection_item != nullptr;
        setConnectionEditControlsEnabled(controlsEnabled);
        if (selected_connection_item) {
            onConnectionOffsetChanged(selected_connection_item->connection->offset.toInt());
            setConnectionMap(selected_connection_item->connection->map_name);
            setCurrentConnectionDirection(selected_connection_item->connection->direction);
        }
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(true, 0.4);
    setConnectionItemsVisible(true);
}

void Editor::setDiveEmergeControls() {
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);
    ui->comboBox_DiveMap->setCurrentText("");
    ui->comboBox_EmergeMap->setCurrentText("");
    for (MapConnection* connection : map->connections) {
        if (connection->direction == "dive") {
            ui->comboBox_DiveMap->setCurrentText(connection->map_name);
        } else if (connection->direction == "emerge") {
            ui->comboBox_EmergeMap->setCurrentText(connection->map_name);
        }
    }
    ui->comboBox_DiveMap->blockSignals(false);
    ui->comboBox_EmergeMap->blockSignals(false);
}

void Editor::populateConnectionMapPickers() {
    ui->comboBox_ConnectedMap->blockSignals(true);
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);

    ui->comboBox_ConnectedMap->clear();
    ui->comboBox_ConnectedMap->addItems(*project->mapNames);
    ui->comboBox_DiveMap->clear();
    ui->comboBox_DiveMap->addItems(*project->mapNames);
    ui->comboBox_EmergeMap->clear();
    ui->comboBox_EmergeMap->addItems(*project->mapNames);

    ui->comboBox_ConnectedMap->blockSignals(false);
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);
}

void Editor::setConnectionItemsVisible(bool visible) {
    for (ConnectionPixmapItem* item : connection_edit_items) {
        item->setVisible(visible);
        item->setEnabled(visible);
    }
}

void Editor::setBorderItemsVisible(bool visible, qreal opacity) {
    for (QGraphicsPixmapItem* item : borderItems) {
        item->setVisible(visible);
        item->setOpacity(opacity);
    }
}

void Editor::setCurrentConnectionDirection(QString curDirection) {
    if (!selected_connection_item)
        return;

    selected_connection_item->connection->direction = curDirection;

    Map *connected_map = project->getMap(selected_connection_item->connection->map_name);
    QPixmap pixmap = connected_map->renderConnection(*selected_connection_item->connection);
    int offset = selected_connection_item->connection->offset.toInt(nullptr, 0);
    selected_connection_item->initialOffset = offset;
    int x = 0, y = 0;
    if (selected_connection_item->connection->direction == "up") {
        x = offset * 16;
        y = -pixmap.height();
    } else if (selected_connection_item->connection->direction == "down") {
        x = offset * 16;
        y = map->getHeight() * 16;
    } else if (selected_connection_item->connection->direction == "left") {
        x = -pixmap.width();
        y = offset * 16;
    } else if (selected_connection_item->connection->direction == "right") {
        x = map->getWidth() * 16;
        y = offset * 16;
    }

    selected_connection_item->basePixmap = pixmap;
    QPainter painter(&pixmap);
    painter.setPen(QColor(255, 0, 255));
    painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    painter.end();
    selected_connection_item->setPixmap(pixmap);
    selected_connection_item->initialX = x;
    selected_connection_item->initialY = y;
    selected_connection_item->blockSignals(true);
    selected_connection_item->setX(x);
    selected_connection_item->setY(y);
    selected_connection_item->setZValue(-1);
    selected_connection_item->blockSignals(false);

    setConnectionEditControlValues(selected_connection_item->connection);
}

void Editor::updateCurrentConnectionDirection(QString curDirection) {
    if (!selected_connection_item)
        return;

    QString originalDirection = selected_connection_item->connection->direction;
    setCurrentConnectionDirection(curDirection);
    updateMirroredConnectionDirection(selected_connection_item->connection, originalDirection);
}

void Editor::onConnectionMoved(MapConnection* connection) {
    updateMirroredConnectionOffset(connection);
    onConnectionOffsetChanged(connection->offset.toInt());
}

void Editor::onConnectionOffsetChanged(int newOffset) {
    ui->spinBox_ConnectionOffset->blockSignals(true);
    ui->spinBox_ConnectionOffset->setValue(newOffset);
    ui->spinBox_ConnectionOffset->blockSignals(false);

}

void Editor::setConnectionEditControlValues(MapConnection* connection) {
    QString mapName = connection ? connection->map_name : "";
    QString direction = connection ? connection->direction : "";
    int offset = connection ? connection->offset.toInt() : 0;

    ui->comboBox_ConnectedMap->blockSignals(true);
    ui->comboBox_ConnectionDirection->blockSignals(true);
    ui->spinBox_ConnectionOffset->blockSignals(true);

    ui->comboBox_ConnectedMap->setCurrentText(mapName);
    ui->comboBox_ConnectionDirection->setCurrentText(direction);
    ui->spinBox_ConnectionOffset->setValue(offset);

    ui->comboBox_ConnectedMap->blockSignals(false);
    ui->comboBox_ConnectionDirection->blockSignals(false);
    ui->spinBox_ConnectionOffset->blockSignals(false);
}

void Editor::setConnectionEditControlsEnabled(bool enabled) {
    ui->comboBox_ConnectionDirection->setEnabled(enabled);
    ui->comboBox_ConnectedMap->setEnabled(enabled);
    ui->spinBox_ConnectionOffset->setEnabled(enabled);

    if (!enabled) {
        setConnectionEditControlValues(nullptr);
    }
}

void Editor::onConnectionItemSelected(ConnectionPixmapItem* connectionItem) {
    if (!connectionItem)
        return;

    for (ConnectionPixmapItem* item : connection_edit_items) {
        bool isSelectedItem = item == connectionItem;
        int zValue = isSelectedItem ? 0 : -1;
        qreal opacity = isSelectedItem ? 1 : 0.75;
        item->setZValue(zValue);
        item->render(opacity);
        if (isSelectedItem) {
            QPixmap pixmap = item->pixmap();
            QPainter painter(&pixmap);
            painter.setPen(QColor(255, 0, 255));
            painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
            painter.end();
            item->setPixmap(pixmap);
        }
    }
    selected_connection_item = connectionItem;
    setConnectionEditControlsEnabled(true);
    setConnectionEditControlValues(selected_connection_item->connection);
    ui->spinBox_ConnectionOffset->setMaximum(selected_connection_item->getMaxOffset());
    ui->spinBox_ConnectionOffset->setMinimum(selected_connection_item->getMinOffset());
    onConnectionOffsetChanged(selected_connection_item->connection->offset.toInt());
}

void Editor::setSelectedConnectionFromMap(QString mapName) {
    // Search for the first connection that connects to the given map map.
    for (ConnectionPixmapItem* item : connection_edit_items) {
        if (item->connection->map_name == mapName) {
            onConnectionItemSelected(item);
            break;
        }
    }
}

void Editor::onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem) {
    emit loadMapRequested(connectionItem->connection->map_name, map->name);
}

void Editor::onConnectionDirectionChanged(QString newDirection) {
    ui->comboBox_ConnectionDirection->blockSignals(true);
    ui->comboBox_ConnectionDirection->setCurrentText(newDirection);
    ui->comboBox_ConnectionDirection->blockSignals(false);
}

void Editor::onBorderMetatilesChanged() {
    displayMapBorder();
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
}

void Editor::onHoveredMovementPermissionChanged(uint16_t collision, uint16_t elevation) {
    this->ui->statusBar->showMessage(this->getMovementPermissionText(collision, elevation));
}

void Editor::onHoveredMovementPermissionCleared() {
    this->ui->statusBar->clearMessage();
}

void Editor::onHoveredMetatileSelectionChanged(uint16_t metatileId) {
    QString message = QString("Metatile: 0x%1")
                        .arg(QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper());
    this->ui->statusBar->showMessage(message);
}

void Editor::onHoveredMetatileSelectionCleared() {
    this->ui->statusBar->clearMessage();
}

void Editor::onSelectedMetatilesChanged() {
    this->redrawCurrentMetatilesSelection();
}

void Editor::onHoveredMapMetatileChanged(int x, int y) {
    if (x >= 0 && x < map->getWidth() && y >= 0 && y < map->getHeight()) {
        int blockIndex = y * map->getWidth() + x;
        int tile = map->layout->blockdata->blocks->at(blockIndex).tile;
        this->ui->statusBar->showMessage(QString("X: %1, Y: %2, Metatile: 0x%3, Scale = %4x")
                              .arg(x)
                              .arg(y)
                              .arg(QString("%1").arg(tile, 3, 16, QChar('0')).toUpper())
                              .arg(QString::number(pow(scale_base, scale_exp), 'g', 2)));
    }
}

void Editor::onHoveredMapMetatileCleared() {
    this->ui->statusBar->clearMessage();
}

void Editor::onHoveredMapMovementPermissionChanged(int x, int y) {
    if (x >= 0 && x < map->getWidth() && y >= 0 && y < map->getHeight()) {
        int blockIndex = y * map->getWidth() + x;
        uint16_t collision = map->layout->blockdata->blocks->at(blockIndex).collision;
        uint16_t elevation = map->layout->blockdata->blocks->at(blockIndex).elevation;
        QString message = QString("X: %1, Y: %2, %3")
                            .arg(x)
                            .arg(y)
                            .arg(this->getMovementPermissionText(collision, elevation));
        this->ui->statusBar->showMessage(message);
    }
}

void Editor::onHoveredMapMovementPermissionCleared() {
    this->ui->statusBar->clearMessage();
}

QString Editor::getMovementPermissionText(uint16_t collision, uint16_t elevation){
    QString message;
    if (collision == 0 && elevation == 0) {
        message = "Collision: Transition between elevations";
    } else if (collision == 0 && elevation == 15) {
        message = "Collision: Multi-Level (Bridge)";
    } else if (collision == 0 && elevation == 1) {
        message = "Collision: Surf";
    } else if (collision == 0) {
        message = QString("Collision: Passable, Elevation: %1").arg(elevation);
    } else {
        message = QString("Collision: Impassable, Elevation: %1").arg(elevation);
    }
    return message;
}

void Editor::setConnectionsVisibility(bool visible) {
    for (QGraphicsPixmapItem* item : connection_items) {
        item->setVisible(visible);
        item->setActive(visible);
    }
}

void Editor::setMap(QString map_name) {
    if (map_name.isNull()) {
        return;
    }
    if (project) {
        map = project->loadMap(map_name);
        selected_events->clear();
        displayMap();
        updateSelectedEvents();
    }
}

void Editor::mouseEvent_map(QGraphicsSceneMouseEvent *event, MapPixmapItem *item) {
    if (map_edit_mode == "paint") {
        if (event->buttons() & Qt::RightButton) {
            item->updateMetatileSelection(event);
        } else if (event->buttons() & Qt::MiddleButton) {
            item->floodFill(event);
        } else {
            item->paint(event);
        }
    } else if (map_edit_mode == "select") {
        item->select(event);
    } else if (map_edit_mode == "fill") {
        if (event->buttons() & Qt::RightButton) {
            item->updateMetatileSelection(event);
        } else {
            item->floodFill(event);
        }
    } else if (map_edit_mode == "pick") {

        if (event->buttons() & Qt::RightButton) {
            item->updateMetatileSelection(event);
        } else {
            item->pick(event);
        }
    } else if (map_edit_mode == "shift") {
        item->shift(event);
    }
}
void Editor::mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item) {
    if (map_edit_mode == "paint") {
        if (event->buttons() & Qt::RightButton) {
            item->updateMovementPermissionSelection(event);
        } else if (event->buttons() & Qt::MiddleButton) {
            item->floodFill(event);
        } else {
            item->paint(event);
        }
    } else if (map_edit_mode == "select") {
        item->select(event);
    } else if (map_edit_mode == "fill") {
        if (event->buttons() & Qt::RightButton) {
            item->pick(event);
        } else {
            item->floodFill(event);
        }
    } else if (map_edit_mode == "pick") {
        item->pick(event);
    } else if (map_edit_mode == "shift") {
        item->shift(event);
    }
}

void Editor::displayMap() {
    if (!scene) {
        scene = new QGraphicsScene;
        MapSceneEventFilter *filter = new MapSceneEventFilter();
        scene->installEventFilter(filter);
        connect(filter, &MapSceneEventFilter::wheelZoom, this, &Editor::wheelZoom);
    }

    if (map_item && scene) {
        scene->removeItem(map_item);
        delete map_item;
    }

    displayMetatileSelector();
    displayTilemapTileSelector();
    //displayCityMapMetatileSelector();
    displayMovementPermissionSelector();
    displayMapMetatiles();
    displayMapMovementPermissions();
    displayBorderMetatiles();
    displayCurrentMetatilesSelection();
    displayMapEvents();
    displayMapConnections();
    displayMapBorder();
    displayMapGrid();

    if (map_item) {
        map_item->setVisible(false);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
}

void Editor::displayMetatileSelector() {
    if (metatile_selector_item && metatile_selector_item->scene()) {
        metatile_selector_item->scene()->removeItem(metatile_selector_item);
        delete scene_metatiles;
    }

    scene_metatiles = new QGraphicsScene;
    if (!metatile_selector_item) {
        metatile_selector_item = new MetatileSelector(8, map->layout->tileset_primary, map->layout->tileset_secondary);
        connect(metatile_selector_item, SIGNAL(hoveredMetatileSelectionChanged(uint16_t)),
                this, SLOT(onHoveredMetatileSelectionChanged(uint16_t)));
        connect(metatile_selector_item, SIGNAL(hoveredMetatileSelectionCleared()),
                this, SLOT(onHoveredMetatileSelectionCleared()));
        connect(metatile_selector_item, SIGNAL(selectedMetatilesChanged()),
                this, SLOT(onSelectedMetatilesChanged()));
        metatile_selector_item->select(0);
    } else {
        metatile_selector_item->setTilesets(map->layout->tileset_primary, map->layout->tileset_secondary);
    }

    scene_metatiles->addItem(metatile_selector_item);
}

void Editor::displayTilemapTileSelector() {
    if (region_map_tile_selector_item && region_map_tile_selector_item->scene()) {
        region_map_tile_selector_item->scene()->removeItem(region_map_tile_selector_item);
        delete scene_region_map_img_tiles;
    }

    scene_region_map_img_tiles = new QGraphicsScene;
    if (!region_map_tile_selector_item) {
        region_map_tile_selector_item = new TilemapTileSelector(8, map->layout->tileset_primary);
        connect(region_map_tile_selector_item, SIGNAL(hoveredMetatileSelectionChanged(uint16_t)),
                this, SLOT(onHoveredMetatileSelectionChanged(uint16_t)));
        connect(region_map_tile_selector_item, SIGNAL(hoveredMetatileSelectionCleared()),
                this, SLOT(onHoveredMetatileSelectionCleared()));
        connect(region_map_tile_selector_item, SIGNAL(selectedMetatilesChanged()),
                this, SLOT(onSelectedMetatilesChanged()));
        region_map_tile_selector_item->select(0);
    } else {
        region_map_tile_selector_item->setTileset(map->layout->tileset_primary);//, map->layout->tileset_secondary);
    }

    scene_region_map_img_tiles->addItem(region_map_tile_selector_item);
}
/*
void Editor::displayCityMapMetatileSelector() {
    //
    if (city_map_metatile_selector_item && city_map_metatile_selector_item->scene()) {
        city_map_metatile_selector_item->scene()->removeItem(city_map_metatile_selector_item);
        delete scene_city_map_metatiles;
    }

    scene_city_map_metatiles = new QGraphicsScene;

    if (!city_map_metatile_selector_item) {
        //city_map_metatile_selector_item = new MetatileSelector(8, tileset_city_map, nullptr);
        city_map_metatile_selector_item = new TilemapTileSelector(8, map->layout->tileset_primary);
        connect(city_map_metatile_selector_item, SIGNAL(hoveredMetatileSelectionChanged(uint16_t)),
                this, SLOT(onHoveredMetatileSelectionChanged(uint16_t)));
        connect(city_map_metatile_selector_item, SIGNAL(hoveredMetatileSelectionCleared()),
                this, SLOT(onHoveredMetatileSelectionCleared()));
        connect(city_map_metatile_selector_item, SIGNAL(selectedMetatilesChanged()),
                this, SLOT(onSelectedMetatilesChanged()));
        //city_map_metatile_selector_item->select(0);
    } else {
        city_map_metatile_selector_item->setTileset(tileset_city_map);
        // wont take nullptr as second item without seg faulting
        //city_map_metatile_selector_item->setTileset(map->layout->tileset_primary);//, map->layout->tileset_secondary);
    }

    scene_city_map_metatiles->addItem(city_map_metatile_selector_item);
}
*/

void Editor::displayMapMetatiles() {
    map_item = new MapPixmapItem(map, this->metatile_selector_item, this->settings);
    connect(map_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,MapPixmapItem*)),
            this, SLOT(mouseEvent_map(QGraphicsSceneMouseEvent*,MapPixmapItem*)));
    connect(map_item, SIGNAL(hoveredMapMetatileChanged(int, int)),
            this, SLOT(onHoveredMapMetatileChanged(int, int)));
    connect(map_item, SIGNAL(hoveredMapMetatileCleared()),
            this, SLOT(onHoveredMapMetatileCleared()));

    map_item->draw(true);
    scene->addItem(map_item);

    int tw = 16;
    int th = 16;
    scene->setSceneRect(
        -6 * tw,
        -6 * th,
        map_item->pixmap().width() + 12 * tw,
        map_item->pixmap().height() + 12 * th
    );
}

void Editor::displayMapMovementPermissions() {
    if (collision_item && scene) {
        scene->removeItem(collision_item);
        delete collision_item;
    }
    collision_item = new CollisionPixmapItem(map, this->movement_permissions_selector_item, this->metatile_selector_item, this->settings);
    connect(collision_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)),
            this, SLOT(mouseEvent_collision(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)));
    connect(collision_item, SIGNAL(hoveredMapMovementPermissionChanged(int, int)),
            this, SLOT(onHoveredMapMovementPermissionChanged(int, int)));
    connect(collision_item, SIGNAL(hoveredMapMovementPermissionCleared()),
            this, SLOT(onHoveredMapMovementPermissionCleared()));

    collision_item->draw(true);
    scene->addItem(collision_item);
}

void Editor::displayBorderMetatiles() {
    if (selected_border_metatiles_item && selected_border_metatiles_item->scene()) {
        selected_border_metatiles_item->scene()->removeItem(selected_border_metatiles_item);
        delete selected_border_metatiles_item;
    }

    scene_selected_border_metatiles = new QGraphicsScene;
    selected_border_metatiles_item = new BorderMetatilesPixmapItem(map, this->metatile_selector_item);
    selected_border_metatiles_item->draw();
    scene_selected_border_metatiles->addItem(selected_border_metatiles_item);

    connect(selected_border_metatiles_item, SIGNAL(borderMetatilesChanged()), this, SLOT(onBorderMetatilesChanged()));
}

void Editor::displayCurrentMetatilesSelection() {
    if (scene_current_metatile_selection_item && scene_current_metatile_selection_item->scene()) {
        scene_current_metatile_selection_item->scene()->removeItem(scene_current_metatile_selection_item);
        delete scene_current_metatile_selection_item;
    }

    scene_current_metatile_selection = new QGraphicsScene;
    scene_current_metatile_selection_item = new CurrentSelectedMetatilesPixmapItem(map, this->metatile_selector_item);
    scene_current_metatile_selection_item->draw();
    scene_current_metatile_selection->addItem(scene_current_metatile_selection_item);
}

void Editor::redrawCurrentMetatilesSelection() {
    if (scene_current_metatile_selection_item) {
        scene_current_metatile_selection_item->draw();
        emit currentMetatilesSelectionChanged();
    }
}

void Editor::displayMovementPermissionSelector() {
    if (movement_permissions_selector_item && movement_permissions_selector_item->scene()) {
        movement_permissions_selector_item->scene()->removeItem(movement_permissions_selector_item);
        delete scene_collision_metatiles;
    }

    scene_collision_metatiles = new QGraphicsScene;
    if (!movement_permissions_selector_item) {
        movement_permissions_selector_item = new MovementPermissionsSelector();
        connect(movement_permissions_selector_item, SIGNAL(hoveredMovementPermissionChanged(uint16_t, uint16_t)),
                this, SLOT(onHoveredMovementPermissionChanged(uint16_t, uint16_t)));
        connect(movement_permissions_selector_item, SIGNAL(hoveredMovementPermissionCleared()),
                this, SLOT(onHoveredMovementPermissionCleared()));
        movement_permissions_selector_item->select(0, 3);
    }

    scene_collision_metatiles->addItem(movement_permissions_selector_item);
}

void Editor::displayMapEvents() {
    if (events_group) {
        for (QGraphicsItem *child : events_group->childItems()) {
            events_group->removeFromGroup(child);
            delete child;
        }

        if (events_group->scene()) {
            events_group->scene()->removeItem(events_group);
        }

        delete events_group;
    }

    selected_events->clear();

    events_group = new QGraphicsItemGroup;
    scene->addItem(events_group);

    QList<Event *> events = map->getAllEvents();
    project->loadEventPixmaps(events);
    for (Event *event : events) {
        addMapEvent(event);
    }
    //objects_group->setFiltersChildEvents(false);
    events_group->setHandlesChildEvents(false);

    emit objectsChanged();
}

DraggablePixmapItem *Editor::addMapEvent(Event *event) {
    DraggablePixmapItem *object = new DraggablePixmapItem(event, this);
    events_group->addToGroup(object);
    return object;
}

void Editor::displayMapConnections() {
    for (QGraphicsPixmapItem* item : connection_items) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    connection_items.clear();

    for (ConnectionPixmapItem* item : connection_edit_items) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    selected_connection_item = nullptr;
    connection_edit_items.clear();

    for (MapConnection *connection : map->connections) {
        if (connection->direction == "dive" || connection->direction == "emerge") {
            continue;
        }
        createConnectionItem(connection, false);
    }

    if (!connection_edit_items.empty()) {
        onConnectionItemSelected(connection_edit_items.first());
    }
}

void Editor::createConnectionItem(MapConnection* connection, bool hide) {
    Map *connected_map = project->getMap(connection->map_name);
    QPixmap pixmap = connected_map->renderConnection(*connection);
    int offset = connection->offset.toInt(nullptr, 0);
    int x = 0, y = 0;
    if (connection->direction == "up") {
        x = offset * 16;
        y = -pixmap.height();
    } else if (connection->direction == "down") {
        x = offset * 16;
        y = map->getHeight() * 16;
    } else if (connection->direction == "left") {
        x = -pixmap.width();
        y = offset * 16;
    } else if (connection->direction == "right") {
        x = map->getWidth() * 16;
        y = offset * 16;
    }

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    item->setZValue(-1);
    item->setX(x);
    item->setY(y);
    scene->addItem(item);
    connection_items.append(item);
    item->setVisible(!hide);

    ConnectionPixmapItem *connection_edit_item = new ConnectionPixmapItem(pixmap, connection, x, y, map->getWidth(), map->getHeight());
    connection_edit_item->setX(x);
    connection_edit_item->setY(y);
    connection_edit_item->setZValue(-1);
    scene->addItem(connection_edit_item);
    connect(connection_edit_item, SIGNAL(connectionMoved(MapConnection*)), this, SLOT(onConnectionMoved(MapConnection*)));
    connect(connection_edit_item, SIGNAL(connectionItemSelected(ConnectionPixmapItem*)), this, SLOT(onConnectionItemSelected(ConnectionPixmapItem*)));
    connect(connection_edit_item, SIGNAL(connectionItemDoubleClicked(ConnectionPixmapItem*)), this, SLOT(onConnectionItemDoubleClicked(ConnectionPixmapItem*)));
    connection_edit_items.append(connection_edit_item);
}

void Editor::displayMapBorder() {
    for (QGraphicsPixmapItem* item : borderItems) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    borderItems.clear();

    QPixmap pixmap = map->renderBorder();
    for (int y = -6; y < map->getHeight() + 6; y += 2)
    for (int x = -6; x < map->getWidth() + 6; x += 2) {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
        item->setX(x * 16);
        item->setY(y * 16);
        item->setZValue(-2);
        scene->addItem(item);
        borderItems.append(item);
    }
}

void Editor::displayMapGrid() {
    for (QGraphicsLineItem* item : gridLines) {
        if (item && item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    gridLines.clear();
    ui->checkBox_ToggleGrid->disconnect();

    int pixelWidth = map->getWidth() * 16;
    int pixelHeight = map->getHeight() * 16;
    for (int i = 0; i <= map->getWidth(); i++) {
        int x = i * 16;
        QGraphicsLineItem *line = scene->addLine(x, 0, x, pixelHeight);
        line->setVisible(ui->checkBox_ToggleGrid->isChecked());
        gridLines.append(line);
        connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
    for (int j = 0; j <= map->getHeight(); j++) {
        int y = j * 16;
        QGraphicsLineItem *line = scene->addLine(0, y, pixelWidth, y);
        line->setVisible(ui->checkBox_ToggleGrid->isChecked());
        gridLines.append(line);
        connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
}

void Editor::updateConnectionOffset(int offset) {
    if (!selected_connection_item)
        return;

    selected_connection_item->blockSignals(true);
    offset = qMin(offset, selected_connection_item->getMaxOffset());
    offset = qMax(offset, selected_connection_item->getMinOffset());
    selected_connection_item->connection->offset = QString::number(offset);
    if (selected_connection_item->connection->direction == "up" || selected_connection_item->connection->direction == "down") {
        selected_connection_item->setX(selected_connection_item->initialX + (offset - selected_connection_item->initialOffset) * 16);
    } else if (selected_connection_item->connection->direction == "left" || selected_connection_item->connection->direction == "right") {
        selected_connection_item->setY(selected_connection_item->initialY + (offset - selected_connection_item->initialOffset) * 16);
    }
    selected_connection_item->blockSignals(false);
    updateMirroredConnectionOffset(selected_connection_item->connection);
}

void Editor::setConnectionMap(QString mapName) {
    if (!mapName.isEmpty() && !project->mapNames->contains(mapName)) {
        qDebug() << "Invalid map name " << mapName << " specified for connection.";
        return;
    }
    if (!selected_connection_item)
        return;

    if (mapName.isEmpty()) {
        removeCurrentConnection();
        return;
    }

    QString originalMapName = selected_connection_item->connection->map_name;
    setConnectionEditControlsEnabled(true);
    selected_connection_item->connection->map_name = mapName;
    setCurrentConnectionDirection(selected_connection_item->connection->direction);
    updateMirroredConnectionMap(selected_connection_item->connection, originalMapName);
}

void Editor::addNewConnection() {
    // Find direction with least number of connections.
    QMap<QString, int> directionCounts = QMap<QString, int>({{"up", 0}, {"right", 0}, {"down", 0}, {"left", 0}});
    for (MapConnection* connection : map->connections) {
        directionCounts[connection->direction]++;
    }
    QString minDirection = "up";
    int minCount = INT_MAX;
    for (QString direction : directionCounts.keys()) {
        if (directionCounts[direction] < minCount) {
            minDirection = direction;
            minCount = directionCounts[direction];
        }
    }

    // Don't connect the map to itself.
    QString defaultMapName = project->mapNames->first();
    if (defaultMapName == map->name) {
        defaultMapName = project->mapNames->value(1);
    }

    MapConnection* newConnection = new MapConnection;
    newConnection->direction = minDirection;
    newConnection->offset = "0";
    newConnection->map_name = defaultMapName;
    map->connections.append(newConnection);
    createConnectionItem(newConnection, true);
    onConnectionItemSelected(connection_edit_items.last());
    ui->label_NumConnections->setText(QString::number(map->connections.length()));

    updateMirroredConnection(newConnection, newConnection->direction, newConnection->map_name);
}

void Editor::updateMirroredConnectionOffset(MapConnection* connection) {
    updateMirroredConnection(connection, connection->direction, connection->map_name);
}
void Editor::updateMirroredConnectionDirection(MapConnection* connection, QString originalDirection) {
    updateMirroredConnection(connection, originalDirection, connection->map_name);
}
void Editor::updateMirroredConnectionMap(MapConnection* connection, QString originalMapName) {
    updateMirroredConnection(connection, connection->direction, originalMapName);
}
void Editor::removeMirroredConnection(MapConnection* connection) {
    updateMirroredConnection(connection, connection->direction, connection->map_name, true);
}
void Editor::updateMirroredConnection(MapConnection* connection, QString originalDirection, QString originalMapName, bool isDelete) {
    if (!ui->checkBox_MirrorConnections->isChecked())
        return;

    static QMap<QString, QString> oppositeDirections = QMap<QString, QString>({
        {"up", "down"}, {"right", "left"},
        {"down", "up"}, {"left", "right"},
        {"dive", "emerge"},{"emerge", "dive"}});
    QString oppositeDirection = oppositeDirections.value(originalDirection);

    // Find the matching connection in the connected map.
    MapConnection* mirrorConnection = nullptr;
    Map* otherMap = project->getMap(originalMapName);
    for (MapConnection* conn : otherMap->connections) {
        if (conn->direction == oppositeDirection && conn->map_name == map->name) {
            mirrorConnection = conn;
        }
    }

    if (isDelete) {
        if (mirrorConnection) {
            otherMap->connections.removeOne(mirrorConnection);
            delete mirrorConnection;
        }
        return;
    }

    if (connection->direction != originalDirection || connection->map_name != originalMapName) {
        if (mirrorConnection) {
            otherMap->connections.removeOne(mirrorConnection);
            delete mirrorConnection;
            mirrorConnection = nullptr;
            otherMap = project->getMap(connection->map_name);
        }
    }

    // Create a new mirrored connection, if a matching one doesn't already exist.
    if (!mirrorConnection) {
        mirrorConnection = new MapConnection;
        mirrorConnection->direction = oppositeDirections.value(connection->direction);
        mirrorConnection->map_name = map->name;
        otherMap->connections.append(mirrorConnection);
    }

    mirrorConnection->offset = QString::number(-connection->offset.toInt());
}

void Editor::removeCurrentConnection() {
    if (!selected_connection_item)
        return;

    map->connections.removeOne(selected_connection_item->connection);
    connection_edit_items.removeOne(selected_connection_item);
    removeMirroredConnection(selected_connection_item->connection);

    if (selected_connection_item && selected_connection_item->scene()) {
        selected_connection_item->scene()->removeItem(selected_connection_item);
        delete selected_connection_item;
    }

    selected_connection_item = nullptr;
    setConnectionEditControlsEnabled(false);
    ui->spinBox_ConnectionOffset->setValue(0);
    ui->label_NumConnections->setText(QString::number(map->connections.length()));

    if (connection_edit_items.length() > 0) {
        onConnectionItemSelected(connection_edit_items.last());
    }
}

void Editor::updateDiveMap(QString mapName) {
    updateDiveEmergeMap(mapName, "dive");
}

void Editor::updateEmergeMap(QString mapName) {
    updateDiveEmergeMap(mapName, "emerge");
}

void Editor::updateDiveEmergeMap(QString mapName, QString direction) {
    if (!mapName.isEmpty() && !project->mapNamesToMapConstants->contains(mapName)) {
        qDebug() << "Invalid " << direction << " map connection: " << mapName;
        return;
    }

    MapConnection* connection = nullptr;
    for (MapConnection* conn : map->connections) {
        if (conn->direction == direction) {
            connection = conn;
            break;
        }
    }

    if (mapName.isEmpty()) {
        // Remove dive/emerge connection
        if (connection) {
            map->connections.removeOne(connection);
            removeMirroredConnection(connection);
        }
    } else {
        if (!connection) {
            connection = new MapConnection;
            connection->direction = direction;
            connection->offset = "0";
            connection->map_name = mapName;
            map->connections.append(connection);
            updateMirroredConnection(connection, connection->direction, connection->map_name);
        } else {
            QString originalMapName = connection->map_name;
            connection->map_name = mapName;
            updateMirroredConnectionMap(connection, originalMapName);
        }
    }

    ui->label_NumConnections->setText(QString::number(map->connections.length()));
}

void Editor::updatePrimaryTileset(QString tilesetLabel, bool forceLoad)
{
    if (map->layout->tileset_primary_label != tilesetLabel || forceLoad)
    {
        map->layout->tileset_primary_label = tilesetLabel;
        map->layout->tileset_primary = project->getTileset(tilesetLabel, forceLoad);
        emit tilesetChanged(map->name);
    }
}

void Editor::updateSecondaryTileset(QString tilesetLabel, bool forceLoad)
{
    if (map->layout->tileset_secondary_label != tilesetLabel || forceLoad)
    {
        map->layout->tileset_secondary_label = tilesetLabel;
        map->layout->tileset_secondary = project->getTileset(tilesetLabel, forceLoad);
        emit tilesetChanged(map->name);
    }
}

void Editor::toggleBorderVisibility(bool visible)
{
    this->setBorderItemsVisible(visible);
    this->setConnectionsVisibility(visible);
}

Tileset* Editor::getCurrentMapPrimaryTileset()
{
    QString tilesetLabel = map->layout->tileset_primary_label;
    return project->getTileset(tilesetLabel);
}

void DraggablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouse) {
    active = true;
    last_x = static_cast<int>(mouse->pos().x() + this->pos().x()) / 16;
    last_y = static_cast<int>(mouse->pos().y() + this->pos().y()) / 16;
    this->editor->selectMapEvent(this, mouse->modifiers() & Qt::ControlModifier);
    this->editor->updateSelectedEvents();
    selectingEvent = true;
    //qDebug() << QString("(%1, %2)").arg(event->get("x")).arg(event->get("y"));
}

void DraggablePixmapItem::move(int x, int y) {
    event->setX(event->x() + x);
    event->setY(event->y() + y);
    updatePosition();
    emitPositionChanged();
}

void DraggablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouse) {
    if (active) {
        int x = static_cast<int>(mouse->pos().x() + this->pos().x()) / 16;
        int y = static_cast<int>(mouse->pos().y() + this->pos().y()) / 16;
        if (x != last_x || y != last_y) {
            if (editor->selected_events->contains(this)) {
                for (DraggablePixmapItem *item : *editor->selected_events) {
                    item->move(x - last_x, y - last_y);
                }
            } else {
                move(x - last_x, y - last_y);
            }
            last_x = x;
            last_y = y;
            //qDebug() << QString("(%1, %2)").arg(event->get("x")).arg(event->get("x"));
        }
    }
}

void DraggablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    active = false;
}

void DraggablePixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    if (this->event->get("event_type") == EventType::Warp) {
        QString destMap = this->event->get("destination_map_name");
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(this->event->get("destination_map_name"), this->event->get("destination_warp"));
        }
    }
}

QList<DraggablePixmapItem *> *Editor::getObjects() {
    QList<DraggablePixmapItem *> *list = new QList<DraggablePixmapItem *>;
    for (Event *event : map->getAllEvents()) {
        for (QGraphicsItem *child : events_group->childItems()) {
            DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
            if (item->event == event) {
                list->append(item);
                break;
            }
        }
    }
    return list;
}

void Editor::redrawObject(DraggablePixmapItem *item) {
    if (item) {
        item->setPixmap(item->event->pixmap);
        item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
        if (selected_events && selected_events->contains(item)) {
            QImage image = item->pixmap().toImage();
            QPainter painter(&image);
            painter.setPen(QColor(250, 0, 255));
            painter.drawRect(0, 0, image.width() - 1, image.height() - 1);
            painter.end();
            item->setPixmap(QPixmap::fromImage(image));
        }
    }
}

void Editor::updateSelectedEvents() {
    for (DraggablePixmapItem *item : *(getObjects())) {
        redrawObject(item);
    }
    emit selectedObjectsChanged();
}

void Editor::selectMapEvent(DraggablePixmapItem *object) {
    selectMapEvent(object, false);
}

void Editor::selectMapEvent(DraggablePixmapItem *object, bool toggle) {
    if (selected_events && object) {
        if (selected_events->contains(object)) {
            if (toggle) {
                selected_events->removeOne(object);
            }
        } else {
            if (!toggle) {
                selected_events->clear();
            }
            selected_events->append(object);
        }
        updateSelectedEvents();
    }
}

DraggablePixmapItem* Editor::addNewEvent(QString event_type) {
    if (project && map) {
        Event *event = Event::createNewEvent(event_type, map->name);
        event->put("map_name", map->name);
        map->addEvent(event);
        project->loadEventPixmaps(map->getAllEvents());
        DraggablePixmapItem *object = addMapEvent(event);
        return object;
    }
    return nullptr;
}

void Editor::deleteEvent(Event *event) {
    Map *map = project->getMap(event->get("map_name"));
    if (map) {
        map->removeEvent(event);
    }
    //selected_events->removeAll(event);
    //updateSelectedObjects();
}

// It doesn't seem to be possible to prevent the mousePress event
// from triggering both event's DraggablePixmapItem and the background mousePress.
// Since the DraggablePixmapItem's event fires first, we can set a temp
// variable "selectingEvent" so that we can detect whether or not the user
// is clicking on the background instead of an event.
void Editor::objectsView_onMousePress(QMouseEvent *event) {
    bool multiSelect = event->modifiers() & Qt::ControlModifier;
    if (!selectingEvent && !multiSelect && selected_events->length() > 1) {
        DraggablePixmapItem *first = selected_events->first();
        selected_events->clear();
        selected_events->append(first);
        updateSelectedEvents();
    }

    selectingEvent = false;
}
