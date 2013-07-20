#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qpandaapplication.h"
#include <QInputDialog>

#include "scenecamera.h"
#include "mouse.h"
#include <QElapsedTimer>
#include "functorthread.h"
#include "selectableresource.h"

#define PANDA_TAB 1

extern PandaFramework framework;

QString objectTypes[] = { "NPC", "Shelf", "Locker", "Door" };

struct PandaTask : public AsyncTask
{
  MainWindow*      main_window;
  QWidget*         panda_widget;
  WindowFramework* window;
  SceneCamera*     camera;
  Mouse*           mouse;
  QElapsedTimer    timer;

  PandaTask() : window(0), camera(0), mouse(0) { }

  DoneStatus do_task(void)
  {
    float elapsedTime = timer.elapsed();

    elapsedTime /= 1000;
    camera->SetEnabledScroll(dynamic_cast<MouseWatcher*>(window->get_mouse().node())->has_mouse());
    camera->Run(elapsedTime);
    mouse->Run();
    timer.start();
    main_window->ShowWaypointZone();
    return (AsyncTask::DS_cont);
  }

  LPoint3 CollidingAt(MapObject* object)
  {
    NodePath                  camera                  = window->get_camera_group();
    PT(CollisionRay)          picker_ray              = new CollisionRay();
    PT(CollisionNode)         picker_node             = new CollisionNode("mouse_ray");
    NodePath                  picker_path             = camera.attach_new_node(picker_node);
    CollisionTraverser        traverser;
    PT(CollisionHandlerQueue) collision_handler_queue = new CollisionHandlerQueue();
    LPoint2f                  cursor_pos              = mouse->GetPosition();

    picker_ray->set_from_lens(window->get_camera(0), cursor_pos.get_x(), cursor_pos.get_y());
    picker_node->add_solid(picker_ray);
    traverser.add_collider(picker_path, collision_handler_queue);
    traverser.traverse(object->nodePath);
    collision_handler_queue->sort_entries();
    for (int i = 0 ; i < collision_handler_queue->get_num_entries() ; ++i)
    {
      CollisionEntry* entry = collision_handler_queue->get_entry(i);
      NodePath        np    = entry->get_into_node_path();
      LPoint3         pos;

      if (object->nodePath.is_ancestor_of(np))
        return (entry->get_surface_point(object->nodePath));
    }
    return (LPoint3(0, 0, 0));
  }
};

PandaTask my_task;

QString strScriptCategories[5] = {
    "Dialogues", "Quests", "Artificial Intelligence", "Items", "Buffs"
};

QString pathScriptCategories[5] = {
    "dialogs", "quests", "ai", "objects", "buffs"
};

MainWindow::MainWindow(QPandaApplication* app, QWidget *parent) : QMainWindow(parent), _app(*app), ui(new Ui::MainWindow), tabScript(this, ui), tabDialog(this, ui), tabL18n(this, ui), splashScreen(this), wizardObject(this), dialogObject(this)
{
    QIcon iconScript("icons/script.png");
    QIcon iconItems("icons/item.png");
    QIcon iconDialogs("icons/dialogs.png");
    QIcon iconLevel("icons/level.png");
    QIcon iconWorldmap("icons/worldmap.png");
    QIcon iconDelete("icons/delete.png");
    QIcon iconSave("icons/save.png");
    QIcon iconAdd("icons/add.png");
    QIcon iconUp("icons/up.png");
    QIcon iconDown("icons/down.png");
    QIcon iconLeft("icons/previous.png");
    QIcon iconRight("icons/next.png");
    QIcon iconClose("icons/close.png");
    QIcon iconLanguage("icons/internationalization.png");
    QIcon iconEdit("icons/edit.png");
    QIcon iconConnect("icons/connect.png");
    QIcon iconDisconnect("icons/disconnect.png");
    QIcon waypointGenerate("icons/waypoint-generate.png");

    level_editor_lock = 0;
    world             = 0;
    objectFile        = 0;
    ui->setupUi(this);

    ui->charsheetAdd->setIcon(iconAdd);
    ui->charsheetDel->setIcon(iconDelete);
    ui->charsheetSave->setIcon(iconSave);

    ui->waypointConnect->setIcon(iconConnect);
    ui->waypointDisconnect->setIcon(iconDisconnect);
    ui->waypointSelDelete->setIcon(iconDelete);
    waypointSelX = waypointSelY = waypointSelZ = 0;

    ui->objectRemove->setIcon(iconDelete);

    ui->tabWidget->setTabIcon(0, iconLevel);
    ui->tabWidget->setTabIcon(1, iconWorldmap);
    ui->tabWidget->setTabIcon(2, iconScript);
    ui->tabWidget->setTabIcon(3, iconScript);
    ui->tabWidget->setTabIcon(4, iconItems);
    ui->tabWidget->setTabIcon(5, iconDialogs);
    ui->tabWidget->setTabIcon(6, iconLanguage);
    ui->scriptNew->setIcon(iconAdd);
    ui->dialogNew->setIcon(iconAdd);
    ui->mapNew->setIcon(iconAdd);
    ui->scriptDelete->setIcon(iconDelete);
    ui->dialogDelete->setIcon(iconDelete);
    ui->scriptNext->setIcon(iconRight);
    ui->scriptPrevious->setIcon(iconLeft);
    ui->scriptSave->setIcon(iconSave);
    ui->scriptClose->setIcon(iconClose);
    ui->languageNew->setIcon(iconAdd);
    ui->languageDelete->setIcon(iconDelete);
    ui->mapNewObject->setIcon(iconAdd);
    ui->objectEdit->setIcon(iconEdit);
    ui->saveMap->setIcon(iconSave);

    ui->progressBar->hide();

    setWindowTitle("Fallout Equestria Editor");
    setWindowIcon(QIcon("icons/app-icon.png"));

    splashScreen.open();

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(CurrentTabChanged(int)));

    connect(&splashScreen, SIGNAL(rejected()), app,  SLOT(Terminate()));
    connect(&splashScreen, SIGNAL(accepted()), this, SLOT(LoadProject()));

    connect(ui->widget, SIGNAL(Initialized()), this, SLOT(PandaInitialized()));
    connect(this,       SIGNAL(Closed()),      app,  SLOT(Terminate()));

    for (short i = 0 ; i < 5 ; ++i)
    {
        tabScript.scriptCategories[i].setText(0, strScriptCategories[i]);
        ui->scriptList->addTopLevelItem(&(tabScript.scriptCategories[i]));
    }

    connect(ui->scriptList,   SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), &tabScript, SLOT(SwapScript(QTreeWidgetItem*)));
    connect(ui->scriptList,   SIGNAL(itemClicked(QTreeWidgetItem*,int)), &tabScript, SLOT(SwapScript(QTreeWidgetItem*)));
    connect(ui->scriptNew,    SIGNAL(clicked()),  &tabScript.dialogNewScript, SLOT(open()));
    connect(&tabScript.dialogNewScript, SIGNAL(accepted()), &tabScript, SLOT(NewScript()));

    connect(ui->scriptClose,    SIGNAL(clicked()), &tabScript, SLOT(CloseScript()));
    connect(ui->scriptSave,     SIGNAL(clicked()), &tabScript, SLOT(SaveScript()));
    connect(ui->scriptPrevious, SIGNAL(clicked()), &tabScript, SLOT(PreviousScript()));
    connect(ui->scriptNext,     SIGNAL(clicked()), &tabScript, SLOT(NextScript()));
    connect(ui->scriptSearch,   SIGNAL(textChanged(QString)), &tabScript, SLOT(FilterScript(QString)));
    connect(ui->scriptDelete,   SIGNAL(clicked()), &tabScript, SLOT(RemoveScript()));
    connect(ui->scriptSearch,   SIGNAL(cursorPositionChanged(int,int)), this, SLOT(FilterInit()));

	connect(&tabDialog, SIGNAL(RequestLocale()), this, SLOT(SetDefaultLocalization()) );

    connect(ui->dialogDelete, SIGNAL(clicked()), &tabDialog, SLOT(RemoveDialog()));
    connect(ui->dialogNew,    SIGNAL(clicked()), &tabDialog, SLOT(NewDialog()));
    connect(ui->dialogList,   SIGNAL(currentIndexChanged(QString)), &tabDialog, SLOT(SwapDialog(QString)));
	//connect(ui->dialogEditor, SIGNAL(UpdateLocale()), &tabL18n, SLOT(UpdateLocale()));
	connect(ui->dialogList,   SIGNAL(currentIndexChanged(QString)), &tabDialog, SLOT(SwapDialog(QString)));
    //connect(ui->dialogSearch, SIGNAL(textChanged(QString)), &tabDialog, SLOT(FilterDialog(QString)));

    connect(ui->languageDelete, SIGNAL(clicked()), &tabL18n, SLOT(RemoveLanguage()));
    connect(ui->languageNew, SIGNAL(clicked()), &tabL18n, SLOT(NewLanguage()));
    connect(ui->languageList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), &tabL18n, SLOT(SwapLanguage(QListWidgetItem*)));

    connect(ui->listMap, SIGNAL(currentIndexChanged(QString)), this, SLOT(LoadMap(QString)));
    connect(ui->saveMap, SIGNAL(clicked()),                    &dialogSaveMap, SLOT(open()));
    connect(&dialogSaveMap, SIGNAL(accepted()),                this, SLOT(SaveMap()));
    connect(ui->mapNew,  SIGNAL(clicked()),                    &dialogNewMap, SLOT(open()));

    connect(&dialogNewMap, SIGNAL(CreateMap()), this, SLOT(CreateMap()));

    connect(this, SIGNAL(SigDisplayError(QString,QString)),     this, SLOT(DisplayError(QString,QString)),     Qt::QueuedConnection);
    connect(this, SIGNAL(SigUpdateProgressBar(QString, float)), this, SLOT(UpdateProgressBar(QString, float)), Qt::QueuedConnection);
    save_map_use_thread = true;

    connect(&this->waypointGenerate, SIGNAL(Generate()), this, SLOT(MapObjectGenerateWaypoints()));
    connect(ui->waypointGenerateObject, SIGNAL(clicked()), &this->waypointGenerate, SLOT(show()));

    connect(ui->charsheetList, SIGNAL(currentTextChanged(QString)), ui->charsheetEditor, SLOT(Load(QString)));
    connect(ui->charsheetAdd,  SIGNAL(clicked()),                   this,                SLOT(AddCharsheet()));
    connect(ui->charsheetDel,  SIGNAL(clicked()),                   this,                SLOT(DeleteCharsheet()));
    connect(ui->charsheetSave, SIGNAL(clicked()),                   ui->charsheetEditor, SLOT(Save()));

    connect(ui->treeWidget, SIGNAL(FocusObject(MapObject*)),            this, SLOT(MapObjectFocus(MapObject*)));
    connect(ui->treeWidget, SIGNAL(FocusDynamicObject(DynamicObject*)), this, SLOT(DynamicObjectFocus(DynamicObject*)));
    connect(ui->treeWidget, SIGNAL(FocusLight(WorldLight*)),            this, SLOT(LightFocus(WorldLight*)));

    ui->treeWidget->header()->hide();
    ui->scriptList->header()->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::CurrentTabChanged(int ntab)
{
    _app.SetPandaEnabled(ntab == PANDA_TAB); // The first tab is the only one using Panda3D
}

