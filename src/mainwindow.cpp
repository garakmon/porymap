#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "project.h"
#include "editor.h"
#include "eventpropertiesframe.h"
#include "ui_eventpropertiesframe.h"
#include "bordermetatilespixmapitem.h"
#include "currentselectedmetatilespixmapitem.h"

#include <QDebug>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QShortcut>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>
#include <QSpacerItem>
#include <QFont>
#include <QScrollBar>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QScroller>
#include <math.h>
#include <QProcess>
#include <QSysInfo>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("porymap");
    QApplication::setApplicationDisplayName("porymap");
    QApplication::setWindowIcon(QIcon(":/icons/porymap-icon-1.ico"));

    ui->setupUi(this);
    this->initCustomUI();
    this->initExtraSignals();
    this->initExtraShortcuts();
    this->initEditor();
    this->initMiscHeapObjects();
    this->initMapSortOrder();
    this->openRecentProject();

    on_toolButton_Paint_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initExtraShortcuts() {
    new QShortcut(QKeySequence("Ctrl+Shift+Z"), this, SLOT(redo()));
    new QShortcut(QKeySequence("Ctrl+0"), this, SLOT(resetMapViewScale()));
    ui->actionZoom_In->setShortcuts({QKeySequence("Ctrl++"), QKeySequence("Ctrl+=")});
}

void MainWindow::initCustomUI() {
    // Right-clicking on items in the map list tree view brings up a context menu.
    ui->mapList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapList, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(onOpenMapListContextMenu(const QPoint &)));
}

void MainWindow::initExtraSignals() {
    connect(ui->newEventToolButton, SIGNAL(newEventAdded(QString)), this, SLOT(addNewEvent(QString)));
}

void MainWindow::initEditor() {
    this->editor = new Editor(ui);
    connect(this->editor, SIGNAL(objectsChanged()), this, SLOT(updateSelectedObjects()));
    connect(this->editor, SIGNAL(selectedObjectsChanged()), this, SLOT(updateSelectedObjects()));
    connect(this->editor, SIGNAL(loadMapRequested(QString, QString)), this, SLOT(onLoadMapRequested(QString, QString)));
    connect(this->editor, SIGNAL(tilesetChanged(QString)), this, SLOT(onTilesetChanged(QString)));
    connect(this->editor, SIGNAL(warpEventDoubleClicked(QString,QString)), this, SLOT(openWarpMap(QString,QString)));
    connect(this->editor, SIGNAL(currentMetatilesSelectionChanged()), this, SLOT(currentMetatilesSelectionChanged()));
    connect(this->editor, &Editor::wheelZoom, this, &MainWindow::scaleMapView);

    this->loadUserSettings();
}

