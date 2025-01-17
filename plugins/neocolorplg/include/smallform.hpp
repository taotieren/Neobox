#ifndef SMALLFORM_HPP
#define SMALLFORM_HPP

#include <QWidget>

#include <windows.h>


namespace Ui {
  class SmallForm;
}

class SmallForm: public QWidget
{
  // Q_OBJECT

protected:
  void showEvent(QShowEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  // void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
public:
  explicit SmallForm(class YJson& settings);
  virtual ~SmallForm();
public:
  static void PickColor(YJson& settings);
private:
  void TransformPoint(QPoint& point);
  void SetColor(const QColor& color);
private:
  void GetScreenColor(int x, int y);
  void AutoPosition(const QPoint& point);
  static bool InstallHook();
  static bool UninstallHook();
  void QuitHook(bool succeed);
private:
  // QPoint WinPoint2QPoint(int x, int y) const;
  static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK LowLevelKeyProc(int nCode, WPARAM wParam, LPARAM lParam);
public slots:
  // void DoMouseMove(LPARAM lParam);
  void DoMouseWheel(LPARAM lParam);
  // void ScalTarget(int value);
public:
  // QPoint m_Position;
  QColor m_Color;
  static SmallForm* m_Instance;
  static HHOOK m_Hoock[2];
private:
  class YJson& m_Settings;
  Ui::SmallForm* ui;
  QScreen* m_Screen;
  short m_ScaleTimes;
  class SquareForm* m_SquareForm;
};

#endif // SMALLFORM_HPP