void MainWindow::AddCharsheet()
{
  QString name = QInputDialog::getText(this, "New statistic sheet", "Name");

  if (name != "")
  {
    ui->charsheetEditor->New(name);
    ui->charsheetList->addItem(name);
    ui->charsheetDel->setEnabled(true);
  }
}

void MainWindow::DeleteCharsheet()
{
  QListWidgetItem* item = ui->charsheetList->currentItem();

  if (item)
  {
    int ret = QMessageBox::warning(this, "Deleting a stat sheet", "Are you sure you want to delete " + item->text() + " ?", QMessageBox::Yes, QMessageBox::No);

    if (ret == QMessageBox::No)
      return ;
    ui->charsheetEditor->Delete();
    ui->charsheetList->removeItemWidget(item);
    delete item;
  }
  if (ui->charsheetList->count() == 0)
    ui->charsheetDel->setEnabled(false);
}

void MainWindow::CreateMap(void)
{
    QString name = dialogNewMap.GetMapName();

    if (world)
      delete world;
    world     = new World(_window);
    save_map_use_thread = false;
    SaveMap();
    save_map_use_thread = true;
    ui->listMap->addItem(name);
    SelectableResource::MapsResource().AddResource(name);
    LoadMap(name);
}

void MainWindow::LoadProject()
{
    bool    success = true;
    QString path    = splashScreen.GetProjectPath();
    QDir    root(path);

    if (!(QDir::setCurrent(path)))
    {
        QMessageBox::warning(this, "Error", "Couldn't set the path properly");
        success = false;
    }
    else // Check Existing paths
    {
        QStringList pathToCheck;
        bool        creatingPath = false;

        pathToCheck << "maps/" << "data/" << "scripts/" << "data/l18n" << "data/dialogs";
        for (short i = 0 ; i < 3 ; ++i)
          pathToCheck << ("scripts/" + pathScriptCategories[i]);
        foreach (QString path, pathToCheck)
        {
            QDir dir(path);

            if (!(dir.exists()))
            {
                if (creatingPath == false)
                {
                    int ret = QMessageBox::warning(this, "Missing directory " + path, "This project doesn't exist yet. Do you want to create it ?", QMessageBox::Yes, QMessageBox::No);

                    if (ret == QMessageBox::No)
                    {
                      success = false;
                      break ;
                    }
                    else
                      creatingPath = true;
                }
                if (creatingPath)
                    root.mkdir(path);
            }
        }
    }

    if (success)
    {
        tabScript.LoadAllScript();
		tabL18n.LoadAllLanguages();
        tabDialog.LoadAllDialogs();
        ui->itemEditor->LoadAllItems();
        ui->worldmapEditor->Load();
        LoadAllMaps();
        LoadAllStatsheets();
    }
    else
        splashScreen.open();
}

void MainWindow::LoadAllStatsheets()
{
    QDir        dir("data/charsheets");
    QStringList fileList = dir.entryList();
    QRegExp     regexp("\.json$");

    foreach (QString string, fileList)
    {
      if ((string.contains(regexp)))
      {
        QString name = string.replace(regexp, "");

        ui->charsheetList->addItem(name);
        SelectableResource::Charsheets().AddResource(name);
      }
    }
    if (ui->charsheetList->count() == 0)
      ui->charsheetDel->setEnabled(false);
}

void MainWindow::LoadAllMaps()
{
  QDir        dir("maps/");
  QStringList fileList = dir.entryList();
  QRegExp     regexp("\.blob$");

  foreach (QString string, fileList)
  {
    if (string.contains(regexp))
    {
      QString name = string.replace(regexp, "");

      ui->listMap->addItem(name);
      SelectableResource::MapsResource().AddResource(name);
    }
  }
}

void MainWindow::FilterInit()
{
    if (ui->scriptSearch->text() == "Search...")
        ui->scriptSearch->setText("");
    /*if (ui->dialogSearch->text() == "Search...")
        ui->dialogSearch->setText("");*/
}

void MainWindow::CameraMoveBottom()
{
    my_task.camera->MoveV(-30.f);
}

void MainWindow::CameraMoveTop()
{
    my_task.camera->MoveV(30.f);
}

void MainWindow::CameraMoveLeft()
{
    my_task.camera->MoveH(-30.f);
}

void MainWindow::CameraMoveRight()
{
    my_task.camera->MoveH(30.f);
}

void MainWindow::PandaButtonPressed(QMouseEvent*)
{
  if (ui->waypointZoneSelector->isChecked() && mapobjectSelected)
  {
    LPoint3 pos = my_task.CollidingAt(mapobjectSelected);

    wp_select_x = pos.get_x();
    wp_select_y = pos.get_y();
  }
}