void MainWindow::initMiscHeapObjects() {
    mapIcon = new QIcon;
    mapIcon->addFile(QStringLiteral(":/icons/map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapIcon->addFile(QStringLiteral(":/icons/map_opened.ico"), QSize(), QIcon::Normal, QIcon::On);

    mapEditedIcon = new QIcon;
    mapEditedIcon->addFile(QStringLiteral(":/icons/map_edited.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapEditedIcon->addFile(QStringLiteral(":/icons/map_opened.ico"), QSize(), QIcon::Normal , QIcon::On);

    mapListModel = new QStandardItemModel;
    mapGroupItemsList = new QList<QStandardItem*>;
    mapListProxyModel = new FilterChildrenProxyModel;

    mapListProxyModel->setSourceModel(mapListModel);
    ui->mapList->setModel(mapListProxyModel);
}

void MainWindow::initMapSortOrder() {
    QMenu *mapSortOrderMenu = new QMenu();
    QActionGroup *mapSortOrderActionGroup = new QActionGroup(ui->toolButton_MapSortOrder);

    mapSortOrderMenu->addAction(ui->actionSort_by_Group);
    mapSortOrderMenu->addAction(ui->actionSort_by_Area);
    mapSortOrderMenu->addAction(ui->actionSort_by_Layout);
    ui->toolButton_MapSortOrder->setMenu(mapSortOrderMenu);

    mapSortOrderActionGroup->addAction(ui->actionSort_by_Group);
    mapSortOrderActionGroup->addAction(ui->actionSort_by_Area);
    mapSortOrderActionGroup->addAction(ui->actionSort_by_Layout);

    connect(ui->toolButton_MapSortOrder, &QToolButton::triggered, this, &MainWindow::mapSortOrder_changed);

    QAction* sortOrder = ui->toolButton_MapSortOrder->menu()->actions()[mapSortOrder];
    ui->toolButton_MapSortOrder->setIcon(sortOrder->icon());
    sortOrder->setChecked(true);
}

void MainWindow::mapSortOrder_changed(QAction *action)
{
    QSettings settings;

    QList<QAction*> items = ui->toolButton_MapSortOrder->menu()->actions();
    int i = 0;
    for (; i < items.count(); i++)
    {
        if (items[i] == action)
        {
            break;
        }
    }

    if (i != mapSortOrder)
    {
        ui->toolButton_MapSortOrder->setIcon(action->icon());
        mapSortOrder = static_cast<MapSortOrder>(i);
        settings.setValue("map_sort_order", i);

        if (isProjectOpen())
        {
            sortMapList();
        }
    }
}

void MainWindow::on_lineEdit_filterBox_textChanged(const QString &arg1)
{
    mapListProxyModel->setFilterRegExp(QRegExp(arg1, Qt::CaseInsensitive, QRegExp::FixedString));
    ui->mapList->expandToDepth(0);
    ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(editor->map->name)), true);
}

void MainWindow::loadUserSettings() {
    QSettings settings;

    bool betterCursors = settings.contains("cursor_mode") && settings.value("cursor_mode") != "0";
    ui->actionBetter_Cursors->setChecked(betterCursors);
    this->editor->settings->betterCursors = betterCursors;

    if (!settings.contains("map_sort_order"))
    {
        settings.setValue("map_sort_order", 0);
    }
    mapSortOrder = static_cast<MapSortOrder>(settings.value("map_sort_order").toInt());
}

void MainWindow::openRecentProject() {
    //QSettings settings;
    //settings.clear();
    //*
    QSettings settings;
    QString key = "recent_projects";
    if (settings.contains(key)) {
        QString default_dir = settings.value(key).toStringList().last();
        if (!default_dir.isNull()) {
            qDebug() << QString("default_dir: '%1'").arg(default_dir);
            openProject(default_dir);
        }
    }
    //*/
}

void MainWindow::openProject(QString dir) {
    if (dir.isNull()) {
        return;
    }

    this->statusBar()->showMessage(QString("Opening project %1").arg(dir));

    bool already_open = isProjectOpen() && (editor->project->root == dir);
    if (!already_open) {
        editor->project = new Project;
        editor->project->root = dir;
        setWindowTitle(editor->project->getProjectTitle());
        loadDataStructures();
        populateMapList();
        setMap(getDefaultMap(), true);
    } else {
        setWindowTitle(editor->project->getProjectTitle());
        loadDataStructures();
        populateMapList();
    }

    this->statusBar()->showMessage(QString("Opened project %1").arg(dir));
}

bool MainWindow::isProjectOpen() {
    return (editor && editor != nullptr)
        && (editor->project && editor->project != nullptr);
}

QString MainWindow::getDefaultMap() {
    if (editor && editor->project) {
        QList<QStringList> names = editor->project->groupedMapNames;
        if (!names.isEmpty()) {
            QSettings settings;
            QString key = "project:" + editor->project->root;
            if (settings.contains(key)) {
                QMap<QString, QVariant> qmap = settings.value(key).toMap();
                if (qmap.contains("recent_map")) {
                    QString map_name = qmap.value("recent_map").toString();
                    for (int i = 0; i < names.length(); i++) {
                        if (names.value(i).contains(map_name)) {
                            return map_name;
                        }
                    }
                }
            }
            // Failing that, just get the first map in the list.
            for (int i = 0; i < names.length(); i++) {
                QStringList list = names.value(i);
                if (list.length()) {
                    return list.value(0);
                }
            }
        }
    }
    return QString();
}

QString MainWindow::getExistingDirectory(QString dir) {
    return QFileDialog::getExistingDirectory(this, "Open Directory", dir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

void MainWindow::on_action_Open_Project_triggered()
{
    QSettings settings;
    QString key = "recent_projects";
    QString recent = ".";
    if (settings.contains(key)) {
        recent = settings.value(key).toStringList().last();
    }
    QString dir = getExistingDirectory(recent);
    if (!dir.isEmpty()) {
        QStringList recents;
        if (settings.contains(key)) {
            recents = settings.value(key).toStringList();
        }
        recents.removeAll(dir);
        recents.append(dir);
        settings.setValue(key, recents);

        openProject(dir);
    }
}

void MainWindow::setMap(QString map_name, bool scrollTreeView) {
    qDebug() << QString("setMap(%1)").arg(map_name);
    if (map_name.isNull()) {
        return;
    }
    if (editor->map != nullptr && !editor->map->name.isNull()) {
        ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(editor->map->name)), false);
    }
    editor->setMap(map_name);
    redrawMapScene();
    displayMapProperties();

    if (scrollTreeView) {
        // Make sure we clear the filter first so we actually have a scroll target
        mapListProxyModel->setFilterRegExp(QString::null);
        ui->mapList->setCurrentIndex(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)));
        ui->mapList->scrollTo(ui->mapList->currentIndex(), QAbstractItemView::PositionAtCenter);
    }

    ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)), true);

    setWindowTitle(map_name + " - " + editor->project->getProjectTitle());

    connect(editor->map, SIGNAL(mapChanged(Map*)), this, SLOT(onMapChanged(Map *)));
    connect(editor->map, SIGNAL(mapNeedsRedrawing()), this, SLOT(onMapNeedsRedrawing()));

    setRecentMap(map_name);
    updateMapList();
    updateTilesetEditor();
}

void MainWindow::redrawMapScene()
{
    editor->displayMap();
    on_tabWidget_currentChanged(ui->tabWidget->currentIndex());

    double base = editor->scale_base;
    double exp  = editor->scale_exp;

    int width = static_cast<int>(ceil((editor->scene->width()) * pow(base,exp))) + 2;
    int height = static_cast<int>(ceil((editor->scene->height()) * pow(base,exp))) + 2;

    ui->graphicsView_Map->setScene(editor->scene);
    ui->graphicsView_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Map->setFixedSize(width, height);

    ui->graphicsView_Objects_Map->setScene(editor->scene);
    ui->graphicsView_Objects_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Objects_Map->setFixedSize(width, height);
    ui->graphicsView_Objects_Map->editor = editor;

    ui->graphicsView_Connections->setScene(editor->scene);
    ui->graphicsView_Connections->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Connections->setFixedSize(width, height);

    ui->graphicsView_Metatiles->setScene(editor->scene_metatiles);
    //ui->graphicsView_Metatiles->setSceneRect(editor->scene_metatiles->sceneRect());
    ui->graphicsView_Metatiles->setFixedSize(editor->metatile_selector_item->pixmap().width() + 2, editor->metatile_selector_item->pixmap().height() + 2);

    ui->graphicsView_Region_Map_Metatiles->setScene(editor->scene_region_map_img_tiles);//scene_city_map_metatiles);

    ui->graphicsView_BorderMetatile->setScene(editor->scene_selected_border_metatiles);
    ui->graphicsView_BorderMetatile->setFixedSize(editor->selected_border_metatiles_item->pixmap().width() + 2, editor->selected_border_metatiles_item->pixmap().height() + 2);

    ui->graphicsView_currentMetatileSelection->setScene(editor->scene_current_metatile_selection);
    ui->graphicsView_currentMetatileSelection->setFixedSize(editor->scene_current_metatile_selection_item->pixmap().width() + 2, editor->scene_current_metatile_selection_item->pixmap().height() + 2);

    ui->graphicsView_Collision->setScene(editor->scene_collision_metatiles);
    //ui->graphicsView_Collision->setSceneRect(editor->scene_collision_metatiles->sceneRect());
    ui->graphicsView_Collision->setFixedSize(editor->movement_permissions_selector_item->pixmap().width() + 2, editor->movement_permissions_selector_item->pixmap().height() + 2);
}

