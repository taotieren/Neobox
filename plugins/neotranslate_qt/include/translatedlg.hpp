#ifndef TRANSLATEDLG_HPP
#define TRANSLATEDLG_HPP

#include <widgetbase.hpp>
#include <QPlainTextEdit>
#include <QTextEdit>

#include <translate.h>
#include <pluginobject.h>

class QPushButton;

class NeoTranslateDlg : public WidgetBase
{
  Q_OBJECT

protected:
  void showEvent(QShowEvent*) override;
  void hideEvent(QHideEvent *event) override;
  bool eventFilter(QObject*, QEvent*) override;

public:
  explicit NeoTranslateDlg(class YJson& setings);
  ~NeoTranslateDlg();
  void ToggleVisibility();
  void GetResultData(QUtf8StringView text);

private:
  friend class HeightCtrl;
  class YJson& m_Settings;
  QWidget* m_CenterWidget;
  class QPlainTextEdit *m_TextFrom;
  class QTextEdit *m_TextTo;
  class QComboBox *m_BoxFrom, *m_BoxTo;
  class Translate* m_Translate;
  class HeightCtrl* m_HeightCtrl;
  QPushButton* m_BtnReverse;
  QPushButton *m_BtnCopyFrom, *m_BtnCopyTo;
  QPushButton* m_BtnTransMode;
  QPoint m_LastPostion;
  QSize m_LastSize;
  bool m_LanPairChanged = false;
  bool m_TextFromChanged = false;
private:
  void SetStyleSheet();
  void SetupUi();
  class QWidget* ReferenceObject() const;
  void CreateFromRightMenu(QMouseEvent* event);
  void CreateToRightMenu(QMouseEvent* event);
  void AddCombbox(class QHBoxLayout* layout);
private slots:
  void ReverseLanguage();
  void ChangeLanguageSource(bool checked);
  void ChangeLanguageFrom(int index);
  void ChangeLanguageTo(int index);
};

#endif // TRANSLATEDLG_HPP