void MainWindow::SelectWaypointZone(float from_x, float from_y, float to_x, float to_y)
{
  if (mapobjectSelected)
  {
    auto it  = mapobjectSelected->waypoints.begin();
    auto end = mapobjectSelected->waypoints.end();

    for (; it != end ; ++it)
    {
      LPoint3 pos = (*it)->nodePath.get_pos();

      if ((!(*it)->IsSelected()) &&
          (pos.get_x() >= from_x && pos.get_x() <= to_x) &&
          (pos.get_y() >= from_y && pos.get_y() <= to_y))
      {
        (*it)->SetSelected(true);
        waypointsSelection.push_back(*it);
      }
    }
  }
}

void MainWindow::ShowWaypointZone(void)
{
  if (mapobjectSelected && !(mapobjectSelected->waypoints_root.is_hidden()))
  {
    static LPoint2f last_position    = LPoint2f(0, 0);
    static bool     needs_update     = true;
    LPoint2f        current_position = my_task.mouse->GetPosition();

    if (last_position != current_position)
      needs_update = true;
    if (needs_update == false || last_position != current_position)
    {
      last_position = current_position;
      return ;
    }
    last_position = current_position;

    LPoint3                   position       = my_task.CollidingAt(mapobjectSelected);
    PT(CollisionSphere)       collision_box  = new CollisionSphere(position, 50);
    PT(CollisionNode)         collision_node = new CollisionNode("waypoint_box");
    NodePath                  collision_path = mapobjectSelected->nodePath.attach_new_node(collision_node);
    CollisionTraverser        traverser;
    PT(CollisionHandlerQueue) collision_handler_queue = new CollisionHandlerQueue();

    collision_node->set_from_collide_mask(CollideMask(ColMask::Waypoint));
    collision_node->add_solid(collision_box);
    traverser.add_collider(collision_path, collision_handler_queue);
    traverser.traverse(mapobjectSelected->nodePath);
    std::for_each(mapobjectSelected->waypoints.begin(), mapobjectSelected->waypoints.end(), [](Waypoint* wp)
    {
      if (wp->IsSelected())
        wp->nodePath.show();
      else
        wp->nodePath.hide();
    });
    for (int i = 0 ; i < collision_handler_queue->get_num_entries() ; ++i)
    {
      CollisionEntry* entry  = collision_handler_queue->get_entry(i);
      NodePath        into   = entry->get_into_node_path();
      NodePath        parent = into.get_parent();

      while (!(parent.is_empty()) && parent != mapobjectSelected->waypoints_root)
      {
        into   = parent;
        parent = into.get_parent();
      }
      if (parent == mapobjectSelected->waypoints_root)
        into.show();
    }
    collision_path.remove_node();
    needs_update = false;
  }
}

void MainWindow::PandaButtonRelease(QMouseEvent*)
{
    waypointHovered      = 0;
    mapobjectHovered     = 0;
    dynamicObjectHovered = 0;
    my_task.mouse->GetHoveredAt(my_task.mouse->GetPosition());
    if (mapobjectSelected && !(mapobjectSelected->waypoints_root.is_hidden()))
    {
      if (ui->waypointPicker->isChecked())
      {
        my_task.mouse->GetWaypointHoveredAt(my_task.mouse->GetPosition(), mapobjectSelected->waypoints_root);
        if (waypointHovered)
          WaypointSelect(waypointHovered);
      }
      else if (ui->waypointZoneSelector->isChecked())
      {
        LPoint3 pos = my_task.CollidingAt(mapobjectSelected);

        SelectWaypointZone(std::min(pos.get_x(), wp_select_x),
                           std::min(pos.get_y(), wp_select_y),
                           std::max(pos.get_x(), wp_select_x),
                           std::max(pos.get_y(), wp_select_y));
      }
    }
    if (mapobjectHovered)
      MapObjectSelect();
    if (dynamicObjectHovered)
      DynamicObjectSelect();
}

#define MAP_TABS_OBJECTS            2
#define OBJECT_TABS_MAP_OBJECTS     0
#define OBJECT_TABS_DYNAMIC_OBJECTS 1
#define OBJECT_TABS_LIGHTS          2

void MainWindow::LightFocus(WorldLight* light)
{
  if (light)
  {
    // TODO Focus the light in the interface too
    ui->map_tabs->setCurrentIndex(MAP_TABS_OBJECTS);
    ui->object_tabs->setCurrentIndex(OBJECT_TABS_LIGHTS);
    my_task.camera->CenterCameraInstant(light->symbol.get_pos());
  }
}

void MainWindow::MapObjectFocus(MapObject* mapobject)
{
  if (mapobject)
  {
    mapobjectHovered = mapobject;
    MapObjectSelect();
    ui->map_tabs->setCurrentIndex(MAP_TABS_OBJECTS);
    ui->object_tabs->setCurrentIndex(OBJECT_TABS_MAP_OBJECTS);
    my_task.camera->CenterCameraInstant(mapobject->nodePath.get_pos());
  }
}

void MainWindow::DynamicObjectFocus(DynamicObject* dynamic_object)
{
    cout << "DynamicObjectFocus" << endl;
  if (dynamic_object)
  {
    cout << "-> Non zero pointre" << endl;
    dynamicObjectHovered = dynamic_object;
    DynamicObjectSelect();
    ui->map_tabs->setCurrentIndex(MAP_TABS_OBJECTS);
    ui->object_tabs->setCurrentIndex(OBJECT_TABS_DYNAMIC_OBJECTS);
    my_task.camera->CenterCameraInstant(dynamic_object->nodePath.get_pos());
  }
}

//Pass the locale data from the localization manager to the dialog editor
void MainWindow::SetDefaultLocalization() {
	tabDialog.LoadLocale( tabL18n.GetDefaultLanguage() );
};