void MainWindow::openWarpMap(QString map_name, QString warp_num) {
    // Ensure valid destination map name.
    if (!editor->project->mapNames->contains(map_name)) {
        qDebug() << QString("Invalid warp destination map name '%1'").arg(map_name);
        return;
    }

    // Ensure valid destination warp number.
    bool ok;
    int warpNum = warp_num.toInt(&ok, 0);
    if (!ok) {
        qDebug() << QString("Invalid warp number '%1' for destination map '%2'").arg(warp_num).arg(map_name);
        return;
    }

    // Open the destination map, and select the target warp event.
    setMap(map_name, true);
    QList<Event*> warp_events = editor->map->events["warp_event_group"];
    if (warp_events.length() > warpNum) {
        Event *warp_event = warp_events.at(warpNum);
        QList<DraggablePixmapItem *> *all_events = editor->getObjects();
        for (DraggablePixmapItem *item : *all_events) {
            if (item->event == warp_event) {
                editor->selected_events->clear();
                editor->selected_events->append(item);
                editor->updateSelectedEvents();
            }
        }

        delete all_events;
    }
}

void MainWindow::setRecentMap(QString map_name) {
    QSettings settings;
    QString key = "project:" + editor->project->root;
    QMap<QString, QVariant> qmap;
    if (settings.contains(key)) {
        qmap = settings.value(key).toMap();
    }
    qmap.insert("recent_map", map_name);
    settings.setValue(key, qmap);
}

void MainWindow::displayMapProperties() {
    ui->comboBox_Song->clear();
    ui->comboBox_Location->clear();
    ui->checkBox_Visibility->setChecked(false);
    ui->comboBox_Weather->clear();
    ui->comboBox_Type->clear();
    ui->comboBox_BattleScene->clear();
    ui->comboBox_PrimaryTileset->clear();
    ui->comboBox_SecondaryTileset->clear();
    ui->checkBox_ShowLocation->setChecked(false);
    if (!editor || !editor->map || !editor->project) {
        ui->frame_3->setEnabled(false);
        return;
    }
    ui->frame_3->setEnabled(true);
    Map *map = editor->map;
    Project *project = editor->project;

    QStringList songs = project->getSongNames();
    ui->comboBox_Song->addItems(songs);
    ui->comboBox_Song->setCurrentText(map->song);

    ui->comboBox_Location->addItems(*project->regionMapSections);
    ui->comboBox_Location->setCurrentText(map->location);

    QMap<QString, QStringList> tilesets = project->getTilesets();
    ui->comboBox_PrimaryTileset->addItems(tilesets.value("primary"));
    ui->comboBox_PrimaryTileset->setCurrentText(map->layout->tileset_primary_label);
    ui->comboBox_SecondaryTileset->addItems(tilesets.value("secondary"));
    ui->comboBox_SecondaryTileset->setCurrentText(map->layout->tileset_secondary_label);

    ui->checkBox_Visibility->setChecked(map->requiresFlash.toInt() > 0 || map->requiresFlash == "TRUE");

    ui->comboBox_Weather->addItems(*project->weatherNames);
    ui->comboBox_Weather->setCurrentText(map->weather);

    ui->comboBox_Type->addItems(*project->mapTypes);
    ui->comboBox_Type->setCurrentText(map->type);

    ui->comboBox_BattleScene->addItems(*project->mapBattleScenes);
    ui->comboBox_BattleScene->setCurrentText(map->battle_scene);

    ui->checkBox_ShowLocation->setChecked(map->show_location.toInt() > 0 || map->show_location == "TRUE");
}

void MainWindow::on_comboBox_Song_activated(const QString &song)
{
    if (editor && editor->map) {
        editor->map->song = song;
    }
}

void MainWindow::on_comboBox_Location_activated(const QString &location)
{
    if (editor && editor->map) {
        editor->map->location = location;
    }
}

void MainWindow::on_comboBox_Visibility_activated(const QString &requiresFlash)
{
    if (editor && editor->map) {
        editor->map->requiresFlash = requiresFlash;
    }
}

void MainWindow::on_comboBox_Weather_activated(const QString &weather)
{
    if (editor && editor->map) {
        editor->map->weather = weather;
    }
}

void MainWindow::on_comboBox_Type_activated(const QString &type)
{
    if (editor && editor->map) {
        editor->map->type = type;
    }
}

void MainWindow::on_comboBox_BattleScene_activated(const QString &battle_scene)
{
    if (editor && editor->map) {
        editor->map->battle_scene = battle_scene;
    }
}

void MainWindow::on_checkBox_Visibility_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->requiresFlash = "TRUE";
        } else {
            editor->map->requiresFlash = "FALSE";
        }
    }
}

void MainWindow::on_checkBox_ShowLocation_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->show_location = "TRUE";
        } else {
            editor->map->show_location = "FALSE";
        }
    }
}

void MainWindow::loadDataStructures() {
    Project *project = editor->project;
    project->readMapLayoutsTable();
    project->readAllMapLayouts();
    project->readRegionMapSections();
    project->readItemNames();
    project->readFlagNames();
    project->readVarNames();
    project->readMovementTypes();
    project->readMapTypes();
    project->readMapBattleScenes();
    project->readWeatherNames();
    project->readCoordEventWeatherNames();
    project->readSecretBaseIds();
    project->readBgEventFacingDirections();
    project->readMapsWithConnections();
    project->readMetatileBehaviors();
    project->readTilesetProperties();
    project->loadRegionMapData();
}

void MainWindow::populateMapList() {
    editor->project->readMapGroups();
    sortMapList();
}

