#ifndef  MOUSE_H
# define MOUSE_H

# include <panda3d/pandaFramework.h>
# include <panda3d/pandaSystem.h>
# include <panda3d/mouseWatcher.h>
# include <panda3d/collisionRay.h>
# include <panda3d/collisionHandlerQueue.h>
# include <panda3d/collisionTraverser.h>
# include <QObject>

class Mouse : public QObject
{
    Q_OBJECT
public:
    explicit Mouse(WindowFramework* window, QObject *parent = 0);

    void                      Run(void);
    LPoint2f                  GetPosition(void) const;

signals:
    void WaypointHovered(NodePath);
    void UnitHovered(NodePath);
    void ObjectHovered(NodePath);

  private:
    WindowFramework*          _window;
    NodePath                  _camera;
    PT(MouseWatcher)          _mouseWatcher;
    PT(CollisionRay)          _pickerRay;
    PT(CollisionNode)         _pickerNode;
    NodePath                  _pickerPath;
    CollisionTraverser        _collisionTraverser;
    PT(CollisionHandlerQueue) _collisionHandlerQueue;
    bool                      _hasPickedUnit;
    NodePath                  _lastPickedUnit;

    int                       _posx, _posy;
};

#endif // MOUSE_H