void MainWindow::PandaInitialized()
{
    waypointHovered = 0;
    WindowFramework* window = ui->widget->Window();

    _window = window;

    my_task.panda_widget = ui->widget;
    my_task.main_window  = this;
    my_task.window  = window;
    my_task.camera  = new SceneCamera(window, window->get_camera_group());
    my_task.mouse   = new Mouse(window);

    connect(ui->widget, SIGNAL(MousePressed(QMouseEvent*)), this, SLOT(PandaButtonPressed(QMouseEvent*)));
    connect(ui->widget, SIGNAL(MouseRelease(QMouseEvent*)), this, SLOT(PandaButtonRelease(QMouseEvent*)));

    connect(ui->mapMoveLeft,   SIGNAL(clicked()), this, SLOT(CameraMoveLeft()));
    connect(ui->mapMoveBottom, SIGNAL(clicked()), this, SLOT(CameraMoveBottom()));
    connect(ui->mapMoveTop,    SIGNAL(clicked()), this, SLOT(CameraMoveTop()));
    connect(ui->mapMoveRight,  SIGNAL(clicked()), this, SLOT(CameraMoveRight()));

    window->enable_keyboard();

     connect(ui->itemEditor, SIGNAL(ItemListChanged(QStringList)), &dialogObject, SLOT(SetObjectList(QStringList)));

// WAYPOINTS
     connect(ui->waypointVisible,    SIGNAL(toggled(bool)),        this, SLOT(WaypointVisible()));

     connect(ui->waypointConnect,    SIGNAL(clicked()),            this, SLOT(WaypointConnect()));
     connect(ui->waypointDisconnect, SIGNAL(clicked()),            this, SLOT(WaypointDisconnect()));
     connect(ui->waypointSelX,       SIGNAL(valueChanged(double)), this, SLOT(WaypointUpdateSelX()));
     connect(ui->waypointSelY,       SIGNAL(valueChanged(double)), this, SLOT(WaypointUpdateSelY()));
     connect(ui->waypointSelZ,       SIGNAL(valueChanged(double)), this, SLOT(WaypointUpdateSelZ()));
     connect(ui->waypointSelDelete,  SIGNAL(clicked()),            this, SLOT(WaypointSelDelete()));


     connect(ui->waypointDiscardSelection, SIGNAL(clicked()), this, SLOT(WaypointDiscardSelection()));

     connect(&wizardObject, SIGNAL(accepted()), this, SLOT(ObjectAdd()));

// ENTRY/EXIT ZONES
     connect(ui->entryZoneAdd,    SIGNAL(clicked()), this, SLOT(EntryZoneAdd()));
     connect(ui->entryZoneDelete, SIGNAL(clicked()), this, SLOT(EntryZoneDelete()));
     connect(ui->entryZoneSetWaypoints, SIGNAL(clicked()), this, SLOT(EntryZoneSetAsSelection()));
     connect(ui->entryZoneList,   SIGNAL(currentIndexChanged(QString)), this, SLOT(EntryZoneChanged(QString)));

     connect(ui->exitZoneAdd,     SIGNAL(clicked()), this, SLOT(ExitZoneAdd()));
     connect(ui->exitZoneDelete,  SIGNAL(clicked()), this, SLOT(ExitZoneDelete()));
     connect(ui->exitZoneSetSelection, SIGNAL(clicked()), this, SLOT(ExitZoneSetAsSelection()));
     connect(ui->exitZoneList,    SIGNAL(currentIndexChanged(QString)), this, SLOT(ExitZoneChanged(QString)));
     connect(ui->exitZoneAddDestination, SIGNAL(clicked()), this, SLOT(ExitZoneDestinationAdd()));
     connect(ui->exitZoneDeleteDestination, SIGNAL(clicked()), this, SLOT(ExitZoneDestinationDelete()));

// MAPOBJECTS
     mapobjectSelected = 0;
     mapobjectHovered  = 0;
     connect(ui->mapNewObject, SIGNAL(clicked()), this, SLOT(MapObjectWizard()));
     //connect(ui->objectEdit, SIGNAL(clicked()), &dialogObject, SLOT(open()));
     //connect(&dialogObject, SIGNAL(accepted()), this, SLOT(EditObject()));

     connect(ui->objectRemove, SIGNAL(clicked()), this, SLOT(MapObjectDelete()));
     connect(ui->objectPosX, SIGNAL(valueChanged(double)), this, SLOT(MapObjectUpdateX()));
     connect(ui->objectPosY, SIGNAL(valueChanged(double)), this, SLOT(MapObjectUpdateY()));
     connect(ui->objectPosZ, SIGNAL(valueChanged(double)), this, SLOT(MapObjectUpdateZ()));
     connect(ui->objectRotationX, SIGNAL(valueChanged(double)), this, SLOT(MapObjectRotationX()));
     connect(ui->objectRotationY, SIGNAL(valueChanged(double)), this, SLOT(MapObjectRotationY()));
     connect(ui->objectRotationZ, SIGNAL(valueChanged(double)), this, SLOT(MapObjectRotationZ()));
     connect(ui->objectScaleX, SIGNAL(valueChanged(double)), this, SLOT(MapObjectScaleX()));
     connect(ui->objectScaleY, SIGNAL(valueChanged(double)), this, SLOT(MapObjectScaleY()));
     connect(ui->objectScaleZ, SIGNAL(valueChanged(double)), this, SLOT(MapObjectScaleZ()));
     connect(ui->objectName, SIGNAL(textChanged(QString)), this, SLOT(MapObjectNameChanged(QString)));
     connect(ui->objectFloor, SIGNAL(valueChanged(int)), this, SLOT(MapObjectFloor()));

// DYNAMICOBJECTS
     dynamicObjectSelected = 0;
     dynamicObjectHovered  = 0;
     connect(ui->interObjVisible, SIGNAL(toggled(bool)),     this, SLOT(DynamicObjectVisible()));
     connect(ui->interObjAdd,  SIGNAL(clicked()),            this, SLOT(DynamicObjectWizard()));
     connect(ui->interObjName, SIGNAL(textChanged(QString)), this, SLOT(DynamicObjectNameChanged(QString)));
     connect(ui->interObjEdit, SIGNAL(clicked()), &dialogObject,   SLOT(open()));
     connect(ui->interObjRemove, SIGNAL(clicked()),          this, SLOT(DynamicObjectDelete()));
     connect(ui->interObjWaypoint, SIGNAL(clicked()),        this, SLOT(DynamicObjectSetWaypoint()));

     connect(ui->interObjPosX, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectUpdateX()));
     connect(ui->interObjPosY, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectUpdateY()));
     connect(ui->interObjPosZ, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectUpdateZ()));
     connect(ui->interObjRotX, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectRotationX()));
     connect(ui->interObjRotY, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectRotationY()));
     connect(ui->interObjRotZ, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectRotationZ()));
     connect(ui->interObjScaleX, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectScaleX()));
     connect(ui->interObjScaleY, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectScaleY()));
     connect(ui->interObjScaleZ, SIGNAL(valueChanged(double)), this, SLOT(DynamicObjectScaleZ()));

// LIGHTS
     lightSelected      = 0;
     lightIgnoreChanges = false;
     connect(ui->lightsAdd,         SIGNAL(clicked()),                this, SLOT(LightAdd()));
     connect(ui->lightsDel,         SIGNAL(clicked()),                this, SLOT(LightDelete()));
     connect(ui->lightCompile,      SIGNAL(clicked()),                this, SLOT(LightCompile()));
     connect(ui->lightsSelect,      SIGNAL(currentIndexChanged(int)), this, SLOT(LightSelected()));
     connect(ui->lightTypesList,    SIGNAL(currentIndexChanged(int)), this, SLOT(LightUpdateType()));
     connect(ui->lightColorR,       SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightColorG,       SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightColorB,       SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightColorA,       SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightPosX,         SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightPosY,         SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightPosZ,         SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightRotX,         SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightRotY,         SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightRotZ,         SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightAttenuationA, SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightAttenuationB, SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightAttenuationC, SIGNAL(valueChanged(double)),     this, SLOT(LightUpdatePosition()));
     connect(ui->lightsVisible,     SIGNAL(toggled(bool)),            this, SLOT(LightVisible()));
     connect(ui->lightSetEnabled,   SIGNAL(toggled(bool)),            this, SLOT(LightSetEnabled()));
     connect(ui->lightSetDisabled,  SIGNAL(toggled(bool)),            this, SLOT(LightSetDisabled()));

     connect(my_task.mouse,      SIGNAL(WaypointHovered(NodePath)), this, SLOT(WaypointHovered(NodePath)));
     connect(my_task.mouse,      SIGNAL(ObjectHovered(NodePath)),   this, SLOT(MapObjectHovered(NodePath)));
     connect(my_task.mouse,      SIGNAL(UnitHovered(NodePath)),     this, SLOT(DynamicObjectHovered(NodePath)));

     waypointSelected = 0;
     waypointHovered  = 0;

    my_task.timer.start();
    AsyncTaskManager::get_global_ptr()->add(&my_task);

    wizardDynObject = false;
    wizardMapObject = false;
    {
      QString level = ui->listMap->currentText();

      if (level != "")
        LoadMap(level);
    }
}

void MainWindow::ObjectAdd()
{
    if (wizardDynObject)
      DynamicObjectAdd();
    else if (wizardMapObject)
      MapObjectAdd();
}

void MainWindow::DynamicObjectSetWaypoint()
{
    if (dynamicObjectSelected)
      world->DynamicObjectSetWaypoint(*dynamicObjectSelected, *waypointSelected);
}

void MainWindow::DynamicObjectWizard()
{
    wizardDynObject = true;
    wizardObject.open();
}

void MainWindow::DynamicObjectAdd()
{
    QString name = wizardObject.GetName();
    QString mod  = wizardObject.GetModel();
    QString tex  = wizardObject.GetTexture();
    float   posx = wizardObject.GetPosX();
    float   posy = wizardObject.GetPosY();
    float   posz = wizardObject.GetPosZ();
    float   scale = wizardObject.GetScale();

    dynamicObjectHovered = world->AddDynamicObject(name.toStdString(), DynamicObject::Character, mod.toStdString(), tex.toStdString());
    dynamicObjectHovered->nodePath.set_pos(posx, posy, posz);
    dynamicObjectHovered->nodePath.set_scale(scale);
    DynamicObjectSelect();
}

void MainWindow::DynamicObjectDelete()
{
    if (dynamicObjectSelected)
    {
      DynamicObject* toDel = dynamicObjectSelected;

      dynamicObjectHovered  = 0;
      DynamicObjectSelect();
      world->DeleteDynamicObject(toDel);
    }
}

void MainWindow::DynamicObjectHovered(NodePath np)
{
    if (ui->interObjVisible->isChecked())
    {
      dynamicObjectHovered = world->GetDynamicObjectFromNodePath(np);
    }
}