void MainWindow::sortMapList() {
    Project *project = editor->project;

    QIcon mapFolderIcon;
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);

    QIcon folderIcon;
    folderIcon.addFile(QStringLiteral(":/icons/folder_closed.ico"), QSize(), QIcon::Normal, QIcon::Off);
    //folderIcon.addFile(QStringLiteral(":/icons/folder.ico"), QSize(), QIcon::Normal, QIcon::On);

    ui->mapList->setUpdatesEnabled(false);
    mapListModel->clear();
    mapGroupItemsList->clear();
    QStandardItem *root = mapListModel->invisibleRootItem();

    switch (mapSortOrder)
    {
        case MapSortOrder::Group:
            for (int i = 0; i < project->groupNames->length(); i++) {
                QString group_name = project->groupNames->value(i);
                QStandardItem *group = new QStandardItem;
                group->setText(group_name);
                group->setIcon(mapFolderIcon);
                group->setEditable(false);
                group->setData(group_name, Qt::UserRole);
                group->setData("map_group", MapListUserRoles::TypeRole);
                group->setData(i, MapListUserRoles::GroupRole);
                root->appendRow(group);
                mapGroupItemsList->append(group);
                QStringList names = project->groupedMapNames.value(i);
                for (int j = 0; j < names.length(); j++) {
                    QString map_name = names.value(j);
                    QStandardItem *map = createMapItem(map_name, i, j);
                    group->appendRow(map);
                    mapListIndexes.insert(map_name, map->index());
                }
            }
            break;
        case MapSortOrder::Name:
        {
            QMap<QString, int> mapsecToGroupNum;
            for (int i = 0; i < project->regionMapSections->length(); i++) {
                QString mapsec_name = project->regionMapSections->value(i);
                QStandardItem *mapsec = new QStandardItem;
                mapsec->setText(mapsec_name);
                mapsec->setIcon(folderIcon);
                mapsec->setEditable(false);
                mapsec->setData(mapsec_name, Qt::UserRole);
                mapsec->setData("map_sec", MapListUserRoles::TypeRole);
                mapsec->setData(i, MapListUserRoles::GroupRole);
                root->appendRow(mapsec);
                mapGroupItemsList->append(mapsec);
                mapsecToGroupNum.insert(mapsec_name, i);
            }
            for (int i = 0; i < project->groupNames->length(); i++) {
                QStringList names = project->groupedMapNames.value(i);
                for (int j = 0; j < names.length(); j++) {
                    QString map_name = names.value(j);
                    QStandardItem *map = createMapItem(map_name, i, j);
                    QString location = project->readMapLocation(map_name);
                    QStandardItem *mapsecItem = mapGroupItemsList->at(mapsecToGroupNum[location]);
                    mapsecItem->setIcon(mapFolderIcon);
                    mapsecItem->appendRow(map);
                    mapListIndexes.insert(map_name, map->index());
                }
            }
            break;
        }
        case MapSortOrder::Layout:
        {
            for (int i = 0; i < project->mapLayoutsTable.length(); i++) {
                QString layoutName = project->mapLayoutsTable.value(i);
                QStandardItem *layout = new QStandardItem;
                layout->setText(layoutName);
                layout->setIcon(folderIcon);
                layout->setEditable(false);
                layout->setData(layoutName, Qt::UserRole);
                layout->setData("map_layout", MapListUserRoles::TypeRole);
                layout->setData(i, MapListUserRoles::GroupRole);
                root->appendRow(layout);
                mapGroupItemsList->append(layout);
            }
            for (int i = 0; i < project->groupNames->length(); i++) {
                QStringList names = project->groupedMapNames.value(i);
                for (int j = 0; j < names.length(); j++) {
                    QString map_name = names.value(j);
                    QStandardItem *map = createMapItem(map_name, i, j);
                    QString layoutId = project->readMapLayoutId(map_name);
                    QStandardItem *layoutItem = mapGroupItemsList->at(layoutId.toInt() - 1);
                    layoutItem->setIcon(mapFolderIcon);
                    layoutItem->appendRow(map);
                    mapListIndexes.insert(map_name, map->index());
                }
            }
            break;
        }
    }

    ui->mapList->setUpdatesEnabled(true);
    ui->mapList->expandToDepth(0);
    ui->mapList->repaint();
}

QStandardItem* MainWindow::createMapItem(QString mapName, int groupNum, int inGroupNum) {
    QStandardItem *map = new QStandardItem;
    map->setText(QString("[%1.%2] ").arg(groupNum).arg(inGroupNum, 2, 10, QLatin1Char('0')) + mapName);
    map->setIcon(*mapIcon);
    map->setEditable(false);
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListUserRoles::TypeRole);
    return map;
}

void MainWindow::onOpenMapListContextMenu(const QPoint &point)
{
    QModelIndex index = mapListProxyModel->mapToSource(ui->mapList->indexAt(point));
    if (!index.isValid()) {
        return;
    }

    QStandardItem *selectedItem = mapListModel->itemFromIndex(index);
    QVariant itemType = selectedItem->data(MapListUserRoles::TypeRole);
    if (!itemType.isValid()) {
        return;
    }

    // Build custom context menu depending on which type of item was selected (map group, map name, etc.)
    if (itemType == "map_group") {
        QString groupName = selectedItem->data(Qt::UserRole).toString();
        int groupNum = selectedItem->data(MapListUserRoles::GroupRole).toInt();
        QMenu* menu = new QMenu(this);
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map to Group"))->setData(groupNum);
        connect(actions, SIGNAL(triggered(QAction*)), this, SLOT(onAddNewMapToGroupClick(QAction*)));
        menu->exec(QCursor::pos());
    }
}

void MainWindow::onAddNewMapToGroupClick(QAction* triggeredAction)
{
    int groupNum = triggeredAction->data().toInt();
    QStandardItem* groupItem = mapGroupItemsList->at(groupNum);

    QString newMapName = editor->project->getNewMapName();
    Map* newMap = editor->project->addNewMapToGroup(newMapName, groupNum);
    editor->project->saveMap(newMap);
    editor->project->saveAllDataStructures();

    int numMapsInGroup = groupItem->rowCount();
    QStandardItem *newMapItem = createMapItem(newMapName, groupNum, numMapsInGroup);
    groupItem->appendRow(newMapItem);
    mapListIndexes.insert(newMapName, newMapItem->index());

    setMap(newMapName);
}

