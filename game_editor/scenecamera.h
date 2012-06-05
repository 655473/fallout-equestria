#ifndef SCENECAMERA_H
# define SCENECAMERA_H

# include <panda3d/pandaFramework.h>
# include <panda3d/pandaSystem.h>
# include <panda3d/mouseWatcher.h>
# include <panda3d/collisionRay.h>
# include <panda3d/collisionHandlerQueue.h>
# include <panda3d/collisionTraverser.h>

enum
{
  MotionNone   = 0,
  MotionTop    = 1,
  MotionBottom = 2,
  MotionLeft   = 4,
  MotionRight  = 8
};

class SceneCamera
{
public:
  SceneCamera(WindowFramework* window, NodePath camera);

  void            Run(float elapsedTime);
  void            SetEnabledScroll(bool set) { _scrollEnabled = set; }

  void            SwapCameraView(void);

  void            SetLimits(unsigned int minX, unsigned int minY, unsigned int maxX, unsigned maxY)
  {
    _minPosX = minX;
    _minPosY = minY;
    _maxPosX = maxX;
    _maxPosY = maxY;
  }

private:
  void            RunScroll(float elapsedTime);

  WindowFramework* _window;
  GraphicsWindow*  _graphicWindow;
  NodePath         _camera;
  bool             _scrollEnabled;

  unsigned char    _currentCameraAngle;
  LPoint3f         _currentHpr;
  LPoint3f         _objectiveHpr;

  unsigned int     _minPosX, _minPosY, _maxPosX, _maxPosY;
};

#endif // SCENECAMERA_H