void MainWindow::DynamicObjectSelect()
{
    ui->interObjEditor->setEnabled(dynamicObjectHovered != 0);
    dynamicObjectSelected = dynamicObjectHovered;
    if (dynamicObjectHovered)
    {
        LVecBase3 pos, hpr, scale;
        NodePath  np = dynamicObjectHovered->nodePath;

        pos   = np.get_pos();
        hpr   = np.get_hpr();
        scale = np.get_scale();
        ui->interObjName->setText(QString::fromStdString(np.get_name()));
        ui->interObjPosX->setValue(pos.get_x());
        ui->interObjPosY->setValue(pos.get_y());
        ui->interObjPosZ->setValue(pos.get_z());
        ui->interObjRotX->setValue(hpr.get_x());
        ui->interObjRotY->setValue(hpr.get_y());
        ui->interObjRotZ->setValue(hpr.get_z());
        ui->interObjScaleX->setValue(scale.get_x());
        ui->interObjScaleY->setValue(scale.get_y());
        ui->interObjScaleZ->setValue(scale.get_z());
        dialogObject.SetCurrentObject(dynamicObjectSelected);
        ui->interObjEditor->setEnabled(true);
    }
    else
        ui->interObjEditor->setEnabled(false);
}

void MainWindow::DynamicObjectNameChanged(QString name)
{
    if (dynamicObjectSelected)
      dynamicObjectSelected->nodePath.set_name(name.toStdString());
}

void MainWindow::DynamicObjectUpdateX()
{
    if (dynamicObjectSelected)
    {
        NodePath  np  = dynamicObjectSelected->nodePath;
        LVecBase3 pos = np.get_pos();

        pos.set_x(ui->interObjPosX->value());
        np.set_pos(pos);
    }
}

void MainWindow::DynamicObjectUpdateY()
{
    if (dynamicObjectSelected)
    {
        NodePath  np  = dynamicObjectSelected->nodePath;
        LVecBase3 pos = np.get_pos();

        pos.set_y(ui->interObjPosY->value());
        np.set_pos(pos);
    }
}

void MainWindow::DynamicObjectUpdateZ()
{
    if (dynamicObjectSelected)
    {
        NodePath  np  = dynamicObjectSelected->nodePath;
        LVecBase3 pos = np.get_pos();

        pos.set_z(ui->interObjPosZ->value());
        np.set_pos(pos);
    }
}

void MainWindow::DynamicObjectRotationX()
{
    if (dynamicObjectSelected)
    {
        NodePath  np  = dynamicObjectSelected->nodePath;
        LVecBase3 hpr = np.get_hpr();

        hpr.set_x(ui->interObjRotX->value());
        np.set_hpr(hpr);
    }
}

void MainWindow::DynamicObjectRotationY()
{
    if (dynamicObjectSelected)
    {
        NodePath  np  = dynamicObjectSelected->nodePath;
        LVecBase3 hpr = np.get_hpr();

        hpr.set_y(ui->interObjRotY->value());
        np.set_hpr(hpr);
    }
}

void MainWindow::DynamicObjectRotationZ()
{
    if (dynamicObjectSelected)
    {
        NodePath  np  = dynamicObjectSelected->nodePath;
        LVecBase3 hpr = np.get_hpr();

        hpr.set_z(ui->interObjRotZ->value());
        np.set_hpr(hpr);
    }
}

void MainWindow::DynamicObjectScaleX()
{
    if (dynamicObjectSelected)
    {
        NodePath  np    = dynamicObjectSelected->nodePath;
        LVecBase3 scale = np.get_scale();

        scale.set_x(ui->interObjScaleX->value());
        np.set_scale(scale);
    }
}

void MainWindow::DynamicObjectScaleY()
{
    if (dynamicObjectSelected)
    {
        NodePath  np    = dynamicObjectSelected->nodePath;
        LVecBase3 scale = np.get_scale();

        scale.set_y(ui->interObjScaleY->value());
        np.set_scale(scale);
    }
}

void MainWindow::DynamicObjectScaleZ()
{
    if (dynamicObjectSelected)
    {
        NodePath  np    = dynamicObjectSelected->nodePath;
        LVecBase3 scale = np.get_scale();

        scale.set_z(ui->interObjScaleZ->value());
        np.set_scale(scale);
    }
}

void MainWindow::DynamicObjectVisible()
{
    world->SetDynamicObjectsVisible(ui->interObjVisible->isChecked());
}

void MainWindow::MapObjectWizard()
{
    std::cout << "MAP OBJECT WIZARD" << std::endl;
    wizardMapObject = true;
    wizardObject.open();
}

void MainWindow::MapObjectAdd()
{
    wizardMapObject = false;
    QString name  = wizardObject.GetName();
    QString model = wizardObject.GetModel();
    QString text  = wizardObject.GetTexture();
    float   posx  = wizardObject.GetPosX();
    float   posy  = wizardObject.GetPosY();
    float   posz  = wizardObject.GetPosZ();
    float   scale = wizardObject.GetScale();

    mapobjectHovered = world->AddMapObject(name.toStdString(), model.toStdString(), text.toStdString(), posx, posy, posz);
    mapobjectHovered->nodePath.set_scale(scale);
    mapobjectHovered->nodePath.show();
    MapObjectSelect();
}

void MainWindow::MapObjectDelete()
{
    if (mapobjectSelected)
    {
      MapObject* toDel = mapobjectSelected;

      mapobjectHovered  = 0;
      MapObjectSelect();
      world->DeleteMapObject(toDel);
    }
}

void MainWindow::MapObjectHovered(NodePath path)
{
  mapobjectHovered     = world->GetMapObjectFromNodePath(path);
}

void MainWindow::MapObjectGenerateWaypoints(void)
{
  if (mapobjectSelected)
  {
    wp_generator = new WaypointGenerator(world,
                                         mapobjectSelected,
                                         waypointGenerate.GetMargin(),
                                         waypointGenerate.GetSpacing());
    connect(wp_generator, SIGNAL(Started()),                     this,            SLOT(DisableLevelEditor()),             Qt::QueuedConnection);
    connect(wp_generator, SIGNAL(Started()),                     ui->progressBar, SLOT(show()),                           Qt::QueuedConnection);
    connect(wp_generator, SIGNAL(UpdateProgress(QString,float)), this,            SLOT(UpdateProgressBar(QString,float)), Qt::QueuedConnection);
    connect(wp_generator, SIGNAL(Done()),                        this,            SLOT(EnableLevelEditor()),              Qt::QueuedConnection);
    connect(wp_generator, SIGNAL(Done()),                        ui->progressBar, SLOT(hide()),                           Qt::QueuedConnection);

    wp_generator->Run();
  }
}

void MainWindow::MapObjectFloor()
{
    if (mapobjectSelected)
      world->MapObjectChangeFloor(*mapobjectSelected, ui->objectFloor->value());
}

void MainWindow::MapObjectNameChanged(QString name)
{
    if (mapobjectSelected)
      mapobjectSelected->nodePath.set_name(name.toStdString());
}

void MainWindow::MapObjectSelect()
{
    mapobjectSelected = mapobjectHovered;
    if (mapobjectSelected)
    {
      ui->waypointVisible->setChecked(!(mapobjectSelected->waypoints_root.is_hidden()));
      ui->objectFloor->setValue(mapobjectSelected->floor);
      ui->objectScaleX->setValue(mapobjectSelected->nodePath.get_scale().get_x());
      ui->objectScaleY->setValue(mapobjectSelected->nodePath.get_scale().get_y());
      ui->objectScaleZ->setValue(mapobjectSelected->nodePath.get_scale().get_z());
      ui->objectPosX->setValue(mapobjectSelected->nodePath.get_x());
      ui->objectPosY->setValue(mapobjectSelected->nodePath.get_y());
      ui->objectPosZ->setValue(mapobjectSelected->nodePath.get_z());
      ui->objectRotationX->setValue(mapobjectSelected->nodePath.get_hpr().get_x());
      ui->objectRotationY->setValue(mapobjectSelected->nodePath.get_hpr().get_y());
      ui->objectRotationZ->setValue(mapobjectSelected->nodePath.get_hpr().get_z());
      ui->objectName->setText(QString::fromStdString(mapobjectSelected->nodePath.get_name()));
      ui->objectEditor->setEnabled(true);
      ui->objectName->setEnabled(true);
    }
    else
      ui->objectEditor->setEnabled(false);
}