void MainWindow::onTilesetChanged(QString mapName)
{
    setMap(mapName);
}

void MainWindow::updateTilesetEditor() {
    if (this->tilesetEditor) {
        this->tilesetEditor->setTilesets(editor->ui->comboBox_PrimaryTileset->currentText(), editor->ui->comboBox_SecondaryTileset->currentText());
    }
}

void MainWindow::currentMetatilesSelectionChanged()
{
    ui->graphicsView_currentMetatileSelection->setFixedSize(editor->scene_current_metatile_selection_item->pixmap().width() + 2, editor->scene_current_metatile_selection_item->pixmap().height() + 2);
    ui->graphicsView_currentMetatileSelection->setSceneRect(0, 0, editor->scene_current_metatile_selection_item->pixmap().width(), editor->scene_current_metatile_selection_item->pixmap().height());

    QPoint size = editor->metatile_selector_item->getSelectionDimensions();
    if (size.x() == 1 && size.y() == 1) {
        QPoint pos = editor->metatile_selector_item->getMetatileIdCoordsOnWidget(editor->metatile_selector_item->getSelectedMetatiles()->at(0));
        ui->scrollArea_2->ensureVisible(pos.x(), pos.y());
    }
}

void MainWindow::on_mapList_activated(const QModelIndex &index)
{
    QVariant data = index.data(Qt::UserRole);
    if (index.data(MapListUserRoles::TypeRole) == "map_name" && !data.isNull()) {
        setMap(data.toString());
    }
}

void MainWindow::markAllEdited(QAbstractItemModel *model) {
    QList<QModelIndex> list;
    list.append(QModelIndex());
    while (list.length()) {
        QModelIndex parent = list.takeFirst();
        for (int i = 0; i < model->rowCount(parent); i++) {
            QModelIndex index = model->index(i, 0, parent);
            if (model->hasChildren(index)) {
                list.append(index);
            }
            markEdited(index);
        }
    }
}

void MainWindow::markEdited(QModelIndex index) {
    QVariant data = index.data(Qt::UserRole);
    if (!data.isNull()) {
        QString map_name = data.toString();
        if (editor->project) {
            if (editor->project->map_cache->contains(map_name)) {
                if (editor->project->map_cache->value(map_name)->hasUnsavedChanges()) {
                    mapListModel->itemFromIndex(mapListIndexes.value(map_name))->setIcon(*mapEditedIcon);
                }
            }
        }
    }
}

void MainWindow::updateMapList() {
    QAbstractItemModel *model = ui->mapList->model();
    markAllEdited(model);
}

void MainWindow::on_action_Save_Project_triggered()
{
    editor->saveProject();
    updateMapList();
}

void MainWindow::undo() {
    editor->undo();
}

void MainWindow::redo() {
    editor->redo();
}

// Open current map scripts in system default editor for .inc files
void MainWindow::openInTextEditor() {
    QString path = QDir::cleanPath("file://" + editor->project->root + QDir::separator() + "data/maps/" + editor->map->name + "/scripts.inc");
    QDesktopServices::openUrl(QUrl(path));
}

void MainWindow::on_action_Save_triggered() {
    editor->save();
    updateMapList();
}

void MainWindow::on_tabWidget_2_currentChanged(int index)
{
    if (index == 0) {
        editor->setEditingMap();
    } else if (index == 1) {
        editor->setEditingCollision();
    }
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 0) {
        on_tabWidget_2_currentChanged(ui->tabWidget_2->currentIndex());
    } else if (index == 1) {
        editor->setEditingObjects();
    } else if (index == 3) {
        editor->setEditingConnections();
    }
}

void MainWindow::on_actionUndo_triggered()
{
    undo();
}

void MainWindow::on_actionRedo_triggered()
{
    redo();
}

void MainWindow::on_actionZoom_In_triggered() {
    scaleMapView(1);
}

void MainWindow::on_actionZoom_Out_triggered() {
    scaleMapView(-1);
}

void MainWindow::on_actionBetter_Cursors_triggered() {
    QSettings settings;
    settings.setValue("cursor_mode", QString::number(ui->actionBetter_Cursors->isChecked()));
    this->editor->settings->betterCursors = ui->actionBetter_Cursors->isChecked();
}

void MainWindow::on_actionPencil_triggered()
{
    on_toolButton_Paint_clicked();
}

void MainWindow::on_actionPointer_triggered()
{
    on_toolButton_Select_clicked();
}

void MainWindow::on_actionFlood_Fill_triggered()
{
    on_toolButton_Fill_clicked();
}

void MainWindow::on_actionEyedropper_triggered()
{
    on_toolButton_Dropper_clicked();
}

void MainWindow::on_actionMove_triggered()
{
    on_toolButton_Move_clicked();
}

void MainWindow::on_actionMap_Shift_triggered()
{
    on_toolButton_Shift_clicked();
}

void MainWindow::scaleMapView(int s) {
    if ((editor->scale_exp + s) <= 5 && (editor->scale_exp + s) >= -2)    // sane limits
    {
        if (s == 0)
        {
            s = -editor->scale_exp;
        }

        editor->scale_exp += s;

        double base = editor->scale_base;
        double exp  = editor->scale_exp;
        double sfactor = pow(base,s);

        ui->graphicsView_Map->scale(sfactor,sfactor);
        ui->graphicsView_Objects_Map->scale(sfactor,sfactor);
        ui->graphicsView_Connections->scale(sfactor,sfactor);

        int width = static_cast<int>(ceil((editor->scene->width()) * pow(base,exp))) + 2;
        int height = static_cast<int>(ceil((editor->scene->height()) * pow(base,exp))) + 2;
        ui->graphicsView_Map->setFixedSize(width, height);
        ui->graphicsView_Objects_Map->setFixedSize(width, height);
        ui->graphicsView_Connections->setFixedSize(width, height);
    }
}