void MainWindow::MapObjectUpdateX()
{
    if (mapobjectSelected)
      mapobjectSelected->nodePath.set_x(ui->objectPosX->value());
}

void MainWindow::MapObjectUpdateY()
{
    if (mapobjectSelected)
      mapobjectSelected->nodePath.set_y(ui->objectPosY->value());
}

void MainWindow::MapObjectUpdateZ()
{
    if (mapobjectSelected)
      mapobjectSelected->nodePath.set_z(ui->objectPosZ->value());
}

void MainWindow::MapObjectRotationX()
{
    if (mapobjectSelected)
    {
      LVecBase3 hpr = mapobjectSelected->nodePath.get_hpr();

      hpr.set_x(ui->objectRotationX->value());
      mapobjectSelected->nodePath.set_hpr(hpr);
    }
}

void MainWindow::MapObjectRotationY()
{
    if (mapobjectSelected)
    {
      LVecBase3 hpr = mapobjectSelected->nodePath.get_hpr();

      hpr.set_y(ui->objectRotationY->value());
      mapobjectSelected->nodePath.set_hpr(hpr);
    }
}

void MainWindow::MapObjectRotationZ()
{
    if (mapobjectSelected)
    {
      LVecBase3 hpr = mapobjectSelected->nodePath.get_hpr();

      hpr.set_z(ui->objectRotationZ->value());
      mapobjectSelected->nodePath.set_hpr(hpr);
    }
}

void MainWindow::MapObjectScaleX()
{
    if (mapobjectSelected)
    {
      LVecBase3 scale = mapobjectSelected->nodePath.get_scale();

      scale.set_x(ui->objectScaleX->value());
      mapobjectSelected->nodePath.set_scale(scale);
    }
}

void MainWindow::MapObjectScaleY()
{
    if (mapobjectSelected)
    {
      LVecBase3 scale = mapobjectSelected->nodePath.get_scale();

      scale.set_y(ui->objectScaleY->value());
      mapobjectSelected->nodePath.set_scale(scale);
    }
}

void MainWindow::MapObjectScaleZ()
{
    if (mapobjectSelected)
    {
      LVecBase3 scale = mapobjectSelected->nodePath.get_scale();

      scale.set_z(ui->objectScaleZ->value());
      mapobjectSelected->nodePath.set_scale(scale);
    }
}

void MainWindow::WaypointConnect()
{
    std::list<Waypoint*>::iterator it  = waypointsSelection.begin();
    std::list<Waypoint*>::iterator end = waypointsSelection.end();

    for (; it != end ; ++it)
    {
      std::list<Waypoint*>::iterator subIt  = waypointsSelection.begin();
      std::list<Waypoint*>::iterator subEnd = waypointsSelection.end();

      for (; subIt != subEnd ; ++subIt)
      {
        if (it == subIt)
          continue ;
        (*it)->Connect(*subIt);
      }
    }
}

void MainWindow::WaypointDisconnect()
{
    std::list<Waypoint*>::iterator it  = waypointsSelection.begin();
    std::list<Waypoint*>::iterator end = waypointsSelection.end();

    for (; it != end ; ++it)
    {
      std::list<Waypoint*>::iterator subIt  = waypointsSelection.begin();
      std::list<Waypoint*>::iterator subEnd = waypointsSelection.end();

      for (; subIt != subEnd ; ++subIt)
      {
        if (it == subIt)
          continue ;
        (*it)->Disconnect(*subIt);
      }
    }
}

void MainWindow::WaypointHovered(NodePath path)
{
    if (ui->waypointVisible->isChecked())
    {
      waypointHovered      = world->GetWaypointFromNodePath(path);
    }
}

void MainWindow::WaypointVisible()
{
  if (mapobjectSelected)
  {
    if (ui->waypointVisible->isChecked())
      mapobjectSelected->waypoints_root.show();
    else
      mapobjectSelected->waypoints_root.hide();
  }
}

void MainWindow::UpdateSelection()
{
    QString str;
    bool    enabled;

    str  = QString::number(waypointsSelection.size());
    str += " waypoints selected";
    ui->waypointSelection->setText(str);
    enabled = waypointsSelection.size() > 1;
    ui->waypointEditor->setEnabled(enabled);

    waypointSelX = waypointSelY = waypointSelZ = 0;
    ui->waypointSelX->setValue(waypointSelX);
    ui->waypointSelY->setValue(waypointSelY);
    ui->waypointSelZ->setValue(waypointSelZ);
}

void MainWindow::WaypointSelDelete()
{
    std::list<Waypoint*>::iterator it  = waypointsSelection.begin();
    std::list<Waypoint*>::iterator end = waypointsSelection.end();

    for (; it != end ; ++it)
      world->DeleteWayPoint(*it);
    waypointsSelection.clear();
    UpdateSelection();
}

void MainWindow::WaypointSelect(Waypoint* waypoint)
{
    std::list<Waypoint*>::iterator it = std::find(waypointsSelection.begin(), waypointsSelection.end(), waypoint);

    if (it != waypointsSelection.end())
    {
      (*it)->SetSelected(false);
      waypointsSelection.erase(it);
      if (waypoint == waypointSelected)
      {
          if (waypointsSelection.size() > 0)
            waypointSelected = (*(--(waypointsSelection.end())));
          else
            waypointSelected = 0;
      }
    }
    else
    {
      waypoint->SetSelected(true);
      waypointSelected = waypoint;
      waypointsSelection.push_back(waypoint);
    }
    UpdateSelection();
}

void MainWindow::WaypointUpdateSelX()
{
    std::list<Waypoint*>::iterator it;
    std::list<Waypoint*>::iterator end    = waypointsSelection.end();
    float                          toMove = ui->waypointSelX->value() - waypointSelX;

    for (it = waypointsSelection.begin() ; it != end ; ++it)
    {
      float posx = (*it)->nodePath.get_x();

      (*it)->nodePath.set_x(posx + toMove);
    }
    for (it = waypointsSelection.begin() ; it != end ; ++it)
      (*it)->PositionChanged();

    waypointSelX = ui->waypointSelX->value();
}

void MainWindow::WaypointUpdateSelY()
{
    std::list<Waypoint*>::iterator it;
    std::list<Waypoint*>::iterator end    = waypointsSelection.end();
    float                          toMove = ui->waypointSelY->value() - waypointSelY;

    for (it = waypointsSelection.begin() ; it != end ; ++it)
    {
      float posy = (*it)->nodePath.get_y();

      (*it)->nodePath.set_y(posy + toMove);
    }
    for (it = waypointsSelection.begin() ; it != end ; ++it)
      (*it)->PositionChanged();

    waypointSelY = ui->waypointSelY->value();
}

void MainWindow::WaypointUpdateSelZ()
{
    std::list<Waypoint*>::iterator it;
    std::list<Waypoint*>::iterator end    = waypointsSelection.end();
    float                          toMove = ui->waypointSelZ->value() - waypointSelZ;

    for (it = waypointsSelection.begin() ; it != end ; ++it)
    {
      float posz = (*it)->nodePath.get_z();

      (*it)->nodePath.set_z(posz + toMove);
    }
    for (it = waypointsSelection.begin() ; it != end ; ++it)
      (*it)->PositionChanged();

    waypointSelZ = ui->waypointSelZ->value();
}

void MainWindow::WaypointDiscardSelection(void)
{
    while (waypointsSelection.size())
      WaypointSelect(waypointsSelection.front());
}

#include <panda3d/collisionRay.h>
void MainWindow::WaypointSyncTerrain(void)
{
  ui->tabLevelDesigner->setEnabled(false);
  ui->progressBar->show();

  FunctorThread& thread = *FunctorThread::Create([this](void)
  {
    unsigned short                 attempt = 0;
    float                          i       = 0;
    std::list<Waypoint*>::iterator it;

    for (it = waypointsSelection.begin() ; it != waypointsSelection.end() ; ++it, ++i)
    {
      NodePath wp = (*it)->nodePath;
      LPoint3  min_pos;
      float    new_height = wp.get_z();

      CollisionTraverser col_traverser;
      PT(CollisionHandlerQueue) col_queue = new CollisionHandlerQueue;

      PT(CollisionNode) cnode = new CollisionNode("waypointSyncTerrainNode");
      cnode->set_from_collide_mask(CollideMask(ColMask::Object));

      PT(CollisionSegment) segment = new CollisionSegment;
      cnode->add_solid(segment);

      NodePath np = world->window->get_render().attach_new_node(cnode);

      segment->set_point_a(wp.get_x(), wp.get_y(), wp.get_z());
      segment->set_point_b(wp.get_x(), wp.get_y(), wp.get_z() - 100000.f);

      cout << "Looking for collisions" << endl;
      col_traverser.add_collider(np, col_queue);
      col_traverser.traverse(world->window->get_render());
      if (col_queue->get_num_entries())
      {
        col_queue->sort_entries();
        min_pos = col_queue->get_entry(0)->get_surface_point(world->window->get_render());
        std::cout << "Found a collision" << std::endl;
        std::cout << min_pos.get_x() << ", " << min_pos.get_y() << ", " << min_pos.get_z() << std::endl;

        // ALMOST DONE !
        LPoint3 min_point, max_point;
        wp.calc_tight_bounds(min_point, max_point);
        float height = max_point.get_y() - min_point.get_y();

        new_height = min_pos.get_z() + (height / 2);
      }
      if (new_height != wp.get_z()) // Success
      {
        wp.set_z(new_height);
        attempt = 0;
      }
      else if (attempt < 20) // Failure: let's try with a slightly different position
      {
        if (attempt % 2)
          wp.set_x(wp.get_x() + 0.01);
        else
          wp.set_y(wp.get_y() + 0.01);
        --it; --i;
        ++attempt;
      }
      else // Failure: 10 consecutive failures, let's give up
        attempt = 0;
      np.show();
      np.remove_node();
      SigUpdateProgressBar("Waypoints Level Terrain: %p%", i / waypointsSelection.size() * 100.f);
    }
  });
  connect(&thread, SIGNAL(Done()), this,            SLOT(EnableLevelEditor()), Qt::QueuedConnection);
  connect(&thread, SIGNAL(Done()), ui->progressBar, SLOT(hide()),              Qt::QueuedConnection);
  thread.start();
}

void MainWindow::MapFocused(void)
{
    LoadMap(ui->listMap->currentText());
    disconnect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(MapFocused()));
}

void MainWindow::LoadMap(const QString& path)
{
    if (ui->tabWidget->currentIndex() != PANDA_TAB)
    {
      connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(MapFocused()));
      return ;
    }
    if (my_task.camera == 0 || (world != 0 && levelName == path))
      return ;
    my_task.camera->SetPosition(0, 0, 75);
    levelName             = path;
    mapobjectSelected     = 0;
    mapobjectHovered      = 0;
    dynamicObjectSelected = 0;
    dynamicObjectHovered  = 0;
    waypointsSelection.clear();

    FunctorThread&  thread   = *FunctorThread::Create([this](void)
    {
      std::string   fullpath = (QDir::currentPath() + "/maps/" + levelName + ".blob").toStdString();
      std::ifstream file;

      file.open(fullpath.c_str(),ios::binary);
      if (file.is_open())
      {
        try
        {
          Utils::Packet* packet;
          long           begin, end;
          long           size;
          char*          raw;

          begin     = file.tellg();
          file.seekg (0, ios::end);
          end       = file.tellg();
          file.seekg(0, ios::beg);
          size      = end - begin;
          raw       = new char[size + 1];
          file.read(raw, size);
          file.close();
          raw[size] = 0;

          packet = new Utils::Packet(raw, size);
          if (world)
            delete world;
          world = new World(_window);
          world->UnSerialize(*packet);
          std::cout << "World unserialized" << std::endl;

          delete   packet;
          delete[] raw;
        }
        catch (unsigned int error)
        {
          QMessageBox::warning(this, "Fatal Error", "Map file is corrupted. You are sooooo screwed.");
        }
      }
      else
        QMessageBox::warning(this, "Fatal Error", "Can't load map file '" + QString::fromStdString(fullpath) + "'");

      std::cout << "Post loading map" << std::endl;
      if (world)
      {
        ui->treeWidget->SetWorld(world);
        dialogSaveMap.SetEnabledSunlight(world->sunlight_enabled);

        ui->entryZoneList->clear();
        ForEach(world->entryZones, [this](EntryZone& zone) { ui->entryZoneList->addItem(zone.name.c_str()); });

        ui->exitZoneList->clear();
        ForEach(world->exitZones,  [this](ExitZone& zone)  { ui->exitZoneList->addItem(zone.name.c_str());  });

        ui->lightsSelect->clear();
        ui->lightFrame->setEnabled(false);
        ForEach(world->lights,     [this](WorldLight& l)   { ui->lightsSelect->addItem(l.name.c_str());     });

        for (int i = 0 ; i < ui->listMap->count() ; ++i)
        {
          if (ui->listMap->itemText(i) == levelName)
          {
            ui->listMap->setCurrentIndex(i);
            break ;
          }
        }

        dialogObject.SetWorld(world);
        std::cout << "Finished Loading Map" << std::endl;
      }
    });

    QPandaApplication::SetPandaEnabled(false);
    DisableLevelEditor();
    connect(&thread, SIGNAL(Done()), ui->progressBar, SLOT(hide()),              Qt::QueuedConnection);
    connect(&thread, SIGNAL(Done()), this,            SLOT(EnableLevelEditor()), Qt::QueuedConnection);
    thread.start_sync();
    //thread.start();
}

void MainWindow::EntryZoneAdd()
{
    QString name = QInputDialog::getText(this, "Create Entry Zone", "Name");

    if (name != "")
    {
        ui->entryZoneList->addItem(name);
        world->AddEntryZone(name.toStdString());
    }
}

void MainWindow::EntryZoneDelete()
{
    QString name = ui->entryZoneList->currentText();

    if (name != "")
    {
        ui->entryZoneList->removeItem(ui->entryZoneList->currentIndex());
        world->DeleteEntryZone(name.toStdString());
    }
}

void MainWindow::EntryZoneSetAsSelection()
{
    QString name = ui->entryZoneList->currentText();

    if (name != "")
    {
        EntryZone* entryZone = world->GetEntryZoneByName(name.toStdString());

        if (entryZone)
            entryZone->waypoints = waypointsSelection;
    }
}

void MainWindow::ExitZoneAdd()
{
    QString name = QInputDialog::getText(this, "Create Exit Zone", "Name");

    if (name != "")
    {
        world->AddExitZone(name.toStdString());
        ui->exitZoneList->addItem(name);
    }
}

void MainWindow::ExitZoneDelete()
{
    QString name = ui->exitZoneList->currentText();

    if (name != "")
    {
        ui->exitZoneList->removeItem(ui->exitZoneList->currentIndex());
        world->DeleteExitZone(name.toStdString());
    }
}

void MainWindow::ExitZoneSetAsSelection()
{
    QString name = ui->exitZoneList->currentText();

    if (name != "")
    {
        ExitZone* exitZone = world->GetExitZoneByName(name.toStdString());

        if (exitZone)
            exitZone->waypoints = waypointsSelection;
    }
}

void MainWindow::EntryZoneChanged(QString string)
{
    if (string != "")
    {
        EntryZone* zone = world->GetEntryZoneByName(string.toStdString());

        if (zone)
        {
            std::list<Waypoint*> tmp = waypointsSelection;
            ForEach(tmp,             [this](Waypoint* wp) { this->WaypointSelect(wp); });
            ForEach(zone->waypoints, [this](Waypoint* wp) { this->WaypointSelect(wp); });
        }
    }
}

void MainWindow::ExitZoneChanged(QString string)
{
    if (string != "")
    {
        ExitZone* zone = world->GetExitZoneByName(string.toStdString());

        if (zone)
        {
            ui->exitZoneDestinations->clear();
            ForEach(zone->destinations, [this](std::string name) { ui->exitZoneDestinations->addItem(name.c_str()); });

            std::list<Waypoint*> tmp = waypointsSelection;
            ForEach(tmp,             [this](Waypoint* wp) { this->WaypointSelect(wp); });
            ForEach(zone->waypoints, [this](Waypoint* wp) { this->WaypointSelect(wp); });
        }
    }
}