void MainWindow::resetMapViewScale() {
    scaleMapView(0);
}

void MainWindow::addNewEvent(QString event_type)
{
    if (editor) {
        DraggablePixmapItem *object = editor->addNewEvent(event_type);
        if (object) {
            editor->selectMapEvent(object, false);
        }
        updateSelectedObjects();
    }
}

// Should probably just pass layout and let the editor work it out
void MainWindow::updateSelectedObjects() {
    QList<DraggablePixmapItem *> *all_events = editor->getObjects();
    QList<DraggablePixmapItem *> *events = nullptr;

    if (editor->selected_events && editor->selected_events->length()) {
        events = editor->selected_events;
    } else {
        events = new QList<DraggablePixmapItem*>;
        if (all_events && all_events->length()) {
            DraggablePixmapItem *selectedEvent = all_events->first();
            editor->selected_events->append(selectedEvent);
            editor->redrawObject(selectedEvent);
            events->append(selectedEvent);
        }
    }

    QMap<QString, int> event_obj_gfx_constants = editor->project->getEventObjGfxConstants();

    QList<EventPropertiesFrame *> frames;

    for (DraggablePixmapItem *item : *events) {
        EventPropertiesFrame *frame = new EventPropertiesFrame;
//        frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        QSpinBox *x = frame->ui->spinBox_x;
        QSpinBox *y = frame->ui->spinBox_y;
        QSpinBox *z = frame->ui->spinBox_z;

        x->setValue(item->event->x());
        connect(x, SIGNAL(valueChanged(QString)), item, SLOT(set_x(QString)));
        connect(item, SIGNAL(xChanged(int)), x, SLOT(setValue(int)));

        y->setValue(item->event->y());
        connect(y, SIGNAL(valueChanged(QString)), item, SLOT(set_y(QString)));
        connect(item, SIGNAL(yChanged(int)), y, SLOT(setValue(int)));

        z->setValue(item->event->elevation());
        connect(z, SIGNAL(valueChanged(QString)), item, SLOT(set_elevation(QString)));
        connect(item, SIGNAL(elevationChanged(int)), z, SLOT(setValue(int)));

        QFont font;
        font.setCapitalization(QFont::Capitalize);
        frame->ui->label_name->setFont(font);
        QString event_type = item->event->get("event_type");
        QString event_group_type = item->event->get("event_group_type");
        QString map_name = item->event->get("map_name");
        int event_offs;
        if (event_type == "event_warp") { event_offs = 0; }
        else { event_offs = 1; }
        frame->ui->label_name->setText(
            QString("%1: %2 %3")
                .arg(editor->project->getMap(map_name)->events.value(event_group_type).indexOf(item->event) + event_offs)
                .arg(map_name)
                .arg(event_type)
        );

        frame->ui->label_spritePixmap->setPixmap(item->event->pixmap);
        connect(item, SIGNAL(spriteChanged(QPixmap)), frame->ui->label_spritePixmap, SLOT(setPixmap(QPixmap)));

        frame->ui->sprite->setVisible(false);

        QMap<QString, QString> field_labels;
        field_labels["script_label"] = "Script";
        field_labels["event_flag"] = "Event Flag";
        field_labels["movement_type"] = "Movement";
        field_labels["radius_x"] = "Movement Radius X";
        field_labels["radius_y"] = "Movement Radius Y";
        field_labels["is_trainer"] = "Trainer";
        field_labels["sight_radius_tree_id"] = "Sight Radius / Berry Tree ID";
        field_labels["destination_warp"] = "Destination Warp";
        field_labels["destination_map_name"] = "Destination Map";
        field_labels["script_var"] = "Var";
        field_labels["script_var_value"] = "Var Value";
        field_labels["player_facing_direction"] = "Player Facing Direction";
        field_labels["item"] = "Item";
        field_labels["item_unknown5"] = "Unknown 5";
        field_labels["item_unknown6"] = "Unknown 6";
        field_labels["weather"] = "Weather";
        field_labels["flag"] = "Flag";
        field_labels["secret_base_id"] = "Secret Base Id";

        QStringList fields;

        if (event_type == EventType::Object) {

            frame->ui->sprite->setVisible(true);
            frame->ui->comboBox_sprite->addItems(event_obj_gfx_constants.keys());
            frame->ui->comboBox_sprite->setCurrentText(item->event->get("sprite"));
            connect(frame->ui->comboBox_sprite, SIGNAL(activated(QString)), item, SLOT(set_sprite(QString)));

            /*
            frame->ui->script->setVisible(true);
            frame->ui->comboBox_script->addItem(item->event->get("script_label"));
            frame->ui->comboBox_script->setCurrentText(item->event->get("script_label"));
            //item->bind(frame->ui->comboBox_script, "script_label");
            connect(frame->ui->comboBox_script, SIGNAL(activated(QString)), item, SLOT(set_script(QString)));
            //connect(frame->ui->comboBox_script, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), item, [item](QString script_label){ item->event->put("script_label", script_label); });
            //connect(item, SIGNAL(scriptChanged(QString)), frame->ui->comboBox_script, SLOT(setValue(QString)));
            */

            fields << "movement_type";
            fields << "radius_x";
            fields << "radius_y";
            fields << "script_label";
            fields << "event_flag";
            fields << "is_trainer";
            fields << "sight_radius_tree_id";
        }
        else if (event_type == EventType::Warp) {
            fields << "destination_map_name";
            fields << "destination_warp";
        }
        else if (event_type == EventType::CoordScript) {
            fields << "script_label";
            fields << "script_var";
            fields << "script_var_value";
        }
        else if (event_type == EventType::CoordWeather) {
            fields << "weather";
        }
        else if (event_type == EventType::Sign) {
            fields << "player_facing_direction";
            fields << "script_label";
        }
        else if (event_type == EventType::HiddenItem) {
            fields << "item";
            fields << "flag";
        }
        else if (event_type == EventType::SecretBase) {
            fields << "secret_base_id";
        }

        for (QString key : fields) {
            QString value = item->event->get(key);
            QWidget *widget = new QWidget(frame);
            QFormLayout *fl = new QFormLayout(widget);
            fl->setContentsMargins(9, 0, 9, 0);

            // is_trainer is the only non-combobox item.
            if (key == "is_trainer") {
                QCheckBox *checkbox = new QCheckBox(widget);
                checkbox->setEnabled(true);
                checkbox->setChecked(value.toInt() != 0 && value != "FALSE");
                checkbox->setToolTip("Whether or not this object is trainer.");
                fl->addRow(new QLabel(field_labels[key], widget), checkbox);
                widget->setLayout(fl);
                frame->layout()->addWidget(widget);
                connect(checkbox, &QCheckBox::stateChanged, [=](int state) {
                    QString isTrainer = state == Qt::Checked ? "TRUE" : "FALSE";
                    item->event->put("is_trainer", isTrainer);
                });
                continue;
            }

            NoScrollComboBox *combo = new NoScrollComboBox(widget);
            combo->setEditable(true);

            if (key == "destination_map_name") {
                if (!editor->project->mapNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->mapNames);
                combo->setToolTip("The destination map name of the warp.");
            } else if (key == "destination_warp") {
                combo->setToolTip("The warp id on the destination map.");
            } else if (key == "item") {
                if (!editor->project->itemNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->itemNames);
            } else if (key == "flag" || key == "event_flag") {
                if (!editor->project->flagNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->flagNames);
                if (key == "flag")
                    combo->setToolTip("The flag which is set when the hidden item is picked up.");
                else if (key == "event_flag")
                    combo->setToolTip("The flag which hides the object when set.");
            } else if (key == "script_var") {
                if (!editor->project->varNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->varNames);
                combo->setToolTip("The variable by which the script is triggered. The script is triggered when this variable's value matches 'Var Value'.");
            } else if (key == "script_var_value")  {
                combo->setToolTip("The variable's value which triggers the script.");
            } else if (key == "movement_type") {
                if (!editor->project->movementTypes->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->movementTypes);
                combo->setToolTip("The object's natural movement behavior when the player is not interacting with it.");
            } else if (key == "weather") {
                if (!editor->project->coordEventWeatherNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->coordEventWeatherNames);
                combo->setToolTip("The weather that starts when the player steps on this spot.");
            } else if (key == "secret_base_id") {
                if (!editor->project->secretBaseIds->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->secretBaseIds);
                combo->setToolTip("The secret base id which is inside this secret base entrance. Secret base ids are meant to be unique to each and every secret base entrance.");
            } else if (key == "player_facing_direction") {
                if (!editor->project->bgEventFacingDirections->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->bgEventFacingDirections);
                combo->setToolTip("The direction which the player must be facing to be able to interact with this event.");
            } else if (key == "radius_x") {
                combo->setToolTip("The maximum number of metatiles this object is allowed to move left or right during its normal movement behavior actions.");
            } else if (key == "radius_y") {
                combo->setToolTip("The maximum number of metatiles this object is allowed to move up or down during its normal movement behavior actions.");
            } else if (key == "script_label") {
                combo->setToolTip("The script which is executed with this event.");
            } else if (key == "sight_radius_tree_id") {
                combo->setToolTip("The maximum sight range of a trainer, OR the unique id of the berry tree.");
            } else {
                combo->addItem(value);
            }
            combo->setCurrentText(value);

            fl->addRow(new QLabel(field_labels[key], widget), combo);
            widget->setLayout(fl);
            frame->layout()->addWidget(widget);

            item->bind(combo, key);
        }

        frames.append(frame);

    }

    //int scroll = ui->scrollArea_4->verticalScrollBar()->value();

    QWidget *target = ui->scrollAreaWidgetContents;

    if (target->children().length()) {
        qDeleteAll(target->children());
    }

    QVBoxLayout *layout = new QVBoxLayout(target);
    target->setLayout(layout);
    ui->scrollArea_4->setWidgetResizable(true);
    ui->scrollArea_4->setWidget(target);

    for (EventPropertiesFrame *frame : frames) {
        layout->addWidget(frame);
    }

    layout->addStretch(1);

    // doesn't work
    //QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    //ui->scrollArea_4->ensureVisible(0, scroll);
}