void MainWindow::LightCompile(void)
{
    if (lightSelected != 0)
      world->CompileLight(lightSelected);
}

void MainWindow::LightVisible(void)
{
    if (ui->lightsVisible->isChecked())
      world->lightSymbols.show();
    else
      world->lightSymbols.hide();
}

void MainWindow::LightSetDisabled()
{
  if (ui->lightSetDisabled->isChecked())
  {
    ui->lightSetEnabled->setChecked(false);
    if (lightSelected)
      lightSelected->SetEnabled(false);
  }
}

void MainWindow::LightSetEnabled()
{
  if (ui->lightSetEnabled->isChecked())
  {
    ui->lightSetDisabled->setChecked(false);
    if (lightSelected)
      lightSelected->SetEnabled(true);
  }
}

void MainWindow::LightSelected(void)
{
    QString     name  = ui->lightsSelect->currentText();
    WorldLight* light = world->GetLightByName(name.toStdString());

    if (light)
    {
        NodePath np = light->nodePath;

        lightIgnoreChanges = true;
        ui->lightTypesList->setCurrentIndex((int)light->type);
        ui->lightPosX->setValue(np.get_x());
        ui->lightPosY->setValue(np.get_y());
        ui->lightPosZ->setValue(np.get_z());
        ui->lightRotX->setValue(np.get_hpr().get_x());
        ui->lightRotY->setValue(np.get_hpr().get_y());
        ui->lightRotZ->setValue(np.get_hpr().get_z());

        LColor color = light->GetColor();
        ui->lightColorR->setValue(color.get_x());
        ui->lightColorG->setValue(color.get_y());
        ui->lightColorB->setValue(color.get_z());
        ui->lightColorA->setValue(0.f);

        if (light->type == WorldLight::Point || light->type == WorldLight::Spot)
        {
          ui->lightAttenuationA->setValue(light->GetAttenuation().get_x());
          ui->lightAttenuationB->setValue(light->GetAttenuation().get_y());
          ui->lightAttenuationC->setValue(light->GetAttenuation().get_z());
          ui->lightAttenuation->show();
        }
        else
          ui->lightAttenuation->hide();

        if (light->type == WorldLight::Point || light->type == WorldLight::Ambient)
          ui->lightRot->hide();
        else
          ui->lightRot->show();

        ui->lightRadius->setValue(light->zoneSize);

        lightIgnoreChanges = false;
        ui->lightFrame->setEnabled(true);
    }
    else
        ui->lightFrame->setEnabled(false);
    lightSelected = light;
}

void MainWindow::LightUpdateType(void)
{
    WorldLight::Type type = (WorldLight::Type)ui->lightTypesList->currentIndex();
    QString          name = ui->lightsSelect->currentText();

    if (lightIgnoreChanges || lightSelected == 0) return ;
    if (lightSelected->type != type)
    {
        world->DeleteLight(lightSelected->name);
        world->AddLight(type, name.toStdString());
        lightSelected = world->GetLightByName(name.toStdString());
        LightUpdatePosition();
        ui->lightAttenuation->setVisible(lightSelected->type == WorldLight::Point || lightSelected->type == WorldLight::Spot);
        ui->lightRot->setVisible(!(lightSelected->type == WorldLight::Point || lightSelected->type == WorldLight::Ambient));
    }
}

void MainWindow::LightUpdatePosition(void)
{
    NodePath np;

    if (lightIgnoreChanges || lightSelected == 0) return ;
    np = lightSelected->nodePath;
    LPoint3 pos(ui->lightPosX->value(), ui->lightPosY->value(), ui->lightPosZ->value());
    lightSelected->SetPosition(pos);
    np.set_hpr(ui->lightRotX->value(),
               ui->lightRotY->value(),
               ui->lightRotZ->value());
    lightSelected->SetColor(ui->lightColorR->value(),
                            ui->lightColorG->value(),
                            ui->lightColorB->value(),
                            ui->lightColorA->value());
    lightSelected->SetAttenuation(ui->lightAttenuationA->value(),
                                  ui->lightAttenuationB->value(),
                                  ui->lightAttenuationC->value());
    lightSelected->zoneSize = ui->lightRadius->value();
}

void MainWindow::LightAdd(void)
{
    QString name = QInputDialog::getText(this, "Add light", "Light name");

    if (name != "")
    {
        world->AddLight(WorldLight::Directional, name.toStdString());
        ui->lightsSelect->addItem(name);
        ui->lightsSelect->setCurrentIndex(ui->lightsSelect->count() - 1);
    }
}

void MainWindow::LightDelete(void)
{
    if (lightSelected)
    {
        int index = ui->lightsSelect->currentIndex();

        world->DeleteLight(lightSelected->name);
        lightSelected = 0;
        ui->lightsSelect->removeItem(index);
    }
}

void MainWindow::ExitZoneDestinationAdd()
{
    QString   name = ui->exitZoneList->currentText();
    ExitZone* zone = world->GetExitZoneByName(name.toStdString());

    if (zone)
    {
        QString newDest = QInputDialog::getText(this, "Add destination", "Map name");

        zone->destinations.push_back(newDest.toStdString());
        ui->exitZoneDestinations->addItem(newDest);
    }
}

void MainWindow::ExitZoneDestinationDelete()
{
    QString   name = ui->exitZoneList->currentText();
    ExitZone* zone = world->GetExitZoneByName(name.toStdString());

    if (zone)
    {
        QListWidgetItem* item = ui->exitZoneDestinations->currentItem();

        if (item)
        {
            QString destName = item->text();

            zone->destinations.remove(destName.toStdString());
            ui->exitZoneDestinations->removeItemWidget(item);
            delete item;
        }
    }
}

void MainWindow::EnableLevelEditor(void)
{
  if (level_editor_lock > 0)
    level_editor_lock--;
  if (level_editor_lock == 0)
  {
    ui->tabLevelDesigner->setEnabled(true);
    QPandaApplication::SetPandaEnabled(true);
  }
}

void MainWindow::DisableLevelEditor(void)
{
    if (level_editor_lock == 0)
      ui->tabLevelDesigner->setEnabled(false);
    level_editor_lock++;
}

void MainWindow::UpdateProgressBar(QString fmt, float percentage)
{
    ui->progressBar->setFormat(fmt);
    ui->progressBar->setValue(percentage);
}

void MainWindow::DisplayError(QString title, QString message)
{
  QMessageBox::warning(this, title, message);
}

void MainWindow::SaveMap()
{
  if (world)
  {
    ui->tabLevelDesigner->setEnabled(false);
    world->do_compile_doors     = dialogSaveMap.DoCompileDoors();
    world->do_compile_waypoints = dialogSaveMap.DoCompileWaypoints();
    world->sunlight_enabled     = dialogSaveMap.DoEnableSunlight();

    FunctorThread& thread = *FunctorThread::Create([this, &thread](void)
    {
      std::ofstream file;
      std::string   path = (QDir::currentPath() + "/maps/" + levelName + ".blob").toStdString();

      file.open(path.c_str(),ios::binary);
      if (world)
      {
        Utils::Packet packet;

        world->Serialize(packet, [this, &thread](const std::string& label, float percentage)
        {
          QString format = QString::fromStdString(label) + "%p%";

          SigUpdateProgressBar(format, percentage);
        });
        packet.PrintContent();
        file.write(packet.raw(), packet.size());
        file.close();
      }
      else
        SigDisplayError("Fatal Error", "Cannot save map");
    });
    ui->progressBar->show();
    connect(&thread, SIGNAL(Done()), ui->progressBar, SLOT(hide()), Qt::QueuedConnection);
    connect(&thread, SIGNAL(Done()), this, SLOT(EnableLevelEditor()), Qt::QueuedConnection);
    if (save_map_use_thread)
      thread.start();
    else
      thread.start_sync();
  }
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    QMainWindow::closeEvent(e);
    emit Closed();
}