void MainWindow::on_toolButton_deleteObject_clicked()
{
    if (editor && editor->selected_events) {
        if (editor->selected_events->length()) {
            for (DraggablePixmapItem *item : *editor->selected_events) {
                if (item->event->get("event_type") != EventType::HealLocation) {
                    editor->deleteEvent(item->event);
                    if (editor->scene->items().contains(item)) {
                        editor->scene->removeItem(item);
                    }
                    editor->selected_events->removeOne(item);
                }
                else { // don't allow deletion of heal locations
                    qDebug() << "Cannot delete event of type " << item->event->get("event_type");
                }
            }
            updateSelectedObjects();
        }
    }
}

void MainWindow::on_toolButton_Open_Scripts_clicked()
{
    openInTextEditor();
}

void MainWindow::on_toolButton_Paint_clicked()
{
    editor->map_edit_mode = "paint";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pencil_cursor.ico"), 10, 10);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Select_clicked()
{
    editor->map_edit_mode = "select";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/cursor.ico"), 0, 0);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Fill_clicked()
{
    editor->map_edit_mode = "fill";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/fill_color_cursor.ico"), 10, 10);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Dropper_clicked()
{
    editor->map_edit_mode = "pick";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pipette_cursor.ico"), 10, 10);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Move_clicked()
{
    editor->map_edit_mode = "move";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/move.ico"), 7, 7);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(ui->scrollArea, QScroller::LeftMouseButtonGesture);

    checkToolButtons();
}

void MainWindow::on_toolButton_Shift_clicked()
{
    editor->map_edit_mode = "shift";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/shift_cursor.ico"), 10, 10);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::checkToolButtons() {
    ui->toolButton_Paint->setChecked(editor->map_edit_mode == "paint");
    ui->toolButton_Select->setChecked(editor->map_edit_mode == "select");
    ui->toolButton_Fill->setChecked(editor->map_edit_mode == "fill");
    ui->toolButton_Dropper->setChecked(editor->map_edit_mode == "pick");
    ui->toolButton_Move->setChecked(editor->map_edit_mode == "move");
    ui->toolButton_Shift->setChecked(editor->map_edit_mode == "shift");
}

void MainWindow::onLoadMapRequested(QString mapName, QString fromMapName) {
    setMap(mapName, true);
    editor->setSelectedConnectionFromMap(fromMapName);
}

void MainWindow::onMapChanged(Map *map) {
    map->layout->has_unsaved_changes = true;
    updateMapList();
}

void MainWindow::onMapNeedsRedrawing() {
    redrawMapScene();
}

void MainWindow::onTilesetsSaved(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    this->editor->updatePrimaryTileset(primaryTilesetLabel, true);
    this->editor->updateSecondaryTileset(secondaryTilesetLabel, true);
}

void MainWindow::on_action_Export_Map_Image_triggered()
{
    QString defaultFilepath = QString("%1/%2.png").arg(editor->project->root).arg(editor->map->name);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Map Image", defaultFilepath, "Image Files (*.png *.jpg *.bmp)");
    if (!filepath.isEmpty()) {
        editor->map_item->pixmap().save(filepath);
    }
}

void MainWindow::on_comboBox_ConnectionDirection_currentIndexChanged(const QString &direction)
{
    editor->updateCurrentConnectionDirection(direction);
}

void MainWindow::on_spinBox_ConnectionOffset_valueChanged(int offset)
{
    editor->updateConnectionOffset(offset);
}

void MainWindow::on_comboBox_ConnectedMap_currentTextChanged(const QString &mapName)
{
    editor->setConnectionMap(mapName);
}

void MainWindow::on_pushButton_AddConnection_clicked()
{
    editor->addNewConnection();
}

void MainWindow::on_pushButton_RemoveConnection_clicked()
{
    editor->removeCurrentConnection();
}

void MainWindow::on_comboBox_DiveMap_currentTextChanged(const QString &mapName)
{
    editor->updateDiveMap(mapName);
}

void MainWindow::on_comboBox_EmergeMap_currentTextChanged(const QString &mapName)
{
    editor->updateEmergeMap(mapName);
}

void MainWindow::on_comboBox_PrimaryTileset_activated(const QString &tilesetLabel)
{
    editor->updatePrimaryTileset(tilesetLabel);
}

void MainWindow::on_comboBox_SecondaryTileset_activated(const QString &tilesetLabel)
{
    editor->updateSecondaryTileset(tilesetLabel);
}

void MainWindow::on_pushButton_clicked()
{
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("Change Map Dimensions");
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);

    QSpinBox *widthSpinBox = new QSpinBox();
    QSpinBox *heightSpinBox = new QSpinBox();
    widthSpinBox->setMinimum(1);
    heightSpinBox->setMinimum(1);
    // See below for explanation of maximum map dimensions
    widthSpinBox->setMaximum(0x1E7);
    heightSpinBox->setMaximum(0x1D1);
    widthSpinBox->setValue(editor->map->getWidth());
    heightSpinBox->setValue(editor->map->getHeight());
    form.addRow(new QLabel("Width"), widthSpinBox);
    form.addRow(new QLabel("Height"), heightSpinBox);

    QLabel *errorLabel = new QLabel();
    QPalette errorPalette;
    errorPalette.setColor(QPalette::WindowText, Qt::red);
    errorLabel->setPalette(errorPalette);
    errorLabel->setVisible(false);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, [&dialog, &widthSpinBox, &heightSpinBox, &errorLabel](){
        // Ensure width and height are an acceptable size.
        // The maximum number of metatiles in a map is the following:
        //    max = (width + 15) * (height + 14)
        // This limit can be found in fieldmap.c in pokeruby/pokeemerald.
        int realWidth = widthSpinBox->value() + 15;
        int realHeight = heightSpinBox->value() + 14;
        int numMetatiles = realWidth * realHeight;
        if (numMetatiles <= 0x2800) {
            dialog.accept();
        } else {
            QString errorText = QString("Error: The specified width and height are too large.\n"
                    "The maximum width and height is the following: (width + 15) * (height + 14) <= 10240\n"
                    "The specified width and height was: (%1 + 15) * (%2 + 14) = %3")
                        .arg(widthSpinBox->value())
                        .arg(heightSpinBox->value())
                        .arg(numMetatiles);
            errorLabel->setText(errorText);
            errorLabel->setVisible(true);
        }
    });
    connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    form.addRow(errorLabel);

    if (dialog.exec() == QDialog::Accepted) {
        editor->map->setDimensions(widthSpinBox->value(), heightSpinBox->value());
        editor->map->commit();
        onMapNeedsRedrawing();
    }
}

void MainWindow::on_checkBox_smartPaths_stateChanged(int selected)
{
    editor->settings->smartPathsEnabled = selected == Qt::Checked;
}

void MainWindow::on_checkBox_ToggleBorder_stateChanged(int selected)
{
    bool visible = selected != 0;
    editor->toggleBorderVisibility(visible);
}

void MainWindow::on_actionTileset_Editor_triggered()
{
    if (!this->tilesetEditor) {
        this->tilesetEditor = new TilesetEditor(this->editor->project, this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label, this);
        connect(this->tilesetEditor, SIGNAL(tilesetsSaved(QString, QString)), this, SLOT(onTilesetsSaved(QString, QString)));
        connect(this->tilesetEditor, SIGNAL(closed()), this, SLOT(onTilesetEditorClosed()));
    }

    if (!this->tilesetEditor->isVisible()) {
        this->tilesetEditor->show();
    } else if (this->tilesetEditor->isMinimized()) {
        this->tilesetEditor->showNormal();
    } else {
        this->tilesetEditor->activateWindow();
    }
}

void MainWindow::onTilesetEditorClosed() {
    if (this->tilesetEditor) {
        delete this->tilesetEditor;
        this->tilesetEditor = nullptr;
    }
}
